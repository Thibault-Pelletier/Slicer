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

#include "vtkMRMLModelDMPipeline.h"

// MRML includes
#include <vtkMRMLClipNode.h>
#include <vtkMRMLColorNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLFolderDisplayNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkActor.h>
#include <vtkAlgorithmOutput.h>
#include <vtkAssignAttribute.h>
#include <vtkCapPolyData.h>
#include <vtkClipDataSet.h>
#include <vtkClipPolyData.h>
#include <vtkDataSetAttributes.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractGeometry.h>
#include <vtkExtractPolyDataGeometry.h>
#include <vtkGeneralTransform.h>
#include <vtkImplicitBoolean.h>
#include <vtkImplicitFunction.h>
#include <vtkLookupTable.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTexture.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

vtkStandardNewMacro(vtkMRMLModelDMPipeline);

vtkMRMLModelDMPipeline::vtkMRMLModelDMPipeline()
  : ModelNode{ nullptr }
  , TransformFilter{ vtkSmartPointer<vtkTransformFilter>::New() }
  , Transform{ vtkSmartPointer<vtkTransform>::New() }
  , NonLinearTransform{ vtkSmartPointer<vtkGeneralTransform>::New() }
  , Clipper{ nullptr }
  , Capper{ vtkSmartPointer<vtkCapPolyData>::New() }
  , Mapper{ vtkSmartPointer<vtkPolyDataMapper>::New() }
  , Actor{ vtkSmartPointer<vtkActor>::New() }
  , ImageActor{ vtkSmartPointer<vtkImageActor>::New() }
  , CapMapper{ vtkSmartPointer<vtkPolyDataMapper>::New() }
  , CapActor{ vtkSmartPointer<vtkActor>::New() }
{
  this->TransformFilter->SetTransform(this->Transform);
  this->CapActor->SetMapper(this->CapMapper);
  this->Actor->SetMapper(this->Mapper);
}

vtkMRMLModelDMPipeline::~vtkMRMLModelDMPipeline()
{
  this->SetModelNode(nullptr);
  this->SetDisplayNode(nullptr);

  this->TransformFilter->RemoveAllInputs();
  this->Mapper->RemoveAllInputs();
  this->CapMapper->RemoveAllInputs();

  if (this->Mapper)
  {
    this->Mapper->SetInputConnection(nullptr);
  }
  if (this->CapMapper)
  {
    this->CapMapper->SetInputConnection(nullptr);
  }
  if (this->TransformFilter)
  {
    this->TransformFilter->SetInputConnection(nullptr);
  }

  if (this->Clipper)
  {
    this->Clipper->SetInputConnection(nullptr);
  }
  this->Capper->SetInputConnection(nullptr);
  this->Capper->SetClipFunction(nullptr);

  if (this->ImageActor && this->ImageActor->GetMapper())
  {
    this->ImageActor->GetMapper()->SetInputConnection(nullptr);
  }
  if (this->Actor)
  {
    this->Actor->SetMapper(nullptr);
  }
  if (this->CapActor)
  {
    this->CapActor->SetMapper(nullptr);
  }
}

void vtkMRMLModelDMPipeline::SetDisplayNode(vtkMRMLNode* displayNode)
{
  this->Superclass::SetDisplayNode(displayNode);
  this->UpdateModelNode();
}

vtkMRMLModelDisplayNode* vtkMRMLModelDMPipeline::GetModelDisplayNode() const
{
  return vtkMRMLModelDisplayNode::SafeDownCast(this->GetDisplayNode());
}

void vtkMRMLModelDMPipeline::UpdateModelNode()
{
  this->SetModelNode(this->GetModelDisplayNode() ? vtkMRMLModelNode::SafeDownCast(this->GetModelDisplayNode()->GetDisplayableNode()) : nullptr);
}

void vtkMRMLModelDMPipeline::SetModelNode(vtkMRMLModelNode* node)
{
  if (this->ModelNode == node)
  {
    return;
  }

  this->UpdateObserver(this->ModelNode, node, { vtkMRMLModelNode::MeshModifiedEvent, vtkMRMLClipNode::ClipNodeModifiedEvent, vtkMRMLTransformableNode::TransformModifiedEvent });
  this->ModelNode = node;
}

vtkMRMLModelNode* vtkMRMLModelDMPipeline::GetModelNode() const
{
  return ModelNode;
}

vtkMRMLDisplayNode* vtkMRMLModelDMPipeline::GetOverrideDisplayNode() const
{
  vtkMRMLModelDisplayNode* modelDisplayNode = this->GetModelDisplayNode();
  if (!modelDisplayNode)
  {
    return nullptr;
  }
  if (modelDisplayNode->GetFolderDisplayOverrideAllowed())
  {
    if (vtkMRMLDisplayNode* overrideHierarchyDisplayNode = vtkMRMLFolderDisplayNode::GetOverridingHierarchyDisplayNode(modelDisplayNode->GetDisplayableNode()))
    {
      return overrideHierarchyDisplayNode;
    }
  }
  return modelDisplayNode;
}

std::vector<vtkSmartPointer<vtkProp>> vtkMRMLModelDMPipeline::GetManagedProps() const
{
  return { this->Actor, this->CapActor };
}

void vtkMRMLModelDMPipeline::OnUpdate(vtkObject* obj, unsigned long eventId, void* callData)
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

void vtkMRMLModelDMPipeline::OnRendererAdded(vtkRenderer* renderer)
{
  if (renderer && !renderer->HasViewProp(this->Actor))
  {
    renderer->AddViewProp(this->Actor);
    renderer->AddViewProp(this->CapActor);
  }
}

void vtkMRMLModelDMPipeline::OnRendererRemoved(vtkRenderer* renderer)
{
  if (renderer && renderer->HasViewProp(this->Actor))
  {
    renderer->RemoveViewProp(this->Actor);
    renderer->RemoveViewProp(this->CapActor);
    renderer->RemoveViewProp(this->ImageActor);
  }
}

void vtkMRMLModelDMPipeline::UpdatePipeline()
{
  this->UpdateClipper();
  this->UpdateMapperConnection();
  this->UpdateTransform();
  this->UpdateDisplayProperty();
  this->RequestRender();
}

void vtkMRMLModelDMPipeline::UpdateClipper()
{
  if (!this->HasClipping())
  {
    this->Capper->SetClipFunction(nullptr);
    this->Clipper = nullptr;
    return;
  }

  vtkMRMLClipNode* clipNode = this->GetModelDisplayNode()->GetClipNode();

  // Check for non-linear transform (needed for guard below)
  bool hasNonLinearTransform = (this->NonLinearTransform->GetNumberOfConcatenatedTransforms() > 0);

  vtkSmartPointer<vtkImplicitBoolean> implicitBoolean = vtkSmartPointer<vtkImplicitBoolean>::New();

  if (clipNode)
  {
    vtkImplicitFunction* clipFunction = clipNode->GetImplicitFunctionWorld();
    if (clipFunction)
    {
      implicitBoolean->AddFunction(clipFunction);
    }
  }

  // If the transform is non-linear, worldTransform will have already been set.
  // Only need to calculate here for linear transforms.
  vtkMRMLTransformNode* tnode = this->GetModelNode()->GetParentTransformNode();
  if (implicitBoolean && tnode && !hasNonLinearTransform && tnode->IsTransformToWorldLinear())
  {
    vtkNew<vtkGeneralTransform> worldTransform;
    tnode->GetTransformToWorld(worldTransform);
    implicitBoolean->SetTransform(worldTransform);
  }
  else if (implicitBoolean)
  {
    // Non-linear case: clip functions work in world space, use nullptr
    implicitBoolean->SetTransform(static_cast<vtkAbstractTransform*>(nullptr));
  }

  vtkSmartPointer<vtkImplicitFunction> finalClipFunction = implicitBoolean;

  const vtkMRMLModelNode::MeshTypeHint type = this->GetModelNode() ? this->GetModelNode()->GetMeshType() : vtkMRMLModelNode::PolyDataMeshType;
  const int clippingMethod = clipNode->GetClippingMethod();

  if (type == vtkMRMLModelNode::UnstructuredGridMeshType)
  {
    if (clippingMethod == vtkMRMLClipNode::Straight)
    {
      if (!vtkClipDataSet::SafeDownCast(this->Clipper))
      {
        this->Clipper = vtkSmartPointer<vtkClipDataSet>::New();
      }
      vtkClipDataSet::SafeDownCast(this->Clipper)->SetClipFunction(finalClipFunction);
    }
    else
    {
      if (!vtkExtractGeometry::SafeDownCast(this->Clipper))
      {
        this->Clipper = vtkSmartPointer<vtkExtractGeometry>::New();
      }
      auto extractGeometry = vtkExtractGeometry::SafeDownCast(this->Clipper);
      extractGeometry->SetImplicitFunction(finalClipFunction);
      extractGeometry->ExtractInsideOff();
      if (clippingMethod == vtkMRMLClipNode::WholeCellsWithBoundary)
      {
        extractGeometry->ExtractBoundaryCellsOn();
      }
    }
  }
  else
  {
    if (clippingMethod == vtkMRMLClipNode::Straight)
    {
      if (!vtkClipPolyData::SafeDownCast(this->Clipper))
      {
        this->Clipper = vtkSmartPointer<vtkClipPolyData>::New();
      }
      auto clipPolyData = vtkClipPolyData::SafeDownCast(this->Clipper);
      clipPolyData->SetValue(0.0);
      clipPolyData->SetClipFunction(finalClipFunction);
    }
    else
    {
      if (!vtkExtractPolyDataGeometry::SafeDownCast(this->Clipper))
      {
        this->Clipper = vtkSmartPointer<vtkExtractPolyDataGeometry>::New();
      }
      auto extractPolyDataGeometry = vtkExtractPolyDataGeometry::SafeDownCast(this->Clipper);
      extractPolyDataGeometry->SetImplicitFunction(finalClipFunction);
      extractPolyDataGeometry->ExtractInsideOff();
      if (clippingMethod == vtkMRMLClipNode::WholeCellsWithBoundary)
      {
        extractPolyDataGeometry->ExtractBoundaryCellsOn();
      }
    }

    if (!vtkCapPolyData::SafeDownCast(this->Capper))
    {
      this->Capper = vtkSmartPointer<vtkCapPolyData>::New();
    }
    vtkCapPolyData::SafeDownCast(this->Capper)->SetClipFunction(finalClipFunction);
  }
}

bool vtkMRMLModelDMPipeline::HasClipping() const
{
  if (!this->GetModelNode() || !this->GetModelDisplayNode())
  {
    return false;
  }

  vtkMRMLDisplayNode* displayNodeForClipping = this->GetOverrideDisplayNode();
  if (!displayNodeForClipping || !displayNodeForClipping->GetClipping())
  {
    return false;
  }

  const auto clipNode = this->GetModelDisplayNode()->GetClipNode();
  if (!clipNode || !clipNode->GetImplicitFunctionWorld())
  {
    return false;
  }

  for (int i = 0; i < clipNode->GetNumberOfClippingNodes(); ++i)
  {
    if (clipNode->GetNthClippingNodeState(i) != vtkMRMLClipNode::ClipOff)
    {
      return true;
    }
  }
  return false;
}

void vtkMRMLModelDMPipeline::UpdateTransform() const
{
  vtkMRMLModelNode* modelNode = this->GetModelNode();
  if (!modelNode)
  {
    return;
  }

  vtkMRMLTransformNode* tnode = modelNode->GetParentTransformNode();
  if (tnode != nullptr && tnode->IsTransformToWorldLinear())
  {
    vtkNew<vtkMatrix4x4> matrix;
    tnode->GetMatrixTransformToWorld(matrix.GetPointer());
    this->Actor->SetUserMatrix(matrix);
    this->CapActor->SetUserMatrix(matrix);
    this->Transform->Identity();
    this->NonLinearTransform->Identity();
  }
  else
  {
    this->Actor->SetUserMatrix(nullptr);
    this->CapActor->SetUserMatrix(nullptr);
    if (tnode != nullptr)
    {
      // Get transformation applied on model
      tnode->GetTransformToWorld(this->NonLinearTransform);
      this->Transform->Identity();
    }
    else
    {
      this->Transform->Identity();
      this->NonLinearTransform->Identity();
    }
  }
}

void vtkMRMLModelDMPipeline::UpdateMapperConnection()
{
  vtkMRMLModelNode* modelNode = this->GetModelNode();
  vtkMRMLModelDisplayNode* modelDisplayNode = this->GetModelDisplayNode();

  vtkAlgorithmOutput* meshConnection = modelDisplayNode ? modelDisplayNode->GetOutputMeshConnection() : nullptr;

  // Check hierarchy override for mesh source
  if (vtkMRMLModelNode* hierarchyModelNode = vtkMRMLModelNode::SafeDownCast(this->GetOverrideDisplayNode()))
  {
    meshConnection = hierarchyModelNode->GetMeshConnection();
  }

  if (!meshConnection && modelNode)
  {
    meshConnection = modelNode->GetMeshConnection();
  }

  if (!meshConnection)
  {
    this->Mapper->SetInputConnection(nullptr);
    this->CapMapper->SetInputConnection(nullptr);
    return;
  }

  vtkAlgorithmOutput* currentOutput = meshConnection;
  vtkAlgorithmOutput* clippingInputSource = currentOutput;

  const vtkMRMLModelNode::MeshTypeHint type = this->GetModelNode() ? this->GetModelNode()->GetMeshType() : vtkMRMLModelNode::PolyDataMeshType;
  if (type == vtkMRMLModelNode::UnstructuredGridMeshType)
  {
    if (!vtkDataSetMapper::SafeDownCast(this->Mapper))
    {
      this->Mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    }
    if (!vtkDataSetMapper::SafeDownCast(this->CapMapper))
    {
      this->CapMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    }
  }
  else
  {
    if (!vtkPolyDataMapper::SafeDownCast(this->Mapper))
    {
      this->Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    }
    if (!vtkPolyDataMapper::SafeDownCast(this->CapMapper))
    {
      this->CapMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    }
  }
  this->Actor->SetMapper(this->Mapper);
  this->CapActor->SetMapper(this->CapMapper);

  if (this->NonLinearTransform->GetNumberOfConcatenatedTransforms() > 0)
  {
    this->TransformFilter->SetTransform(this->NonLinearTransform);
    this->TransformFilter->SetInputConnection(currentOutput);
    currentOutput = this->TransformFilter->GetOutputPort();
    clippingInputSource = currentOutput;
  }

  if (this->Clipper)
  {
    this->Clipper->SetInputConnection(clippingInputSource);
    currentOutput = this->Clipper->GetOutputPort();
  }

  this->Mapper->SetInputConnection(currentOutput);

  // Clipping Cap
  if (this->Clipper && modelDisplayNode && (modelDisplayNode->GetClippingCapSurface() || modelDisplayNode->GetClippingOutline()))
  {
    this->Capper->SetInputConnection(clippingInputSource);
    this->CapMapper->SetInputConnection(this->Capper->GetOutputPort());
  }
  else
  {
    this->CapMapper->SetInputConnection(nullptr);
  }
}

void vtkMRMLModelDMPipeline::UpdateDisplayProperty() const
{
  vtkMRMLModelDisplayNode* modelDisplayNode = this->GetModelDisplayNode();
  if (!modelDisplayNode || !this->GetViewNode() || !this->GetRenderer())
  {
    this->Actor->SetVisibility(false);
    this->CapActor->SetVisibility(false);
    return;
  }

  bool hierarchyVisibility = true;
  double hierarchyOpacity = 1.0;
  vtkMRMLDisplayNode* displayNode = this->GetOverrideDisplayNode();
  if (modelDisplayNode->GetFolderDisplayOverrideAllowed())
  {
    // Get visibility and opacity defined by the hierarchy.
    // These two properties are influenced by the hierarchy regardless the fact whether there is override
    // or not. Visibility of items defined by hierarchy is off if any of the ancestors is explicitly hidden,
    // and the opacity is the product of the ancestors' opacities.
    // However, this does not apply on display nodes that do not allow overrides (FolderDisplayOverrideAllowed)
    hierarchyVisibility = vtkMRMLFolderDisplayNode::GetHierarchyVisibility(modelDisplayNode->GetDisplayableNode());
    hierarchyOpacity = vtkMRMLFolderDisplayNode::GetHierarchyOpacity(modelDisplayNode->GetDisplayableNode());
  }

  // If there is an overriding hierarchy display node, then consider its visibility as well
  // as the model's. It is important to consider the model's visibility, because the user will
  // still want to show/hide children regardless of application of display properties from the
  // hierarchy.
  bool visibility =
    hierarchyVisibility && modelDisplayNode->GetVisibility() && modelDisplayNode->GetVisibility3D() && modelDisplayNode->IsDisplayableInView(this->GetViewNode()->GetID());

  double opacity = displayNode->GetOpacity() * hierarchyOpacity;

  bool capVisibility = visibility && modelDisplayNode->GetClipping() && (modelDisplayNode->GetClippingCapSurface() || modelDisplayNode->GetClippingOutline());

  this->UpdateMapperProperties(this->Mapper);
  this->UpdateActorProperties(this->Actor, opacity, visibility);

  this->UpdateMapperProperties(this->CapMapper);
  this->UpdateCapActorProperties(opacity, capVisibility);

  if (vtkAlgorithmOutput* textureConnection = modelDisplayNode->GetTextureImageDataConnection())
  {
    this->ImageActor->SetVisibility(visibility);
    this->ImageActor->SetUserMatrix(this->Actor->GetUserMatrix());
    this->ImageActor->GetMapper()->SetInputConnection(textureConnection);
    this->ImageActor->SetDisplayExtent(-1, 0, 0, 0, 0, 0);
    if (!this->GetRenderer()->HasViewProp(this->ImageActor))
    {
      this->GetRenderer()->AddViewProp(this->ImageActor);
    }
  }
  else
  {
    if (this->GetRenderer()->HasViewProp(this->ImageActor))
    {
      this->GetRenderer()->RemoveViewProp(this->ImageActor);
    }
  }
}

void vtkMRMLModelDMPipeline::UpdateMapperProperties(vtkMapper* mapper) const
{
  vtkMRMLDisplayNode* displayNode = this->GetOverrideDisplayNode();

  if (!mapper || !displayNode)
  {
    return;
  }

  mapper->SetScalarVisibility(displayNode->GetScalarVisibility());
  // if the scalars are visible, set active scalars, the lookup table
  // and the scalar range
  if (displayNode->GetScalarVisibility())
  {
    // Check if using point data or cell data
    bool isCellScalarsActive = false;
    if (displayNode->GetActiveScalarName())
    {
      isCellScalarsActive = (displayNode->GetActiveAttributeLocation() == vtkAssignAttribute::CELL_DATA);
    }
    else if (this->GetModelNode() && this->GetModelNode()->GetActiveCellScalarName(vtkDataSetAttributes::SCALARS))
    {
      isCellScalarsActive = true;
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
      mapper->UseLookupTableScalarRangeOn();
      mapper->SetColorModeToDirectScalars();
      mapper->SetLookupTable(nullptr);
    }
    else
    {
      mapper->UseLookupTableScalarRangeOff();
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
        vtkSmartPointer<vtkLookupTable>::Take(displayNode->GetColorNode() ? displayNode->GetColorNode()->CreateLookupTableCopy() : nullptr);
      mapper->SetLookupTable(dNodeLUT);
    }
    mapper->SetScalarRange(displayNode->GetScalarRange());
  }
}

void vtkMRMLModelDMPipeline::UpdateActorProperties(vtkActor* actor, double opacity, bool visibility) const
{
  auto modelNode = this->GetModelNode();
  if (!actor || !modelNode)
  {
    return;
  }

  actor->SetVisibility(visibility);
  if (!visibility)
  {
    return;
  }

  vtkMRMLModelDisplayNode* modelDisplayNode = this->GetModelDisplayNode();
  vtkMRMLDisplayNode* displayNode = this->GetOverrideDisplayNode();

  if (!displayNode)
  {
    return;
  }

  vtkProperty* actorProperties = actor->GetProperty();
  actorProperties->SetRepresentation(displayNode->GetRepresentation());
  actorProperties->SetPointSize(static_cast<float>(displayNode->GetPointSize()));
  actorProperties->SetLineWidth(static_cast<float>(displayNode->GetLineWidth()));
  actorProperties->SetLighting(displayNode->GetLighting());
  actorProperties->SetInterpolation(displayNode->GetInterpolation());
  actorProperties->SetShading(displayNode->GetShading());
  actorProperties->SetFrontfaceCulling(displayNode->GetFrontfaceCulling());
  actorProperties->SetBackfaceCulling(displayNode->GetBackfaceCulling());

  actor->SetPickable(modelNode->GetSelectable());
  if (displayNode->GetSelected())
  {
    actorProperties->SetColor(displayNode->GetSelectedColor());
    actorProperties->SetAmbient(displayNode->GetSelectedAmbient());
    actorProperties->SetSpecular(displayNode->GetSelectedSpecular());
  }
  else
  {
    actorProperties->SetColor(displayNode->GetColor());
    actorProperties->SetAmbient(displayNode->GetAmbient());
    actorProperties->SetSpecular(displayNode->GetSpecular());
  }
  // Opacity will be the product of the opacities of the model and the overriding
  // hierarchy, in order to keep the relative opacities the same.
  actorProperties->SetOpacity(opacity);
  actorProperties->SetDiffuse(displayNode->GetDiffuse());
  actorProperties->SetSpecularPower(displayNode->GetPower());
  actorProperties->SetMetallic(displayNode->GetMetallic());
  actorProperties->SetRoughness(displayNode->GetRoughness());
  actorProperties->SetEdgeVisibility(displayNode->GetEdgeVisibility());
  actorProperties->SetEdgeColor(displayNode->GetEdgeColor());

  if (displayNode->GetTextureImageDataConnection() != nullptr)
  {
    if (actor->GetTexture() == nullptr)
    {
      vtkNew<vtkTexture> texture;
      actor->SetTexture(texture);
    }
    actor->GetTexture()->SetInputConnection(displayNode->GetTextureImageDataConnection());
    actor->GetTexture()->SetInterpolate(displayNode->GetInterpolateTexture());
    actorProperties->SetColor(1., 1., 1.);

    // Force actors to be treated as opaque. Otherwise, transparent
    // elements in the texture cause the actor to be treated as
    // translucent, i.e. rendered without writing to the depth buffer.
    // See https://github.com/Slicer/Slicer/issues/4253.
    actor->SetForceOpaque(actorProperties->GetOpacity() >= 1.0);
  }
  else
  {
    actor->SetTexture(nullptr);
    if (vtkPolyDataMapper::SafeDownCast(actor->GetMapper()))
    {
      actor->ForceOpaqueOff();
    }
  }

  // Set backface properties
  vtkProperty* actorBackfaceProperties = actor->GetBackfaceProperty();
  if (!actorBackfaceProperties)
  {
    vtkNew<vtkProperty> newActorBackfaceProperties;
    actor->SetBackfaceProperty(newActorBackfaceProperties);
    actorBackfaceProperties = newActorBackfaceProperties;
  }
  actorBackfaceProperties->DeepCopy(actorProperties);

  double offsetHsv[3];
  modelDisplayNode->GetBackfaceColorHSVOffset(offsetHsv);

  double colorHsv[3];
  vtkMath::RGBToHSV(actorProperties->GetColor(), colorHsv);
  double colorRgb[3];
  colorHsv[0] += offsetHsv[0];
  // wrap around hue value
  if (colorHsv[0] < 0.0)
  {
    colorHsv[0] += 1.0;
  }
  else if (colorHsv[0] > 1.0)
  {
    colorHsv[0] -= 1.0;
  }
  colorHsv[1] = vtkMath::ClampValue<double>(colorHsv[1] + offsetHsv[1], 0, 1);
  colorHsv[2] = vtkMath::ClampValue<double>(colorHsv[2] + offsetHsv[2], 0, 1);
  vtkMath::HSVToRGB(colorHsv, colorRgb);
  actorBackfaceProperties->SetColor(colorRgb);
}

void vtkMRMLModelDMPipeline::UpdateCapActorProperties(double opacity, bool visibility) const
{
  if (!this->CapActor)
  {
    return;
  }

  this->CapActor->SetVisibility(visibility);
  if (!visibility)
  {
    return;
  }

  vtkMRMLModelDisplayNode* modelDisplayNode = this->GetModelDisplayNode();
  if (!modelDisplayNode)
  {
    return;
  }

  this->UpdateActorProperties(this->CapActor, opacity, visibility);

  vtkProperty* capActorProperties = this->CapActor->GetProperty();
  capActorProperties->SetLineWidth(static_cast<float>(modelDisplayNode->GetLineWidth()));

  vtkMapper* capMapper = this->CapActor->GetMapper();
  if (!capMapper)
  {
    return;
  }

  double offsetHsv[3];
  modelDisplayNode->GetClippingCapColorHSVOffset(offsetHsv);

  double colorHsv[3];
  vtkMath::RGBToHSV(capActorProperties->GetColor(), colorHsv);
  colorHsv[0] += offsetHsv[0];
  // wrap around hue value
  if (colorHsv[0] < 0.0)
  {
    colorHsv[0] += 1.0;
  }
  else if (colorHsv[0] > 1.0)
  {
    colorHsv[0] -= 1.0;
  }
  colorHsv[1] = vtkMath::ClampValue<double>(colorHsv[1] + offsetHsv[1], 0.0, 1.0);
  colorHsv[2] = vtkMath::ClampValue<double>(colorHsv[2] + offsetHsv[2], 0.0, 1.0);

  double colorRgb[3];
  vtkMath::HSVToRGB(colorHsv, colorRgb);
  capActorProperties->SetColor(colorRgb);

  bool capSurface = modelDisplayNode->GetClippingCapSurface();
  bool clipOutline = modelDisplayNode->GetClippingOutline();

  double capOpacity = capSurface ? opacity * modelDisplayNode->GetClippingCapOpacity() : 0.0;
  double outlineOpacity = clipOutline ? opacity : 0.0;

  double edgeColor[3];
  modelDisplayNode->GetEdgeColor(edgeColor);
  double edgeColorWithAlpha[4] = { edgeColor[0], edgeColor[1], edgeColor[2], outlineOpacity };

  // Create a lookup table to map cell data to colors.
  vtkNew<vtkLookupTable> lut;
  lut->SetTableRange(VTK_LINE, VTK_POLY_LINE);
  lut->SetNumberOfColors(1);
  lut->Build();
  lut->SetTableValue(0, edgeColorWithAlpha);
  lut->UseBelowRangeColorOn();
  lut->UseAboveRangeColorOn();
  lut->SetBelowRangeColor(colorRgb[0], colorRgb[1], colorRgb[2], capOpacity);
  lut->SetAboveRangeColor(colorRgb[0], colorRgb[1], colorRgb[2], capOpacity);

  capMapper->SetLookupTable(lut);
  capMapper->UseLookupTableScalarRangeOn();
  capMapper->SetScalarModeToUseCellData();
  capMapper->SetColorModeToMapScalars();
  capMapper->SetScalarVisibility(true);
}
