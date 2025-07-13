/*==============================================================================

Program: 3D Slicer

Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "vtkSegmentEditorLogic.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSegmentEditorNode.h"

// vtkSegmentationCore Includes
#include "vtkSegmentation.h"
#include "vtkSegmentationHistory.h"
#include "vtkSegment.h"
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkBinaryLabelmapToClosedSurfaceConversionRule.h"

// Segment editor effects includes
#include "vtkSegmentEditorAbstractEffect.h"

// VTK includes
#include <vtkAddonMathUtilities.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCallbackCommand.h>
#include <vtkCollection.h>
#include <vtkGeneralTransform.h>
#include <vtkImageThreshold.h>
#include <vtkImageExtractComponents.h>
#include <vtkInteractorObserver.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

// Slicer includes
#include <vtkMRMLSliceLogic.h>

// MRML includes
#include <vtkMRMLCameraDisplayableManager.h>
#include <vtkMRMLCameraWidget.h>
#include <vtkMRMLCrosshairDisplayableManager.h>
#include <vtkMRMLLabelMapVolumeNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceIntersectionWidget.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLViewNode.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLSliceCompositeNode.h>

// Terminologies includes
#include <vtkSlicerTerminologyEntry.h>
#include <vtkSlicerTerminologiesModuleLogic.h>

#include <vtkObjectFactory.h>

#include "vtkMRMLDisplayableManagerGroup.h"

vtkStandardNewMacro(vtkSegmentEditorLogic);

static const int BINARY_LABELMAP_SCALAR_TYPE = VTK_UNSIGNED_CHAR;
// static const unsigned char BINARY_LABELMAP_VOXEL_FULL = 1; // unused
static const unsigned char BINARY_LABELMAP_VOXEL_EMPTY = 0;

static const char NULL_EFFECT_NAME[] = "NULL";

//---------------------------------------------------------------------------
class vtkSegmentEditorEventCallbackCommand : public vtkCallbackCommand
{
public:
  static vtkSegmentEditorEventCallbackCommand* New() { return new vtkSegmentEditorEventCallbackCommand; }
  /// Segment editor widget observing the event
  vtkWeakPointer<vtkSegmentEditorLogic> EditorWidget;
  /// Slice widget or 3D widget
  vtkWeakPointer<vtkMRMLAbstractViewNode> ViewNode;
  vtkWeakPointer<vtkRenderWindow> ViewRenderWindow;
};

//-----------------------------------------------------------------------------
struct SegmentEditorEventObservation
{
  vtkSmartPointer<vtkSegmentEditorEventCallbackCommand> CallbackCommand = nullptr;
  vtkWeakPointer<vtkObject> ObservedObject = nullptr;
  std::vector<int> ObservationTags;

  bool isValid() const { return static_cast<bool>(CallbackCommand); }
};

//-----------------------------------------------------------------------------
vtkSegmentEditorLogic::vtkSegmentEditorLogic()
  : Locked(false)
  , ActiveEffect(nullptr)
  , ViewsObserved(false)
  , AutoShowSourceVolumeNode(true)
  , AlignedSourceVolume(nullptr)
  , ModifierLabelmap(nullptr)
  , SelectedSegmentLabelmap(nullptr)
  , MaskLabelmap(nullptr)
  , ReferenceGeometryImage(nullptr)
  , AlignedSourceVolumeUpdateSourceVolumeNode(nullptr)
  , AlignedSourceVolumeUpdateSourceVolumeNodeTransform(nullptr)
  , AlignedSourceVolumeUpdateSegmentationNodeTransform(nullptr)
  , MRMLScene(nullptr)
  , InteractionNodeObs(0)
{
  this->AlignedSourceVolume = vtkOrientedImageData::New();
  this->ModifierLabelmap = vtkOrientedImageData::New();
  this->MaskLabelmap = vtkOrientedImageData::New();
  this->SelectedSegmentLabelmap = vtkOrientedImageData::New();
  this->ReferenceGeometryImage = vtkOrientedImageData::New();
  this->SegmentationHistory = vtkSmartPointer<vtkSegmentationHistory>::New();
}

//-----------------------------------------------------------------------------
vtkSegmentEditorLogic::~vtkSegmentEditorLogic()
{
  removeViewObservations();

  if (this->AlignedSourceVolume)
  {
    this->AlignedSourceVolume->Delete();
    this->AlignedSourceVolume = nullptr;
  }
  if (this->ModifierLabelmap)
  {
    this->ModifierLabelmap->Delete();
    this->ModifierLabelmap = nullptr;
  }
  if (this->MaskLabelmap)
  {
    this->MaskLabelmap->Delete();
    this->MaskLabelmap = nullptr;
  }
  if (this->SelectedSegmentLabelmap)
  {
    this->SelectedSegmentLabelmap->Delete();
    this->SelectedSegmentLabelmap = nullptr;
  }
  if (this->ReferenceGeometryImage)
  {
    this->ReferenceGeometryImage->Delete();
    this->ReferenceGeometryImage = nullptr;
  }
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::resetModifierLabelmapToDefault()
{
  std::string referenceImageGeometry = this->referenceImageGeometry();
  if (referenceImageGeometry.empty())
  {
    vtkErrorMacro(": Cannot determine default modifierLabelmap geometry");
    return false;
  }

  std::string modifierLabelmapReferenceImageGeometryBaseline = vtkSegmentationConverter::SerializeImageGeometry(this->ModifierLabelmap);

  // Set reference geometry to labelmap (origin, spacing, directions, extents) and allocate scalars
  vtkNew<vtkMatrix4x4> referenceGeometryMatrix;
  int referenceExtent[6] = { 0, -1, 0, -1, 0, -1 };
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, referenceGeometryMatrix.GetPointer(), referenceExtent);
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, this->ModifierLabelmap, true, BINARY_LABELMAP_SCALAR_TYPE, 1);

  vtkOrientedImageDataResample::FillImage(this->ModifierLabelmap, BINARY_LABELMAP_VOXEL_EMPTY);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::updateSelectedSegmentLabelmap()
{
  if (!this->ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    return false;
  }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  std::string referenceImageGeometry = this->referenceImageGeometry();
  if (!segmentationNode || referenceImageGeometry.empty())
  {
    return false;
  }
  const char* selectedSegmentID = this->ParameterSetNode->GetSelectedSegmentID();
  if (!selectedSegmentID)
  {
    vtkErrorMacro(": Invalid segment selection");
    return false;
  }

  vtkSegment* selectedSegment = segmentationNode->GetSegmentation()->GetSegment(selectedSegmentID);
  if (selectedSegment == nullptr)
  {
    vtkWarningMacro(" failed: Segment " << selectedSegmentID << " not found in segmentation");
    return false;
  }
  vtkOrientedImageData* segmentLabelmap =
    vtkOrientedImageData::SafeDownCast(selectedSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()));
  if (!segmentLabelmap)
  {
    vtkErrorMacro(": Failed to get binary labelmap representation in segmentation " << segmentationNode->GetName());
    return false;
  }
  int* extent = segmentLabelmap->GetExtent();
  if (extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5])
  {
    vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, this->SelectedSegmentLabelmap, false);
    this->SelectedSegmentLabelmap->SetExtent(0, -1, 0, -1, 0, -1);
    this->SelectedSegmentLabelmap->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    return true;
  }

  vtkNew<vtkImageThreshold> threshold;
  threshold->SetInputData(segmentLabelmap);
  threshold->ThresholdBetween(selectedSegment->GetLabelValue(), selectedSegment->GetLabelValue());
  threshold->SetInValue(1);
  threshold->SetOutValue(0);
  threshold->Update();

  vtkSmartPointer<vtkOrientedImageData> thresholdedSegmentLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();
  thresholdedSegmentLabelmap->ShallowCopy(threshold->GetOutput());
  thresholdedSegmentLabelmap->CopyDirections(segmentLabelmap);

  vtkNew<vtkOrientedImageData> referenceImage;
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, referenceImage.GetPointer(), false);
  vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
    thresholdedSegmentLabelmap, referenceImage.GetPointer(), this->SelectedSegmentLabelmap, /*linearInterpolation=*/false);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::updateAlignedSourceVolume()
{
  if (!this->ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    return false;
  }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  vtkMRMLScalarVolumeNode* sourceVolumeNode = this->ParameterSetNode->GetSourceVolumeNode();
  std::string referenceImageGeometry = this->referenceImageGeometry();
  if (!segmentationNode || !sourceVolumeNode || !sourceVolumeNode->GetImageData() //
      || !sourceVolumeNode->GetImageData()->GetPointData() || referenceImageGeometry.empty())
  {
    return false;
  }

  vtkNew<vtkOrientedImageData> referenceImage;
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, referenceImage.GetPointer(), false);

  int* referenceImageExtent = referenceImage->GetExtent();
  int* alignedSourceVolumeExtent = this->AlignedSourceVolume->GetExtent();
  // If source volume node and transform nodes did not change and the aligned source volume covers the entire reference geometry
  // then we don't need to update the aligned source volume.
  if (vtkOrientedImageDataResample::DoGeometriesMatch(referenceImage.GetPointer(), this->AlignedSourceVolume)               //
      && alignedSourceVolumeExtent[0] <= referenceImageExtent[0] && alignedSourceVolumeExtent[1] >= referenceImageExtent[1] //
      && alignedSourceVolumeExtent[2] <= referenceImageExtent[2] && alignedSourceVolumeExtent[3] >= referenceImageExtent[3] //
      && alignedSourceVolumeExtent[4] <= referenceImageExtent[4] && alignedSourceVolumeExtent[5] >= referenceImageExtent[5] //
      && vtkOrientedImageDataResample::DoExtentsMatch(referenceImage.GetPointer(), this->AlignedSourceVolume)               //
      && this->AlignedSourceVolumeUpdateSourceVolumeNode == sourceVolumeNode                                                //
      && this->AlignedSourceVolumeUpdateSourceVolumeNodeTransform == sourceVolumeNode->GetParentTransformNode()             //
      && this->AlignedSourceVolumeUpdateSegmentationNodeTransform == segmentationNode->GetParentTransformNode())
  {
    // Extents and nodes are matching, check if they have not been modified since the aligned master
    // volume generation.
    bool updateAlignedSourceVolumeRequired = false;
    if (sourceVolumeNode->GetMTime() > this->AlignedSourceVolume->GetMTime())
    {
      updateAlignedSourceVolumeRequired = true;
    }
    else if (sourceVolumeNode->GetParentTransformNode() && sourceVolumeNode->GetParentTransformNode()->GetMTime() > this->AlignedSourceVolume->GetMTime())
    {
      updateAlignedSourceVolumeRequired = true;
    }
    else if (segmentationNode->GetParentTransformNode() && segmentationNode->GetParentTransformNode()->GetMTime() > this->AlignedSourceVolume->GetMTime())
    {
      updateAlignedSourceVolumeRequired = true;
    }
    if (!updateAlignedSourceVolumeRequired)
    {
      return true;
    }
  }

  // Get a read-only version of sourceVolume as a vtkOrientedImageData
  vtkNew<vtkOrientedImageData> sourceVolume;

  if (sourceVolumeNode->GetImageData()->GetNumberOfScalarComponents() == 1)
  {
    sourceVolume->vtkImageData::ShallowCopy(sourceVolumeNode->GetImageData());
  }
  else
  {
    vtkNew<vtkImageExtractComponents> extract;
    extract->SetInputData(sourceVolumeNode->GetImageData());
    extract->Update();
    extract->SetComponents(0); // TODO: allow user to configure this
    sourceVolume->vtkImageData::ShallowCopy(extract->GetOutput());
  }
  vtkSmartPointer<vtkMatrix4x4> ijkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  sourceVolumeNode->GetIJKToRASMatrix(ijkToRasMatrix);
  sourceVolume->SetGeometryFromImageToWorldMatrix(ijkToRasMatrix);

  vtkNew<vtkGeneralTransform> sourceVolumeToSegmentationTransform;
  vtkMRMLTransformNode::GetTransformBetweenNodes(
    sourceVolumeNode->GetParentTransformNode(), segmentationNode->GetParentTransformNode(), sourceVolumeToSegmentationTransform.GetPointer());

  double backgroundValue = sourceVolumeNode->GetImageBackgroundScalarComponentAsDouble(0);
  vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(sourceVolume,
                                                                              referenceImage,
                                                                              this->AlignedSourceVolume,
                                                                              /*linearInterpolation=*/true,
                                                                              /*padImage=*/false,
                                                                              sourceVolumeToSegmentationTransform,
                                                                              backgroundValue);

  this->AlignedSourceVolumeUpdateSourceVolumeNode = sourceVolumeNode;
  this->AlignedSourceVolumeUpdateSourceVolumeNodeTransform = sourceVolumeNode->GetParentTransformNode();
  this->AlignedSourceVolumeUpdateSegmentationNodeTransform = segmentationNode->GetParentTransformNode();

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::updateMaskLabelmap()
{
  if (!this->ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    return false;
  }
  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    vtkErrorMacro(": Invalid segmentation node");
    return false;
  }

  std::string referenceGeometryStr = this->referenceImageGeometry();
  if (referenceGeometryStr.empty())
  {
    vtkErrorMacro(": Cannot determine mask labelmap geometry");
    return false;
  }
  vtkNew<vtkOrientedImageData> referenceGeometry;
  if (!vtkSegmentationConverter::DeserializeImageGeometry(referenceGeometryStr, referenceGeometry, false))
  {
    vtkErrorMacro(": Cannot determine mask labelmap geometry");
    return false;
  }

  // GenerateEditMask can add intensity range based mask, too. We do not use it here, as currently
  // editable intensity range is taken into account in vtkSegmentEditorAbstractEffect::modifySelectedSegmentByLabelmap.
  // It would simplify implementation if we passed source volume and intensity range to GenerateEditMask here
  // and removed intensity range based masking from modifySelectedSegmentByLabelmap.
  if (!segmentationNode->GenerateEditMask(this->MaskLabelmap,
                                          this->ParameterSetNode->GetMaskMode(),
                                          referenceGeometry,
                                          this->ParameterSetNode->GetSelectedSegmentID() ? this->ParameterSetNode->GetSelectedSegmentID() : "",
                                          this->ParameterSetNode->GetMaskSegmentID() ? this->ParameterSetNode->GetMaskSegmentID() : ""))
  {
    vtkErrorMacro(": Mask generation failed");
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::updateReferenceGeometryImage()
{
  std::string geometry = this->referenceImageGeometry();
  if (geometry.empty())
  {
    return false;
  }
  return vtkSegmentationConverter::DeserializeImageGeometry(geometry, this->ReferenceGeometryImage, false /* don't allocate scalars */);
}

//-----------------------------------------------------------------------------
std::string vtkSegmentEditorLogic::getReferenceImageGeometryFromSegmentation(vtkSegmentation* segmentation)
{
  if (!segmentation)
  {
    return "";
  }

  // If "reference image geometry" conversion parameter is set then use that
  std::string referenceImageGeometry = segmentation->GetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName());
  if (!referenceImageGeometry.empty())
  {
    // Extend reference image geometry to contain all segments (needed for example for properly handling imported segments
    // that do not fit into the reference image geometry)
    vtkSmartPointer<vtkOrientedImageData> commonGeometryImage = vtkSmartPointer<vtkOrientedImageData>::New();
    vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, commonGeometryImage, false);
    // Determine extent that contains all segments
    int commonSegmentExtent[6] = { 0, -1, 0, -1, 0, -1 };
    segmentation->DetermineCommonLabelmapExtent(commonSegmentExtent, commonGeometryImage);
    if (commonSegmentExtent[0] <= commonSegmentExtent[1]    //
        && commonSegmentExtent[2] <= commonSegmentExtent[3] //
        && commonSegmentExtent[4] <= commonSegmentExtent[5])
    {
      // Expand commonGeometryExtent as needed to contain commonSegmentExtent
      int commonGeometryExtent[6] = { 0, -1, 0, -1, 0, -1 };
      commonGeometryImage->GetExtent(commonGeometryExtent);
      for (int i = 0; i < 3; i++)
      {
        commonGeometryExtent[i * 2] = std::min(commonSegmentExtent[i * 2], commonGeometryExtent[i * 2]);
        commonGeometryExtent[i * 2 + 1] = std::max(commonSegmentExtent[i * 2 + 1], commonGeometryExtent[i * 2 + 1]);
      }
      commonGeometryImage->SetExtent(commonGeometryExtent);
      referenceImageGeometry = vtkSegmentationConverter::SerializeImageGeometry(commonGeometryImage);
    }

    // TODO: Use oversampling (if it's 'A' then ignore and changed to 1)
    return referenceImageGeometry;
  }
  if (segmentation->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    // If no reference image geometry is specified but there are labels already then determine geometry from that
    referenceImageGeometry = segmentation->DetermineCommonLabelmapGeometry();
    return referenceImageGeometry;
  }
  return "";
}

//-----------------------------------------------------------------------------
std::string vtkSegmentEditorLogic::referenceImageGeometry()
{
  if (!this->ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    notifyEffectsOfReferenceGeometryChange("");
    return "";
  }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  vtkSegmentation* segmentation = segmentationNode ? segmentationNode->GetSegmentation() : nullptr;
  if (!segmentationNode || !segmentation)
  {
    vtkErrorMacro(": Invalid segmentation");
    notifyEffectsOfReferenceGeometryChange("");
    return "";
  }

  std::string referenceImageGeometry;
  referenceImageGeometry = this->getReferenceImageGeometryFromSegmentation(segmentation);
  if (referenceImageGeometry.empty())
  {
    // If no reference image geometry could be determined then use the source volume's geometry
    vtkMRMLScalarVolumeNode* sourceVolumeNode = this->ParameterSetNode->GetSourceVolumeNode();
    if (!sourceVolumeNode)
    {
      // cannot determine reference geometry
      return "";
    }
    // Update the referenceImageGeometry parameter for next time
    segmentationNode->SetReferenceImageGeometryParameterFromVolumeNode(sourceVolumeNode);
    // Update extents to include all existing segments
    referenceImageGeometry = this->getReferenceImageGeometryFromSegmentation(segmentation);
  }
  notifyEffectsOfReferenceGeometryChange(referenceImageGeometry);
  return referenceImageGeometry;
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::segmentationDisplayableInView(vtkMRMLAbstractViewNode* viewNode)
{
  if (!viewNode)
  {
    return false;
  }
  if (!this->ParameterSetNode)
  {
    return false;
  }
  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    return false;
  }
  const char* viewNodeID = viewNode->GetID();
  int numberOfDisplayNodes = segmentationNode->GetNumberOfDisplayNodes();
  for (int displayNodeIndex = 0; displayNodeIndex < numberOfDisplayNodes; displayNodeIndex++)
  {
    vtkMRMLDisplayNode* segmentationDisplayNode = segmentationNode->GetNthDisplayNode(displayNodeIndex);
    if (segmentationDisplayNode && segmentationDisplayNode->IsDisplayableInView(viewNodeID))
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::getSegmentationIJKToRAS(vtkMatrix4x4* ijkToRas)
{
  if (!this->ParameterSetNode)
  {
    return false;
  }
  if (!this->updateReferenceGeometryImage())
  {
    return false;
  }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode || !segmentationNode->GetSegmentation())
  {
    return false;
  }
  if (!segmentationNode->GetSegmentation()->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    return false;
  }
  this->ReferenceGeometryImage->GetImageToWorldMatrix(ijkToRas);
  vtkMRMLTransformNode* transformNode = segmentationNode->GetParentTransformNode();
  if (transformNode)
  {
    if (!transformNode->IsTransformToWorldLinear())
    {
      return false;
    }
    vtkSmartPointer<vtkMatrix4x4> volumeRasToWorldRas = vtkSmartPointer<vtkMatrix4x4>::New();
    transformNode->GetMatrixTransformToWorld(volumeRasToWorldRas);
    vtkMatrix4x4::Multiply4x4(volumeRasToWorldRas, ijkToRas, ijkToRas);
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::canTriviallyConvertSourceRepresentationToBinaryLabelMap()
{
  if (!isSegmentationNodeValid())
  {
    return false;
  }

  if (SegmentationNode->GetSegmentation()->GetNumberOfSegments() < 1)
  {
    return true;
  }

  if (SegmentationNode->GetSegmentation()->GetSourceRepresentationName() == vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName())
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::setSourceRepresentationToBinaryLabelmap()
{
  if (!isSegmentationNodeValid())
  {
    return false;
  }

  MRMLNodeModifyBlocker blocker(SegmentationNode);

  if (canTriviallyConvertSourceRepresentationToBinaryLabelMap())
  {
    SegmentationNode->GetSegmentation()->SetSourceRepresentationName(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
    return true;
  }

  // All other representations are invalidated when changing to binary labelmap.
  // Re-creating closed surface if it was present before, so that changes can be seen.
  bool closedSurfacePresent = SegmentationNode->GetSegmentation()->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());

  bool createBinaryLabelmapRepresentationSuccess =
    SegmentationNode->GetSegmentation()->CreateRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());

  if (!createBinaryLabelmapRepresentationSuccess)
  {
    vtkErrorMacro("Failed to create binary labelmap representation in segmentation " + std::string(SegmentationNode->GetName())
                  + " for editing!\nPlease see Segmentations module for details.");
    return false;
  }

  SegmentationNode->GetSegmentation()->SetSourceRepresentationName(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());

  if (closedSurfacePresent)
  {
    SegmentationNode->GetSegmentation()->CreateRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
  }

  // Show binary labelmap in 2D
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(SegmentationNode->GetDisplayNode());
  if (displayNode)
  {
    displayNode->SetPreferredDisplayRepresentationName2D(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
  }
  return true;
}

//-----------------------------------------------------------------------------
vtkSegmentEditorAbstractEffect* vtkSegmentEditorLogic::activeEffect() const
{
  return ActiveEffect;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::setActiveEffect(vtkSegmentEditorAbstractEffect* effect)
{
  if (ActiveEffect == effect)
  {
    return;
  }

  if (!ParameterSetNode)
  {
    if (effect != nullptr)
    {
      vtkErrorMacro("Invalid segment editor parameter set node");
    }
    return;
  }

  ActiveEffect = effect;
  ParameterSetNode->SetActiveEffectName(effect ? effect->name().c_str() : "");
}

//-----------------------------------------------------------------------------
vtkMRMLScene* vtkSegmentEditorLogic::mrmlScene() const
{
  return MRMLScene;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::setMRMLScene(vtkMRMLScene* newScene)
{
  if (newScene == this->mrmlScene())
  {
    return;
  }

  MRMLScene = newScene;

  vtkMRMLInteractionNode* interactionNode = nullptr;
  if (newScene)
  {
    interactionNode = vtkMRMLInteractionNode::SafeDownCast(newScene->GetNodeByID("vtkMRMLInteractionNodeSingleton"));
  }
  setInteractionNode(interactionNode);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::setMRMLSegmentEditorNode(vtkMRMLSegmentEditorNode* newSegmentEditorNode)
{
  if (ParameterSetNode == newSegmentEditorNode)
  {
    return;
  }
  ParameterSetNode = newSegmentEditorNode;
  InvokeEvent(SegmentationNodeChangedEvent);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onInteractionNodeModified(vtkObject* caller, unsigned long eid, void* clientData, void* callData)
{
  auto self = reinterpret_cast<vtkSegmentEditorLogic*>(clientData);
  if (!self)
    return;
  self->onInteractionNodeModified();
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onMRMLSceneEndCloseEvent()
{
  this->initializeParameterSetNode();
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onInteractionNodeModified()
{
  if (!InteractionNode || !ActiveEffect)
  {
    return;
  }
  // Only notify the active effect about interaction node changes
  // (inactive effects should not interact with the user)
  ActiveEffect->interactionNodeModified(InteractionNode);
}

//------------------------------------------------------------------------------
vtkMRMLSegmentEditorNode* vtkSegmentEditorLogic::mrmlSegmentEditorNode() const
{
  return ParameterSetNode;
}

//-----------------------------------------------------------------------------
vtkMRMLInteractionNode* vtkSegmentEditorLogic::interactionNode() const
{
  return InteractionNode;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::setInteractionNode(vtkMRMLInteractionNode* interactionNode)
{
  if (InteractionNode == interactionNode)
  {
    return;
  }
  if (InteractionNode)
  {
    InteractionNode->RemoveObserver(InteractionNodeObs);
  }
  InteractionNode = interactionNode;

  if (InteractionNode)
  {
    auto callback = vtkSmartPointer<vtkCallbackCommand>::New();
    callback->SetClientData(this);
    callback->SetCallback(&vtkSegmentEditorLogic::onInteractionNodeModified);
    InteractionNodeObs = InteractionNode->AddObserver(vtkCommand::ModifiedEvent, callback);
  }
}

//------------------------------------------------------------------------------
void vtkSegmentEditorLogic::setSegmentationNode(vtkMRMLNode* node)
{
  if (!ParameterSetNode)
  {
    if (node)
    {
      vtkErrorMacro(" failed: need to set segment editor node first");
    }
    return;
  }
  if (ParameterSetNode->GetSegmentationNode() == node)
  {
    // no change
    return;
  }

  this->setActiveEffect(nullptr); // deactivate current effect when we switch to a different segmentation
  ParameterSetNode->SetAndObserveSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
}

//-----------------------------------------------------------------------------
vtkMRMLNode* vtkSegmentEditorLogic::segmentationNode() const
{
  return SegmentationNode;
}

//------------------------------------------------------------------------------
void vtkSegmentEditorLogic::setSegmentationNodeID(const std::string& nodeID)
{
  if (!this->mrmlScene())
  {
    vtkErrorMacro(" failed: MRML scene is not set");
    return;
  }
  this->setSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(this->mrmlScene()->GetNodeByID(nodeID.c_str())));
}

//------------------------------------------------------------------------------
std::string vtkSegmentEditorLogic::segmentationNodeID() const
{
  return SegmentationNode ? SegmentationNode->GetID() : "";
}

//-----------------------------------------------------------------------------
std::string vtkSegmentEditorLogic::currentSegmentID() const
{
  if (!ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    return {};
  }

  return ParameterSetNode->GetSelectedSegmentID();
}

//------------------------------------------------------------------------------
void vtkSegmentEditorLogic::setSourceVolumeNode(vtkMRMLNode* node)
{
  if (!ParameterSetNode || !this->segmentationNode())
  {
    if (node)
    {
      vtkErrorMacro(" failed: need to set segment editor and segmentation nodes first");
    }
    return;
  }

  // Set source volume to parameter set node
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (ParameterSetNode->GetSourceVolumeNode() == volumeNode)
  {
    return;
  }

  ParameterSetNode->SetAndObserveSourceVolumeNode(volumeNode);
}

//-----------------------------------------------------------------------------
vtkMRMLNode* vtkSegmentEditorLogic::sourceVolumeNode() const
{
  if (!ParameterSetNode)
  {
    return nullptr;
  }
  return ParameterSetNode->GetSourceVolumeNode();
}

//------------------------------------------------------------------------------
void vtkSegmentEditorLogic::setSourceVolumeNodeID(const std::string& nodeID)
{
  if (!this->mrmlScene())
  {
    vtkErrorMacro(" failed: MRML scene is not set");
    return;
  }
  this->setSourceVolumeNode(this->mrmlScene()->GetNodeByID(nodeID.c_str()));
}

//------------------------------------------------------------------------------
std::string vtkSegmentEditorLogic::sourceVolumeNodeID() const
{
  vtkMRMLNode* sourceVolumeNode = this->sourceVolumeNode();
  if (!sourceVolumeNode || !sourceVolumeNode->GetID())
  {
    return "";
  }
  return sourceVolumeNode->GetID();
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onSegmentationNodeChanged(vtkMRMLNode* node)
{
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
  {
    return;
  }
  this->setSegmentationNode(node);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::setCurrentSegmentID(const std::string segmentID)
{
  if (ParameterSetNode)
  {
    ParameterSetNode->SetSelectedSegmentID(segmentID.c_str());
  }
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onSourceVolumeNodeChanged(vtkMRMLNode* node)
{
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
  {
    return;
  }
  this->setSourceVolumeNode(node);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onAddSegment(std::string emptySegmentName, int firstVisibleStatus)
{
  if (!ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    return;
  }

  SegmentationHistory->SaveState();

  // Create empty segment in current segmentation
  std::string addedSegmentID = segmentationNode->GetSegmentation()->AddEmptySegment(emptySegmentName.c_str());

  // Set default terminology entry from application settings
  vtkSegment* addedSegment = segmentationNode->GetSegmentation()->GetSegment(addedSegmentID);
  if (addedSegment)
  {
    std::string defaultTerminologyEntryStr;
    // Default terminology in the segmentation node has the highest priority
    vtkNew<vtkSlicerTerminologyEntry> entry;
    if (vtkSlicerTerminologiesModuleLogic::GetDefaultTerminologyEntry(segmentationNode, entry) && !entry->IsEmpty())
    {
      defaultTerminologyEntryStr = vtkSlicerTerminologiesModuleLogic::SerializeTerminologyEntry(entry);
    }
    if (defaultTerminologyEntryStr.empty())
    {
      defaultTerminologyEntryStr = DefaultTerminologyEntry;
    }

    if (!defaultTerminologyEntryStr.empty())
    {
      addedSegment->SetTerminology(defaultTerminologyEntryStr);
    }
  }

  // Set segment status to one that is visible by current filtering criteria
  vtkSlicerSegmentationsModuleLogic::SetSegmentStatus(addedSegment, firstVisibleStatus);

  // Select the new segment
  if (!addedSegmentID.empty())
  {
    ParameterSetNode->SetSelectedSegmentID(addedSegmentID.c_str());
  }

  // Assign the new segment the terminology of the (now second) last segment
  if (segmentationNode->GetSegmentation()->GetNumberOfSegments() > 1)
  {
    vtkSegment* secondLastSegment = segmentationNode->GetSegmentation()->GetNthSegment(segmentationNode->GetSegmentation()->GetNumberOfSegments() - 2);
    std::string repeatedTerminologyEntry = secondLastSegment->GetTerminology();
    segmentationNode->GetSegmentation()->GetSegment(addedSegmentID)->SetTerminology(repeatedTerminologyEntry);
  }
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::hasSelectedSegmentID(int offset)
{
  return !(getSelectedSegmentID(offset).empty());
}

//-----------------------------------------------------------------------------
std::vector<std::string> vtkSegmentEditorLogic::getSegmentIDs()
{
  vtkMRMLSegmentationNode* segmentationNode = ParameterSetNode ? ParameterSetNode->GetSegmentationNode() : nullptr;
  vtkSegmentation* segmentation = segmentationNode ? segmentationNode->GetSegmentation() : nullptr;
  if (!segmentation)
  {
    return {};
  }

  std::vector<std::string> segmentIDs;
  segmentation->GetSegmentIDs(segmentIDs);
  return segmentIDs;
}

//-----------------------------------------------------------------------------
std::string vtkSegmentEditorLogic::getSelectedSegmentID(int offset)
{
  std::vector<std::string> segmentIDs = getSegmentIDs();

  // Find the currently selected segment iterator. If no valid selection, return empty ID
  auto currentIt = std::find(std::begin(segmentIDs), std::end(segmentIDs), currentSegmentID());
  if (currentIt == std::end(segmentIDs))
  {
    return {};
  }

  // Compute the offset iterator and check if it's valid. If not valid return empty ID.
  auto offsetIt = currentIt + offset;
  if (offsetIt < segmentIDs.begin() || offsetIt >= segmentIDs.end())
  {
    return {};
  }

  // Return the offset it
  return *offsetIt;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onRemoveSegment()
{
  if (!ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = ParameterSetNode->GetSegmentationNode();
  std::string selectedSegmentID = ParameterSetNode->GetSelectedSegmentID();
  if (!segmentationNode || selectedSegmentID.empty())
  {
    return;
  }

  SegmentationHistory->SaveState();

  // Switch to a new valid segment now (to avoid transient state when no segments are selected
  // as it could deactivate current effect).
  std::string newId = getSelectedSegmentID(1);
  if (newId.empty())
  {
    newId = getSelectedSegmentID(-1);
  }
  if (!newId.empty())
  {
    ParameterSetNode->SetSelectedSegmentID(newId.c_str());
  }

  // Remove segment
  segmentationNode->GetSegmentation()->RemoveSegment(selectedSegmentID);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onCreateSurfaceToggled(bool on)
{
  if (!ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    return;
  }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (!displayNode)
  {
    return;
  }

  MRMLNodeModifyBlocker segmentationNodeBlocker(segmentationNode);
  MRMLNodeModifyBlocker displayNodeBlocker(displayNode);

  // If just have been checked, then create closed surface representation and show it
  if (on)
  {
    // Make sure closed surface representation exists
    if (segmentationNode->GetSegmentation()->CreateRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName()))
    {
      // Set closed surface as displayed poly data representation
      displayNode->SetPreferredDisplayRepresentationName3D(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
      // But keep binary labelmap for 2D
      bool binaryLabelmapPresent = segmentationNode->GetSegmentation()->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
      if (binaryLabelmapPresent)
      {
        displayNode->SetPreferredDisplayRepresentationName2D(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
      }
    }
  }
  // If unchecked, then remove representation (but only if it's not the source representation)
  else if (segmentationNode->GetSegmentation()->GetSourceRepresentationName() != vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName())
  {
    segmentationNode->GetSegmentation()->RemoveRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
  }
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::showSourceVolumeInSliceViewers(bool forceShowInBackground /*=false*/, bool fitSlice /*=false*/)
{
  auto sliceNodes = mrmlScene()->GetNodesByClass<vtkMRMLSliceNode>("vtkMRMLSliceNode");

  if (!ParameterSetNode->GetSourceVolumeNode())
  {
    return;
  }
  for (const auto& sliceNode : sliceNodes)
  {
    if (!sliceNode)
    {
      continue;
    }
    auto sliceLogic = vtkMRMLSliceLogic::SafeDownCast(sliceNode->GetLogic());
    if (!sliceLogic)
    {
      continue;
    }
    if (!segmentationDisplayableInView(sliceNode))
    {
      continue;
    }
    vtkMRMLSliceCompositeNode* sliceCompositeNode = sliceLogic->GetSliceCompositeNode();
    if (!sliceCompositeNode)
    {
      continue;
    }
    std::string backgroundVolumeID = (sliceCompositeNode->GetBackgroundVolumeID() ? sliceCompositeNode->GetBackgroundVolumeID() : "");
    std::string foregroundVolumeID = (sliceCompositeNode->GetForegroundVolumeID() ? sliceCompositeNode->GetForegroundVolumeID() : "");
    std::string sourceVolumeID = (ParameterSetNode->GetSourceVolumeNode()->GetID() ? ParameterSetNode->GetSourceVolumeNode()->GetID() : "");
    bool sourceVolumeAlreadyShown = (backgroundVolumeID == sourceVolumeID || foregroundVolumeID == sourceVolumeID);
    if (!sourceVolumeAlreadyShown || forceShowInBackground)
    {
      sliceCompositeNode->SetBackgroundVolumeID(ParameterSetNode->GetSourceVolumeNode()->GetID());
      sliceCompositeNode->SetForegroundVolumeID(nullptr);
      sliceCompositeNode->SetLabelVolumeID(nullptr);
    }
    if (fitSlice)
    {
      sliceLogic->FitSliceToBackground(true);
    }
  }
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkSegmentEditorEventCallbackCommand> vtkSegmentEditorLogic::createViewInteractionCallbackCommand(vtkRenderWindow* renderWindow, vtkMRMLAbstractViewNode* viewNode)
{
  auto interactionCallbackCommand = vtkSmartPointer<vtkSegmentEditorEventCallbackCommand>::New();
  interactionCallbackCommand->EditorWidget = this;
  interactionCallbackCommand->ViewNode = viewNode;
  interactionCallbackCommand->ViewRenderWindow = renderWindow;
  interactionCallbackCommand->SetClientData(reinterpret_cast<void*>(interactionCallbackCommand.GetPointer()));
  interactionCallbackCommand->SetCallback(vtkSegmentEditorLogic::processEvents);
  return interactionCallbackCommand;
}

//---------------------------------------------------------------------------
SegmentEditorEventObservation vtkSegmentEditorLogic::createInteractorEventObservation(const vtkSmartPointer<vtkSegmentEditorEventCallbackCommand>& interactionCallbackCommand)
{
  // Connect interactor events
  auto renderWindow = vtkRenderWindow::SafeDownCast(interactionCallbackCommand->ViewNode->GetRenderWindow());

  SegmentEditorEventObservation interactorObservation;
  if (!renderWindow)
  {
    vtkErrorMacro("View node's render window is nullptr : ViewNodeID = " + std::string(interactionCallbackCommand->ViewNode->GetID()));
    return interactorObservation;
  }

  vtkRenderWindowInteractor* interactor = renderWindow->GetInteractor();
  interactorObservation.CallbackCommand = interactionCallbackCommand;
  interactorObservation.ObservedObject = interactor;
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::LeftButtonPressEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::LeftButtonDoubleClickEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::RightButtonPressEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::RightButtonDoubleClickEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::MiddleButtonPressEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::MiddleButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::MiddleButtonDoubleClickEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::MouseMoveEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::MouseWheelLeftEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::MouseWheelRightEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::KeyPressEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::KeyReleaseEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::EnterEvent, interactorObservation.CallbackCommand, 1.0));
  interactorObservation.ObservationTags.emplace_back(interactor->AddObserver(vtkCommand::LeaveEvent, interactorObservation.CallbackCommand, 1.0));
  return interactorObservation;
}

//---------------------------------------------------------------------------
SegmentEditorEventObservation vtkSegmentEditorLogic::createViewNodeModifiedEventObservation(const vtkSmartPointer<vtkSegmentEditorEventCallbackCommand>& interactionCallbackCommand)
{
  SegmentEditorEventObservation viewNodeObservation;
  viewNodeObservation.CallbackCommand = interactionCallbackCommand;
  viewNodeObservation.ObservedObject = interactionCallbackCommand->ViewNode;
  viewNodeObservation.ObservationTags.emplace_back(interactionCallbackCommand->ViewNode->AddObserver(vtkCommand::ModifiedEvent, viewNodeObservation.CallbackCommand, 1.0));
  return viewNodeObservation;
}

//---------------------------------------------------------------------------
SegmentEditorEventObservation vtkSegmentEditorLogic::createSlicePoseModifiedEventObservation(
  const vtkSmartPointer<vtkSegmentEditorEventCallbackCommand>& interactionCallbackCommand)
{
  auto sliceNode = vtkMRMLSliceNode::SafeDownCast(interactionCallbackCommand->ViewNode);
  if (!sliceNode)
    return {};

  SegmentEditorEventObservation slicePoseObservation;
  slicePoseObservation.CallbackCommand = interactionCallbackCommand;
  slicePoseObservation.ObservedObject = sliceNode->GetSliceToRAS();
  slicePoseObservation.ObservationTags.emplace_back(sliceNode->GetSliceToRAS()->AddObserver(vtkCommand::ModifiedEvent, slicePoseObservation.CallbackCommand, 1.0));
  return slicePoseObservation;
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::setupViewObservations(const std::vector<vtkMRMLAbstractViewNode*>& viewNodes)
{
  for (const auto& viewNode : viewNodes)
  {
    if (!viewNode)
    {
      continue;
    }

    auto renderWindow = vtkRenderWindow::SafeDownCast(viewNode->GetRenderWindow());
    if (!renderWindow)
    {
      continue;
    }

    auto interactionCallback = createViewInteractionCallbackCommand(renderWindow, viewNode);
    std::vector<SegmentEditorEventObservation> observations{ createInteractorEventObservation(interactionCallback),
                                                             createViewNodeModifiedEventObservation(interactionCallback),
                                                             createSlicePoseModifiedEventObservation(interactionCallback) };

    for (const auto& observation : observations)
    {
      if (observation.isValid())
      {
        EventObservations.emplace_back(observation);
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::refreshObservedViewNodeIds()
{
  ObservedViewNodeIDs.clear();
  if (!SegmentationNode)
  {
    return;
  }

  int numberOfDisplayNodes = SegmentationNode->GetNumberOfDisplayNodes();
  for (int displayNodeIndex = 0; displayNodeIndex < numberOfDisplayNodes; displayNodeIndex++)
  {
    vtkMRMLDisplayNode* segmentationDisplayNode = SegmentationNode->GetNthDisplayNode(displayNodeIndex);
    if (!segmentationDisplayNode)
    {
      // this may occur during scene close
      continue;
    }
    ObservedViewNodeIDs[segmentationDisplayNode->GetID()] = segmentationDisplayNode->GetViewNodeIDs();
  }
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::setupViewObservations()
{
  // Make sure previous observations are cleared before setting up the new ones
  this->removeViewObservations();

  // Setup observations for slice and threed views
  setupViewObservations(mrmlScene()->GetNodesByClass<vtkMRMLAbstractViewNode>("vtkMRMLAbstractViewNode"));

  // Refresh observed view node ids
  refreshObservedViewNodeIds();

  ViewsObserved = true;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::removeViewObservations()
{
  for (const SegmentEditorEventObservation& eventObservation : EventObservations)
  {
    if (eventObservation.ObservedObject)
    {
      for (int observationTag : eventObservation.ObservationTags)
      {
        eventObservation.ObservedObject->RemoveObserver(observationTag);
      }
    }
  }
  EventObservations.clear();
  ViewsObserved = false;
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::saveStateForUndo()
{
  if (SegmentationHistory->GetMaximumNumberOfStates() > 0)
  {
    SegmentationHistory->SaveState();
  }
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::updateVolume(void* volumeToUpdate, bool& success)
{
  if (volumeToUpdate == AlignedSourceVolume)
  {
    success = updateAlignedSourceVolume();
  }
  else if (volumeToUpdate == ModifierLabelmap)
  {
    success = resetModifierLabelmapToDefault();
  }
  else if (volumeToUpdate == MaskLabelmap)
  {
    success = updateMaskLabelmap();
  }
  else if (volumeToUpdate == SelectedSegmentLabelmap)
  {
    success = updateSelectedSegmentLabelmap();
  }
  else if (volumeToUpdate == ReferenceGeometryImage)
  {
    success = updateReferenceGeometryImage();
  }
  else
  {
    vtkErrorMacro(": Failed to update volume");
    success = false;
  }
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::processEvents(vtkObject* caller, unsigned long eid, void* clientData, void* vtkNotUsed(callData))
{
  // Get and parse client data
  vtkSegmentEditorEventCallbackCommand* callbackCommand = reinterpret_cast<vtkSegmentEditorEventCallbackCommand*>(clientData);
  vtkSegmentEditorLogic* self = callbackCommand->EditorWidget;
  vtkRenderWindow* viewRenderWindow = callbackCommand->ViewRenderWindow;
  vtkMRMLAbstractViewNode* viewNode = callbackCommand->ViewNode;
  if (!self || !viewRenderWindow || !viewNode)
  {
    vtkErrorWithObjectMacro(nullptr, "Invalid event data");
    return;
  }

  // Do nothing if scene is closing
  if (!self->mrmlScene() || self->mrmlScene()->IsClosing())
  {
    return;
  }
  // If the segment editor node is no longer valid then ignore all events
  if (!self->mrmlSegmentEditorNode())
  {
    return;
  }

  vtkMatrix4x4* sliceToRAS = vtkMatrix4x4::SafeDownCast(caller);
  if (sliceToRAS)
  {
    self->InvokeEvent(UpdateSliceRotateWarningEvent);
    return;
  }

  // Get active effect
  vtkSegmentEditorAbstractEffect* activeEffect = self->activeEffect();
  if (!activeEffect)
  {
    return;
  }

  // Call processing function of active effect. Handle both interactor and view node events
  vtkRenderWindowInteractor* callerInteractor = vtkRenderWindowInteractor::SafeDownCast(caller);
  vtkMRMLAbstractViewNode* callerViewNode = vtkMRMLAbstractViewNode::SafeDownCast(caller);
  if (callerInteractor)
  {
    // Do not process events while a touch gesture is in progress (e.g., do not paint in the view
    // while doing multi-touch pinch/rotate).
    auto displayableManagerGroup = vtkMRMLDisplayableManagerGroup::SafeDownCast(viewNode->GetDisplayableManagerGroup());
    if (displayableManagerGroup)
    {
      vtkMRMLCrosshairDisplayableManager* crosshairDisplayableManager =
        vtkMRMLCrosshairDisplayableManager::SafeDownCast(displayableManagerGroup->GetDisplayableManagerByClassName("vtkMRMLCrosshairDisplayableManager"));
      if (crosshairDisplayableManager)
      {
        int widgetState = crosshairDisplayableManager->GetSliceIntersectionWidget()->GetWidgetState();
        if (widgetState == vtkMRMLSliceIntersectionWidget::WidgetStateTouchGesture)
        {
          return;
        }
      }

      vtkMRMLCameraDisplayableManager* cameraDisplayableManager =
        vtkMRMLCameraDisplayableManager::SafeDownCast(displayableManagerGroup->GetDisplayableManagerByClassName("vtkMRMLCameraDisplayableManager"));
      if (cameraDisplayableManager)
      {
        int widgetState = cameraDisplayableManager->GetCameraWidget()->GetWidgetState();
        if (widgetState == vtkMRMLCameraWidget::WidgetStateTouchGesture)
        {
          return;
        }
      }
    }

    bool abortEvent = activeEffect->processInteractionEvents(callerInteractor, eid, viewRenderWindow, viewNode);
    if (abortEvent)
    {
      /// Set the AbortFlag on the vtkCommand associated with the event.
      /// It causes other observers of the interactor not to receive the events.
      callbackCommand->SetAbortFlag(1);
    }
  }
  else if (callerViewNode)
  {
    activeEffect->processViewNodeEvents(callerViewNode, eid, viewRenderWindow);
  }
  else
  {
    vtkErrorWithObjectMacro(nullptr, ": Unsupported caller object");
  }
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onSourceVolumeIntensityMaskChecked(bool checked)
{
  if (!ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    return;
  }
  ParameterSetNode->SetSourceVolumeIntensityMask(checked);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onSourceVolumeIntensityMaskRangeChanged(double min, double max)
{
  if (!ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    return;
  }
  ParameterSetNode->SetSourceVolumeIntensityMaskRange(min, max);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::setUndoEnabled(bool enabled)
{
  if (enabled)
  {
    SegmentationHistory->RemoveAllStates();
  }
}

//-----------------------------------------------------------------------------
int vtkSegmentEditorLogic::maximumNumberOfUndoStates() const
{
  return SegmentationHistory->GetMaximumNumberOfStates();
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::setMaximumNumberOfUndoStates(int maxNumberOfStates)
{
  SegmentationHistory->SetMaximumNumberOfStates(maxNumberOfStates);
}

//------------------------------------------------------------------------------
bool vtkSegmentEditorLogic::readOnly() const
{
  return Locked;
}

//------------------------------------------------------------------------------
void vtkSegmentEditorLogic::setReadOnly(bool aReadOnly)
{
  Locked = aReadOnly;
  if (aReadOnly)
  {
    setActiveEffect(nullptr);
  }
}

//------------------------------------------------------------------------------
void vtkSegmentEditorLogic::toggleSourceVolumeIntensityMaskEnabled()
{
  if (!ParameterSetNode)
  {
    vtkErrorMacro(": Invalid segment editor parameter set node");
    return;
  }
  ParameterSetNode->SetSourceVolumeIntensityMask(!ParameterSetNode->GetSourceVolumeIntensityMask());
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::undo()
{
  if (!SegmentationNode)
  {
    return;
  }

  MRMLNodeModifyBlocker blocker(SegmentationNode);
  SegmentationHistory->RestorePreviousState();
  SegmentationNode->InvokeCustomModifiedEvent(vtkMRMLDisplayableNode::DisplayModifiedEvent, SegmentationNode->GetDisplayNode());
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::redo()
{
  if (!SegmentationNode)
  {
    return;
  }

  MRMLNodeModifyBlocker blocker(SegmentationNode);
  SegmentationHistory->RestoreNextState();
  SegmentationNode->InvokeCustomModifiedEvent(vtkMRMLDisplayableNode::DisplayModifiedEvent, SegmentationNode->GetDisplayNode());
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::isUndoEnabled() const
{
  return !Locked && SegmentationHistory->IsRestorePreviousStateAvailable();
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorLogic::isRedoEnabled() const
{
  return !Locked && SegmentationHistory->IsRestoreNextStateAvailable();
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::hideLabelLayer()
{
  auto sliceNodes = mrmlScene()->GetNodesByClass<vtkMRMLSliceNode>("vtkMRMLSliceNode");
  for (const auto& sliceNode : sliceNodes)
  {
    if (!sliceNode)
    {
      continue;
    }

    auto sliceLogic = vtkMRMLSliceLogic::SafeDownCast(sliceNode->GetLogic());
    if (!sliceLogic)
    {
      continue;
    }
    if (!segmentationDisplayableInView(sliceLogic->GetSliceNode()))
    {
      continue;
    }
    vtkMRMLSliceCompositeNode* sliceCompositeNode = sliceLogic->GetSliceCompositeNode();
    if (!sliceCompositeNode)
    {
      continue;
    }
    sliceCompositeNode->SetLabelVolumeID(nullptr);
  }
}

//---------------------------------------------------------------------------
bool vtkSegmentEditorLogic::turnOffLightboxes()
{
  auto sliceNodes = mrmlScene()->GetNodesByClass<vtkMRMLSliceNode>("vtkMRMLSliceNode");
  bool lightboxFound = false;
  for (const auto& sliceNode : sliceNodes)
  {
    if (!sliceNode || !segmentationDisplayableInView(sliceNode))
    {
      continue;
    }

    if (sliceNode->GetLayoutGridRows() != 1 || sliceNode->GetLayoutGridColumns() != 1)
    {
      lightboxFound = true;
      sliceNode->SetLayoutGrid(1, 1);
    }
  }

  return lightboxFound;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onExportToColorTableActionClicked()
{
  vtkMRMLSegmentationNode* segmentationNode = ParameterSetNode ? ParameterSetNode->GetSegmentationNode() : nullptr;
  if (!segmentationNode)
  {
    return;
  }

  vtkMRMLColorTableNode* newColorTable = vtkSlicerSegmentationsModuleLogic::AddColorTableNodeForSegmentation(segmentationNode);
  if (newColorTable == nullptr)
  {
    vtkErrorMacro("Failed to create color table node for segmentation " << segmentationNode->GetName());
    return;
  }

  // Export all segments to the new color table
  std::vector<std::string> segmentIDs;
  if (!vtkSlicerSegmentationsModuleLogic::ExportSegmentsToColorTableNode(segmentationNode, segmentIDs, newColorTable))
  {
    vtkErrorMacro("Failed to export color and terminology information from segmentation " + std::string(segmentationNode->GetName()) + " to color table )"
                  + newColorTable->GetName());
    return;
  }
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorLogic::onSegmentationDisplayModified()
{
  // Update observed views
  if (!ViewsObserved)
  {
    // no views are observed, nothing to do
    return;
  }

  if (!SegmentationNode)
  {
    return;
  }

  bool observedNodeIDsChanged = false;
  if (ObservedViewNodeIDs.size() == SegmentationNode->GetNumberOfDisplayNodes())
  {
    int numberOfDisplayNodes = SegmentationNode->GetNumberOfDisplayNodes();
    for (int displayNodeIndex = 0; displayNodeIndex < numberOfDisplayNodes; displayNodeIndex++)
    {
      vtkMRMLDisplayNode* segmentationDisplayNode = SegmentationNode->GetNthDisplayNode(displayNodeIndex);
      if (!segmentationDisplayNode)
      {
        continue;
      }
      if (ObservedViewNodeIDs.find(segmentationDisplayNode->GetID()) == std::end(ObservedViewNodeIDs))
      {
        observedNodeIDsChanged = true;
        break;
      }
      if (ObservedViewNodeIDs[segmentationDisplayNode->GetID()] != segmentationDisplayNode->GetViewNodeIDs())
      {
        observedNodeIDsChanged = true;
        break;
      }
    }
  }
  else
  {
    observedNodeIDsChanged = true;
  }
  if (!observedNodeIDsChanged)
  {
    return;
  }

  // Refresh view observations
  InvokeEvent(LayoutChangedEvent);
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::createAndSetBlankSourceVolumeIfNeeded()
{
  // If no source volume is selected but a valid geometry is specified then create a blank source volume
  if (ParameterSetNode->GetSourceVolumeNode() != nullptr)
    return;

  std::string referenceImageGeometry = getReferenceImageGeometryFromSegmentation(SegmentationNode->GetSegmentation());
  vtkNew<vtkMatrix4x4> referenceGeometryMatrix;
  int referenceExtent[6] = { 0, -1, 0, -1, 0, -1 };
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, referenceGeometryMatrix.GetPointer(), referenceExtent);
  if (referenceExtent[0] <= referenceExtent[1]    //
      && referenceExtent[2] <= referenceExtent[3] //
      && referenceExtent[4] <= referenceExtent[5])
  {
    // Create new image, allocate memory
    vtkNew<vtkOrientedImageData> blankImage;
    vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, blankImage.GetPointer());
    vtkOrientedImageDataResample::FillImage(blankImage.GetPointer(), 0.0);

    // Create volume node from blank image
    std::string sourceVolumeNodeName = (SegmentationNode->GetName() ? SegmentationNode->GetName() : "Volume") + std::string(" source volume");
    vtkMRMLScalarVolumeNode* sourceVolumeNode =
      vtkMRMLScalarVolumeNode::SafeDownCast(this->mrmlScene()->AddNewNodeByClass("vtkMRMLScalarVolumeNode", sourceVolumeNodeName.c_str()));
    sourceVolumeNode->SetAndObserveTransformNodeID(SegmentationNode->GetTransformNodeID());
    vtkSlicerSegmentationsModuleLogic::CopyOrientedImageDataToVolumeNode(blankImage.GetPointer(), sourceVolumeNode);

    // Use blank volume as master
    this->setSourceVolumeNode(sourceVolumeNode);
  }
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::selectPreviousSegment()
{
  this->selectSegmentAtOffset(-1);
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::selectNextSegment()
{
  this->selectSegmentAtOffset(1);
}

//---------------------------------------------------------------------------
std::vector<std::string> vtkSegmentEditorLogic::getVisibleSegmentIDs()
{
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(this->segmentationNode());
  if (segmentationNode == nullptr || segmentationNode->GetDisplayNode() == nullptr)
  {
    return {};
  }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (displayNode == nullptr)
  {
    return {};
  }
  std::vector<std::string> visibleSegmentIDs;
  displayNode->GetVisibleSegmentIDs(visibleSegmentIDs);
  return visibleSegmentIDs;
}

//---------------------------------------------------------------------------
bool isSegmentIDValid(const std::string& segmentID)
{
  return !segmentID.empty();
}

//---------------------------------------------------------------------------
bool vtkSegmentEditorLogic::isSegmentIDVisible(const std::string& segmentID)
{
  std::vector<std::string> visibleSegmentIDs = getVisibleSegmentIDs();
  return std::find(std::begin(visibleSegmentIDs), std::end(visibleSegmentIDs), segmentID) != std::end(visibleSegmentIDs);
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::selectSegmentAtOffset(int offset)
{
  std::vector<std::string> visibleSegmentIDs = getVisibleSegmentIDs();

  auto newId = getSelectedSegmentID(offset);
  bool isValid = !newId.empty();
  bool isDisplayed = std::find(std::begin(visibleSegmentIDs), std::end(visibleSegmentIDs), newId) != std::end(visibleSegmentIDs);
  if (isValid && isDisplayed)
  {
    this->setCurrentSegmentID(newId.c_str());
  }
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::pauseRender()
{
  InvokeEvent(PauseRenderEvent);
}

//---------------------------------------------------------------------------
void vtkSegmentEditorLogic::resumeRender()
{
  InvokeEvent(ResumeRenderEvent);
}
