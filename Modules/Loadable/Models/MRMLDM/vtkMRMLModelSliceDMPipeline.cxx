/*==============================================================================

  Program: 3D Slicer

  Copyright (c)

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#include "vtkMRMLModelSliceDMPipeline.h"

// MRML includes
#include <vtkMRMLClipNode.h>
#include <vtkMRMLColorNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLFolderDisplayNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLProceduralColorNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkActor2D.h>
#include <vtkAlgorithmOutput.h>
#include <vtkAssignAttribute.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataSetAttributes.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkGeneralTransform.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkPlaneCutter.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkScalarsToColors.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkTransformPolyDataFilter.h>

// VTK includes: customization
#include <vtkGeometryFilter.h>
#include <vtkSampleImplicitFunctionFilter.h>

vtkStandardNewMacro(vtkMRMLModelSliceDMPipeline);

//------------------------------------------------------------------------------
vtkMRMLModelSliceDMPipeline::vtkMRMLModelSliceDMPipeline()
  : ModelNode{ nullptr }
  , Plane{ vtkSmartPointer<vtkPlane>::New() }
  , NodeToWorld{ vtkSmartPointer<vtkGeneralTransform>::New() }
  , TransformToSlice{ vtkSmartPointer<vtkTransform>::New() }
  , Transformer{ vtkSmartPointer<vtkTransformPolyDataFilter>::New() }
  , SurfaceExtractor{ vtkSmartPointer<vtkDataSetSurfaceFilter>::New() }
  , ModelWarper{ vtkSmartPointer<vtkTransformFilter>::New() }
  , Cutter{ vtkSmartPointer<vtkPlaneCutter>::New() }
  , GeometryFilter{ vtkSmartPointer<vtkGeometryFilter>::New() }
  , SliceDistance{ vtkSmartPointer<vtkSampleImplicitFunctionFilter>::New() }
  , Mapper{ vtkSmartPointer<vtkPolyDataMapper2D>::New() }
  , Actor{ vtkSmartPointer<vtkActor2D>::New() }
{
  this->Transformer->SetTransform(this->TransformToSlice);
  this->Cutter->SetPlane(this->Plane);
  this->Cutter->BuildTreeOff(); // the cutter crashes for complex geometries if build tree is enabled
  this->Cutter->SetInputConnection(this->ModelWarper->GetOutputPort());
  this->GeometryFilter->SetInputConnection(this->Cutter->GetOutputPort());

  // Projection is created from outer surface of volumetric meshes (for polydata surface
  // extraction is just shallow-copy)
  this->SurfaceExtractor->SetInputConnection(this->ModelWarper->GetOutputPort());
  this->SliceDistance->SetImplicitFunction(this->Plane);
  this->SliceDistance->SetInputConnection(this->SurfaceExtractor->GetOutputPort());
  this->Transformer->SetInputConnection(this->GeometryFilter->GetOutputPort());

  this->Actor->SetMapper(this->Mapper);
  this->Actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
}

//------------------------------------------------------------------------------
vtkMRMLModelSliceDMPipeline::~vtkMRMLModelSliceDMPipeline() = default;

//------------------------------------------------------------------------------
void vtkMRMLModelSliceDMPipeline::SetDisplayNode(vtkMRMLNode* displayNode)
{
  this->Superclass::SetDisplayNode(displayNode);
  this->UpdateModelNode();
}

//------------------------------------------------------------------------------
vtkMRMLModelDisplayNode* vtkMRMLModelSliceDMPipeline::GetModelDisplayNode() const
{
  return vtkMRMLModelDisplayNode::SafeDownCast(this->GetDisplayNode());
}

//------------------------------------------------------------------------------
void vtkMRMLModelSliceDMPipeline::UpdateModelNode()
{
  this->SetModelNode(this->GetModelDisplayNode() ? vtkMRMLModelNode::SafeDownCast(this->GetModelDisplayNode()->GetDisplayableNode()) : nullptr);
}

//------------------------------------------------------------------------------
void vtkMRMLModelSliceDMPipeline::SetModelNode(vtkMRMLModelNode* node)
{
  if (this->ModelNode == node)
  {
    return;
  }

  this->UpdateObserver(this->ModelNode, node, { vtkMRMLModelNode::MeshModifiedEvent, vtkMRMLTransformableNode::TransformModifiedEvent });
  this->ModelNode = node;
}

//------------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLModelSliceDMPipeline::GetModelNode() const
{
  return ModelNode;
}

//------------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkProp>> vtkMRMLModelSliceDMPipeline::GetManagedProps() const
{
  return { this->Actor };
}

//------------------------------------------------------------------------------
void vtkMRMLModelSliceDMPipeline::OnRendererAdded(vtkRenderer* renderer)
{
  if (renderer && !renderer->HasViewProp(this->Actor))
  {
    renderer->AddViewProp(this->Actor);
  }
}

//------------------------------------------------------------------------------
void vtkMRMLModelSliceDMPipeline::OnRendererRemoved(vtkRenderer* renderer)
{
  if (renderer && renderer->HasViewProp(this->Actor))
  {
    renderer->RemoveViewProp(this->Actor);
  }
}

//------------------------------------------------------------------------------
void vtkMRMLModelSliceDMPipeline::OnUpdate(vtkObject* obj, unsigned long eventId, void* callData)
{
  if (obj == this->GetDisplayNode())
  {
    if (eventId == vtkMRMLNode::ReferenceAddedEvent || eventId == vtkMRMLNode::ReferenceRemovedEvent)
    {
      this->UpdateModelNode();
    }
  }
  this->ResetDisplay();
}

//------------------------------------------------------------------------------
void vtkMRMLModelSliceDMPipeline::UpdatePipeline()
{
  // Sets visibility, set pipeline mesh input, update color
  //   calculate and set pipeline transforms.

  vtkMRMLModelDisplayNode* originalModelDisplayNode = this->GetModelDisplayNode();
  if (!originalModelDisplayNode || !this->GetViewNode())
  {
    this->Actor->SetVisibility(false);
    this->RequestRender();
    return;
  }

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(this->GetViewNode());
  if (!sliceNode)
  {
    this->Actor->SetVisibility(false);
    this->RequestRender();
    return;
  }

  // Exclude fiber bundle display nodes (handled by a dedicated displayable manager)
  if (originalModelDisplayNode->IsA("vtkMRMLFiberBundleLineDisplayNode") || originalModelDisplayNode->IsA("vtkMRMLFiberBundleGlyphDisplayNode"))
  {
    this->Actor->SetVisibility(false);
    this->RequestRender();
    return;
  }

  // Get display node from hierarchy that applies display properties on branch
  vtkMRMLDisplayableNode* displayableNode = originalModelDisplayNode->GetDisplayableNode();
  vtkMRMLDisplayNode* overrideHierarchyDisplayNode = vtkMRMLFolderDisplayNode::GetOverridingHierarchyDisplayNode(displayableNode);

  // Use hierarchy display node if any, and if overriding is allowed for the current display node.
  // If override is explicitly disabled, then do not apply hierarchy visibility or opacity either.
  bool hierarchyVisibility = true;
  double hierarchyOpacity = 1.0;
  if (originalModelDisplayNode->GetFolderDisplayOverrideAllowed())
  {
    if (overrideHierarchyDisplayNode)
    {
      originalModelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(overrideHierarchyDisplayNode);
    }

    // Get visibility and opacity defined by the hierarchy.
    // These two properties are influenced by the hierarchy regardless the fact whether there is override
    // or not. Visibility of items defined by hierarchy is off if any of the ancestors is explicitly hidden,
    // and the opacity is the product of the ancestors' opacities.
    // However, this does not apply on display nodes that do not allow overrides (FolderDisplayOverrideAllowed)
    hierarchyVisibility = vtkMRMLFolderDisplayNode::GetHierarchyVisibility(displayableNode);
    hierarchyOpacity = vtkMRMLFolderDisplayNode::GetHierarchyOpacity(displayableNode);
  }

  // Visibility checks (slice intersection models are shown by crosshair DM)
  if (vtkMRMLSliceLogic::IsSliceModelDisplayNode(originalModelDisplayNode))
  {
    this->Actor->SetVisibility(false);
    this->RequestRender();
    return;
  }

  bool visibilityOnNode = originalModelDisplayNode->GetVisibility() && originalModelDisplayNode->IsDisplayableInView(this->GetViewNode()->GetID());
  if (!visibilityOnNode || (originalModelDisplayNode->GetVisibility2D() == 0))
  {
    this->Actor->SetVisibility(false);
    this->RequestRender();
    return;
  }

  vtkMRMLModelNode* modelNode = this->GetModelNode();
  if (!modelNode)
  {
    this->Actor->SetVisibility(false);
    this->RequestRender();
    return;
  }

  vtkPointSet* pointSet = originalModelDisplayNode->GetOutputMesh();
  if (!pointSet)
  {
    this->Actor->SetVisibility(false);
    this->RequestRender();
    return;
  }

  // Need this to update bounds of the locator, to avoid crash in the cutter
  originalModelDisplayNode->GetOutputMeshConnection()->GetProducer()->Update();

  // there are no points, so there is nothing to cut
  if (!pointSet->GetPoints() || pointSet->GetNumberOfPoints() == 0)
  {
    this->Actor->SetVisibility(false);
    this->RequestRender();
    return;
  }

  // Update transforms
  this->NodeToWorld->Identity();
  vtkMRMLTransformNode* transformNode = modelNode->GetParentTransformNode();
  if (transformNode)
  {
    transformNode->GetTransformToWorld(this->NodeToWorld);
  }

  this->ModelWarper->SetInputData(pointSet);
  this->ModelWarper->SetTransform(this->NodeToWorld);

  // Set Plane Transform
  this->UpdateSlicePlaneFromMatrix();

  // Configure pipeline based on slice display mode
  bool isIntersectionMode = originalModelDisplayNode->GetSliceDisplayMode() == vtkMRMLModelDisplayNode::SliceDisplayIntersection;

  if (isIntersectionMode)
  {
    // show intersection in the slice view
    this->UpdateProjectionTransform();
    this->Transformer->SetInputConnection(this->GeometryFilter->GetOutputPort());

    // If there is no input or if the input has no points, the vtkTransformPolyDataFilter will display an error message
    // on every update: "No input data".
    // To prevent the error, if the input is empty then the actor should not be visible since there is nothing to display.
    this->GeometryFilter->Update();
    if (!this->GeometryFilter->GetOutput() || this->GeometryFilter->GetOutput()->GetNumberOfPoints() < 1)
    {
      this->Actor->SetVisibility(false);
      return;
    }
  }
  else if (originalModelDisplayNode->GetSliceDisplayMode() == vtkMRMLModelDisplayNode::SliceDisplayProjection)
  {
    // remove cutter from the pipeline if projection mode is used, we just need to extract surface
    // and flatten the model
    this->UpdateProjectionTransform();
    this->Transformer->SetInputConnection(this->SurfaceExtractor->GetOutputPort());
  }
  else if (originalModelDisplayNode->GetSliceDisplayMode() == vtkMRMLModelDisplayNode::SliceDisplayDistanceEncodedProjection)
  {
    // replace cutter in the pipeline by surface extraction, slice distance computation,
    // and flattening of the model
    this->TransformToSlice->Identity();
    this->UpdateProjectionTransform();
    this->Transformer->SetInputConnection(this->SliceDistance->GetOutputPort());
  }

  // Resolve display node for scalar/color properties (hierarchy may override)
  vtkMRMLDisplayNode* displayNode = originalModelDisplayNode;
  if (originalModelDisplayNode->GetFolderDisplayOverrideAllowed() && overrideHierarchyDisplayNode)
  {
    displayNode = overrideHierarchyDisplayNode;
  }

  // Connect mapper to transformer output
  vtkPolyDataMapper2D* mapper = vtkPolyDataMapper2D::SafeDownCast(this->Actor->GetMapper());
  if (mapper)
  {
    mapper->SetInputConnection(this->Transformer->GetOutputPort());

    // Set scalar/color mapping
    if (originalModelDisplayNode->GetSliceDisplayMode() == vtkMRMLModelDisplayNode::SliceDisplayDistanceEncodedProjection)
    {
      // Distance-encoded projection: color from slice distance lookup table
      // (handled independently of displayNode->GetScalarVisibility())
      vtkMRMLColorNode* colorNode = originalModelDisplayNode->GetDistanceEncodedProjectionColorNode();
      vtkSmartPointer<vtkScalarsToColors> lut = nullptr;
      vtkSmartPointer<vtkLookupTable> dNodeLUT = nullptr;

      if (colorNode)
      {
        if (vtkMRMLProceduralColorNode* proceduralColor = vtkMRMLProceduralColorNode::SafeDownCast(colorNode))
        {
          lut = proceduralColor->GetColorTransferFunction();
        }

        if (!lut)
        {
          dNodeLUT = vtkSmartPointer<vtkLookupTable>::Take(colorNode->CreateLookupTableCopy());
          if (dNodeLUT)
          {
            lut = dNodeLUT;
            mapper->SetScalarRange(displayNode->GetScalarRange());
            lut->SetAlpha(hierarchyOpacity * displayNode->GetSliceIntersectionOpacity());
          }
        }
      }

      if (lut != nullptr)
      {
        mapper->SetLookupTable(lut);
        mapper->SetScalarRange(lut->GetRange());
        mapper->SetScalarVisibility(true);
      }
      else
      {
        mapper->SetScalarVisibility(false);
      }
    }
    else if (displayNode->GetScalarVisibility())
    {
      // Check if using point data or cell data
      bool isCellScalarsActive = false;
      if (displayNode->GetActiveScalarName())
      {
        isCellScalarsActive = (displayNode->GetActiveAttributeLocation() == vtkAssignAttribute::CELL_DATA);
      }
      else if (modelNode)
      {
        isCellScalarsActive = static_cast<bool>(modelNode->GetActiveCellScalarName(vtkDataSetAttributes::SCALARS));
      }

      if (isCellScalarsActive)
      {
        mapper->SetScalarModeToUseCellData();
      }
      else
      {
        mapper->SetScalarModeToUsePointData();
      }

      if (displayNode->GetScalarRangeFlag() == vtkMRMLDisplayNode::UseDirectMapping)
      {
        mapper->SetColorModeToDirectScalars();
        mapper->SetLookupTable(nullptr);
        mapper->SetScalarRange(displayNode->GetScalarRange());
        mapper->SetScalarVisibility(true);
      }
      else
      {
        mapper->SetColorModeToMapScalars();

        // The renderer uses the lookup table scalar range to
        // render colors. By default, UseLookupTableScalarRange
        // is set to false and SetScalarRange can be used on the
        // mapper to map scalars into the lookup table. When set
        // to true, SetScalarRange has no effect and it is necessary
        // to force the scalarRange on the lookup table manually.
        // Whichever way is used, the look up table range needs
        // to be changed to render the correct scalar values, thus
        // one lookup table can not be shared by multiple mappers
        // if any of those mappers needs to map using its scalar
        // values range. It is therefore necessary to make a copy
        // of the colorNode vtkLookupTable in order not to impact
        // that lookup table original range.
        vtkSmartPointer<vtkLookupTable> dNodeLUT =
          vtkSmartPointer<vtkLookupTable>::Take(displayNode->GetColorNode() != nullptr ? displayNode->GetColorNode()->CreateLookupTableCopy() : nullptr);
        if (dNodeLUT)
        {
          dNodeLUT->SetAlpha(hierarchyOpacity * displayNode->GetSliceIntersectionOpacity());
        }
        mapper->SetLookupTable(dNodeLUT);
        mapper->SetScalarRange(displayNode->GetScalarRange());
        mapper->SetScalarVisibility(true);
      }
    }
    else
    {
      mapper->SetScalarVisibility(false);
    }
  }

  // Update actor2D properties

  // Opacity of the slice intersection is intentionally not set by
  // actorProps->SetOpacity(displayNode->GetOpacity()),
  // because most often users only want to make 3D model transparent.
  // Visibility of slice intersections can be tuned by modifying
  // slice intersection thickness or a new SliceIntersectionOpacity attribute
  // could be introduced.

  vtkProperty2D* actorProps = this->Actor->GetProperty();
  actorProps->SetColor(displayNode->GetColor());
  actorProps->SetPointSize(displayNode->GetSliceIntersectionThickness());
  actorProps->SetLineWidth(displayNode->GetSliceIntersectionThickness());

  double intersectionOpacity = displayNode->GetSliceIntersectionOpacity();
  double opacity = hierarchyVisibility ? (hierarchyOpacity * intersectionOpacity) : 0.0;
  actorProps->SetOpacity(opacity);

  bool finalVisibility = hierarchyVisibility && visibilityOnNode;
  this->UpdateActor2DProperties(actorProps, opacity, finalVisibility, originalModelDisplayNode);

  this->Actor->SetPosition(0, 0);
  this->Actor->SetVisibility(finalVisibility);

  this->RequestRender();
}

//------------------------------------------------------------------------------
void vtkMRMLModelSliceDMPipeline::UpdateSlicePlaneFromMatrix()
{
  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(this->GetViewNode());
  if (!sliceNode || !sliceNode->GetXYToRAS())
  {
    return;
  }

  double normal[3];
  double origin[3];

  // +/-1: orientation of the normal
  const int planeOrientation = 1;
  for (int i = 0; i < 3; i++)
  {
    normal[i] = planeOrientation * sliceNode->GetXYToRAS()->GetElement(i, 2);
    origin[i] = sliceNode->GetXYToRAS()->GetElement(i, 3);
  }

  vtkMath::Normalize(normal);
  this->Plane->SetNormal(normal);
  this->Plane->SetOrigin(origin);
  this->Plane->Modified();
}

//------------------------------------------------------------------------------
bool vtkMRMLModelSliceDMPipeline::IsIntersectionMode() const
{
  vtkMRMLModelDisplayNode* modelDisplayNode = this->GetModelDisplayNode();
  if (!modelDisplayNode)
  {
    return false;
  }

  return modelDisplayNode->GetSliceDisplayMode() == vtkMRMLModelDisplayNode::SliceDisplayIntersection;
}

//------------------------------------------------------------------------------
void vtkMRMLModelSliceDMPipeline::UpdateProjectionTransform() const
{
  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(this->GetViewNode());
  if (!sliceNode || !sliceNode->GetXYToRAS())
  {
    return;
  }

  vtkNew<vtkMatrix4x4> rasToSliceXY;
  vtkMatrix4x4::Invert(sliceNode->GetXYToRAS(), rasToSliceXY.GetPointer());

  if (!this->IsIntersectionMode())
  {
    // Project all points to the slice plane (slice Z coordinate = 0)
    rasToSliceXY->SetElement(2, 0, 0);
    rasToSliceXY->SetElement(2, 1, 0);
    rasToSliceXY->SetElement(2, 2, 0);
  }

  this->TransformToSlice->SetMatrix(rasToSliceXY.GetPointer());
}

//------------------------------------------------------------------------------
void vtkMRMLModelSliceDMPipeline::UpdateActor2DProperties(vtkProperty2D* prop, double opacity, bool visibility, vtkMRMLModelDisplayNode*) const
{
  if (!prop)
  {
    return;
  }
  prop->SetOpacity(opacity);
}
