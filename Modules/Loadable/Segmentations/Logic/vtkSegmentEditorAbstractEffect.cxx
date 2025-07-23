/*==============================================================================

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
#include "vtkSegmentEditorAbstractEffect.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSegmentEditorNode.h"

#include "vtkSlicerSegmentationsModuleLogic.h"

// SegmentationCore includes
#include <vtkOrientedImageData.h>
#include <vtkOrientedImageDataResample.h>

// Slicer includes
#include "vtkMRMLSliceLogic.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLSliceNode.h"
#include "vtkMRMLViewNode.h"
#include "vtkMRMLInteractionNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkMRMLTransformNode.h"

// VTK includes
#include <vtkImageConstantPad.h>
#include <vtkImageShiftScale.h>
#include <vtkImageThreshold.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkProp.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>
#include <vtkObjectFactory.h>

#include "vtkMouseCursor.h"


//----------------------------------------------------------------------------
/// \brief Helper class to emit pause / resume events on construction and destruction
class RenderBlocker
{
public:
  explicit RenderBlocker(vtkObject* obj, size_t pauseEvent, size_t resumeEvent)
    : m_obj{ obj }
    , m_pauseEvent(pauseEvent)
    , m_resumeEvent(resumeEvent)
  {
    if (m_obj)
    {
      m_obj->InvokeEvent(m_pauseEvent);
    }
  }

  ~RenderBlocker()
  {
    if (m_obj)
    {
      m_obj->InvokeEvent(m_resumeEvent);
    }
  }

private:
  vtkWeakPointer<vtkObject> m_obj;
  size_t m_pauseEvent{};
  size_t m_resumeEvent{};
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSegmentEditorAbstractEffect);

//----------------------------------------------------------------------------
vtkSegmentEditorAbstractEffect::~vtkSegmentEditorAbstractEffect() = default;

//-----------------------------------------------------------------------------
std::string vtkSegmentEditorAbstractEffect::name() const
{
  if (m_Name.empty())
  {
    vtkErrorMacro("Empty effect name!");
  }
  return this->m_Name;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setName(const std::string& name)
{
  m_Name = name;
}

//-----------------------------------------------------------------------------
std::string vtkSegmentEditorAbstractEffect::title() const
{
  if (!this->m_Title.empty())
  {
    return this->m_Title;
  }
  else if (!this->m_Name.empty())
  {
    return this->m_Name;
  }
  else
  {
    vtkWarningMacro("Empty effect title!");
    return std::string();
  }
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setTitle(std::string title)
{
  this->m_Title = title;
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorAbstractEffect::perSegment() const
{
  return this->m_PerSegment;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setPerSegment(bool perSegment)
{
  (void)(perSegment);
  vtkErrorMacro("Cannot set per-segment flag by method, only in constructor!");
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorAbstractEffect::requireSegments() const
{
  return this->m_RequireSegments;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setRequireSegments(bool requireSegments)
{
  this->m_RequireSegments = requireSegments;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::activate()
{
  this->m_Active = true;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::deactivate()
{
  this->m_Active = false;
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorAbstractEffect::active()
{
  return m_Active;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::applyImageMask(vtkOrientedImageData* input, vtkOrientedImageData* mask, double fillValue, bool notMask /*=false*/)
{
  // The method is now moved to vtkOrientedImageDataResample::ApplyImageMask but kept here
  // for a while for backward compatibility.
  vtkOrientedImageDataResample::ApplyImageMask(input, mask, fillValue, notMask);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::modifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap, ModificationMode modificationMode, bool bypassMasking /*=false*/)
{
  int modificationExtent[6] = { 0, -1, 0, -1, 0, -1 };
  this->modifySelectedSegmentByLabelmap(modifierLabelmap, modificationMode, modificationExtent, bypassMasking);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::modifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap,
                                                                     ModificationMode modificationMode,
                                                                     const std::vector<int>& extent,
                                                                     bool bypassMasking /*=false*/)
{
  if (extent.size() != 6)
  {
    vtkErrorWithObjectMacro(nullptr, "failed: extent must have 6 int values");
    return;
  }
  int modificationExtent[6] = { extent[0], extent[1], extent[2], extent[3], extent[4], extent[5] };
  this->modifySelectedSegmentByLabelmap(modifierLabelmap, modificationMode, modificationExtent, bypassMasking);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::modifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap,
                                                                     ModificationMode modificationMode,
                                                                     const int modificationExtent[6],
                                                                     bool bypassMasking /*=false*/)
{
  this->modifySegmentByLabelmap(this->parameterSetNode()->GetSegmentationNode(),
                                this->parameterSetNode()->GetSelectedSegmentID() ? this->parameterSetNode()->GetSelectedSegmentID() : "",
                                modifierLabelmap,
                                modificationMode,
                                modificationExtent,
                                bypassMasking);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::modifySegmentByLabelmap(vtkMRMLSegmentationNode* segmentationNode,
                                                             const char* segmentID,
                                                             vtkOrientedImageData* modifierLabelmap,
                                                             ModificationMode modificationMode,
                                                             bool bypassMasking /*=false*/)
{
  int modificationExtent[6] = { 0, -1, 0, -1, 0, -1 };
  this->modifySegmentByLabelmap(segmentationNode, segmentID, modifierLabelmap, modificationMode, modificationExtent, bypassMasking);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::modifySegmentByLabelmap(vtkMRMLSegmentationNode* segmentationNode,
                                                             const char* segmentID,
                                                             vtkOrientedImageData* modifierLabelmapInput,
                                                             ModificationMode modificationMode,
                                                             const int modificationExtent[6],
                                                             bool bypassMasking /*=false*/)
{
  vtkMRMLSegmentEditorNode* parameterSetNode = this->parameterSetNode();
  if (!parameterSetNode)
  {
    vtkErrorMacro("Invalid segment editor parameter set node");
    this->defaultModifierLabelmap();
    return;
  }

  if (!segmentationNode)
  {
    vtkErrorMacro("Invalid segmentation");
    this->defaultModifierLabelmap();
    return;
  }

  vtkSegment* segment = nullptr;
  if (segmentID)
  {
    segment = segmentationNode->GetSegmentation()->GetSegment(segmentID);
  }
  if (!segment)
  {
    vtkErrorMacro("Invalid segment");
    this->defaultModifierLabelmap();
    return;
  }

  if (!modifierLabelmapInput)
  {
    // If per-segment flag is off, then it is not an error (the effect itself has written it back to segmentation)
    if (this->perSegment())
    {
      vtkErrorMacro("Cannot apply edit operation because modifier labelmap cannot be accessed");
    }
    this->defaultModifierLabelmap();
    return;
  }

  // Prevent disappearing and reappearing of 3D representation during update
  RenderBlocker renderBlocker(this, PauseRenderEvent, ResumeRenderEvent);

  vtkSmartPointer<vtkOrientedImageData> modifierLabelmap = modifierLabelmapInput;
  if ((!bypassMasking && parameterSetNode->GetMaskMode() != vtkMRMLSegmentationNode::EditAllowedEverywhere) || parameterSetNode->GetSourceVolumeIntensityMask())
  {
    vtkNew<vtkOrientedImageData> maskImage;
    maskImage->SetExtent(modifierLabelmap->GetExtent());
    maskImage->SetSpacing(modifierLabelmap->GetSpacing());
    maskImage->SetOrigin(modifierLabelmap->GetOrigin());
    maskImage->CopyDirections(modifierLabelmap);
    maskImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    vtkOrientedImageDataResample::FillImage(maskImage, m_EraseValue);

    // Apply mask to modifier labelmap if masking is enabled
    if (!bypassMasking && parameterSetNode->GetMaskMode() != vtkMRMLSegmentationNode::EditAllowedEverywhere)
    {
      vtkOrientedImageDataResample::ModifyImage(maskImage, this->maskLabelmap(), vtkOrientedImageDataResample::OPERATION_MAXIMUM);
    }

    // Apply threshold mask if paint threshold is turned on
    if (parameterSetNode->GetSourceVolumeIntensityMask())
    {
      vtkOrientedImageData* sourceVolumeOrientedImageData = this->sourceVolumeImageData();
      if (!sourceVolumeOrientedImageData)
      {
        vtkErrorMacro("Unable to get source volume image");
        this->defaultModifierLabelmap();
        return;
      }
      // Make sure the modifier labelmap has the same geometry as the source volume
      if (!vtkOrientedImageDataResample::DoGeometriesMatch(modifierLabelmap, sourceVolumeOrientedImageData))
      {
        vtkErrorMacro("Modifier labelmap should have the same geometry as the source volume");
        this->defaultModifierLabelmap();
        return;
      }

      // Create threshold image
      vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
      threshold->SetInputData(sourceVolumeOrientedImageData);
      threshold->ThresholdBetween(parameterSetNode->GetSourceVolumeIntensityMaskRange()[0], parameterSetNode->GetSourceVolumeIntensityMaskRange()[1]);
      threshold->SetInValue(m_EraseValue);
      threshold->SetOutValue(m_FillValue);
      threshold->SetOutputScalarTypeToUnsignedChar();
      threshold->Update();

      vtkSmartPointer<vtkOrientedImageData> thresholdMask = vtkSmartPointer<vtkOrientedImageData>::New();
      thresholdMask->ShallowCopy(threshold->GetOutput());
      vtkSmartPointer<vtkMatrix4x4> modifierLabelmapToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      modifierLabelmap->GetImageToWorldMatrix(modifierLabelmapToWorldMatrix);
      thresholdMask->SetGeometryFromImageToWorldMatrix(modifierLabelmapToWorldMatrix);
      vtkOrientedImageDataResample::ModifyImage(maskImage, thresholdMask, vtkOrientedImageDataResample::OPERATION_MAXIMUM);
    }

    vtkSmartPointer<vtkOrientedImageData> segmentLayerLabelmap =
      vtkOrientedImageData::SafeDownCast(segment->GetRepresentation(segmentationNode->GetSegmentation()->GetSourceRepresentationName()));
    if (segmentLayerLabelmap && this->parameterSetNode()->GetMaskMode() == vtkMRMLSegmentationNode::EditAllowedInsideSingleSegment
        && modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeRemove)
    {
      // If we are painting inside a segment, the erase effect can modify the current segment outside the masking region by adding back regions
      // in the current segment. Add the current segment to the editable area
      vtkNew<vtkImageThreshold> segmentInverter;
      segmentInverter->SetInputData(segmentLayerLabelmap);
      segmentInverter->SetInValue(m_EraseValue);
      segmentInverter->SetOutValue(m_FillValue);
      segmentInverter->ReplaceInOn();
      segmentInverter->ThresholdBetween(segment->GetLabelValue(), segment->GetLabelValue());
      segmentInverter->SetOutputScalarTypeToUnsignedChar();
      segmentInverter->Update();

      vtkNew<vtkOrientedImageData> invertedSegment;
      invertedSegment->ShallowCopy(segmentInverter->GetOutput());
      invertedSegment->CopyDirections(segmentLayerLabelmap);
      vtkOrientedImageDataResample::ModifyImage(maskImage, invertedSegment, vtkOrientedImageDataResample::OPERATION_MINIMUM);
    }

    // Apply the mask to the modifier labelmap. Make a copy so that we don't modify the original.
    modifierLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();
    modifierLabelmap->DeepCopy(modifierLabelmapInput);
    vtkOrientedImageDataResample::ApplyImageMask(modifierLabelmap, maskImage, m_EraseValue, true);

    if (segmentLayerLabelmap && modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeSet)
    {
      // If modification mode is "set", we don't want to erase the existing labelmap outside of the mask region,
      // so we need to add it to the modifier labelmap
      vtkNew<vtkImageThreshold> segmentThreshold;
      segmentThreshold->SetInputData(segmentLayerLabelmap);
      segmentThreshold->SetInValue(m_FillValue);
      segmentThreshold->SetOutValue(m_EraseValue);
      segmentThreshold->ReplaceInOn();
      segmentThreshold->ThresholdBetween(segment->GetLabelValue(), segment->GetLabelValue());
      segmentThreshold->SetOutputScalarTypeToUnsignedChar();
      segmentThreshold->Update();

      int segmentThresholdExtent[6] = { 0, -1, 0, -1, 0, -1 };
      segmentThreshold->GetOutput()->GetExtent(segmentThresholdExtent);
      if (segmentThresholdExtent[0] <= segmentThresholdExtent[1] && segmentThresholdExtent[2] <= segmentThresholdExtent[3]
          && segmentThresholdExtent[4] <= segmentThresholdExtent[5])
      {
        vtkNew<vtkOrientedImageData> segmentOutsideMask;
        segmentOutsideMask->ShallowCopy(segmentThreshold->GetOutput());
        segmentOutsideMask->CopyDirections(segmentLayerLabelmap);
        vtkOrientedImageDataResample::ModifyImage(segmentOutsideMask, maskImage, vtkOrientedImageDataResample::OPERATION_MINIMUM);
        vtkOrientedImageDataResample::ModifyImage(modifierLabelmap, segmentOutsideMask, vtkOrientedImageDataResample::OPERATION_MAXIMUM);
      }
    }
  }

  // Copy the temporary padded modifier labelmap to the segment.
  // Mask and threshold was already applied on modifier labelmap at this point if requested.
  const int* extent = modificationExtent;
  if (extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5])
  {
    // invalid extent, it means we have to work with the entire modifier labelmap
    extent = nullptr;
  }

  std::vector<std::string> allSegmentIDs;
  segmentationNode->GetSegmentation()->GetSegmentIDs(allSegmentIDs);
  // remove selected segment, that is already handled
  allSegmentIDs.erase(std::remove(allSegmentIDs.begin(), allSegmentIDs.end(), segmentID), allSegmentIDs.end());

  std::vector<std::string> visibleSegmentIDs;
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (displayNode)
  {
    for (std::vector<std::string>::iterator segmentIDIt = allSegmentIDs.begin(); segmentIDIt != allSegmentIDs.end(); ++segmentIDIt)
    {
      if (displayNode->GetSegmentVisibility(*segmentIDIt))
      {
        visibleSegmentIDs.push_back(*segmentIDIt);
      }
    }
  }

  std::vector<std::string> segmentIDsToOverwrite;
  switch (this->parameterSetNode()->GetOverwriteMode())
  {
    case vtkMRMLSegmentEditorNode::OverwriteNone:
      // nothing to overwrite
      break;
    case vtkMRMLSegmentEditorNode::OverwriteVisibleSegments: segmentIDsToOverwrite = visibleSegmentIDs; break;
    case vtkMRMLSegmentEditorNode::OverwriteAllSegments: segmentIDsToOverwrite = allSegmentIDs; break;
  }

  if (bypassMasking)
  {
    segmentIDsToOverwrite.clear();
  }

  if (modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeRemoveAll)
  {
    // If we want to erase all segments, then mark all segments as overwritable
    segmentIDsToOverwrite = allSegmentIDs;
  }

  // Create inverted binary labelmap
  vtkSmartPointer<vtkImageThreshold> inverter = vtkSmartPointer<vtkImageThreshold>::New();
  inverter->SetInputData(modifierLabelmap);
  inverter->SetInValue(VTK_UNSIGNED_CHAR_MAX);
  inverter->SetOutValue(m_EraseValue);
  inverter->ThresholdByLower(0);
  inverter->SetOutputScalarTypeToUnsignedChar();

  if (modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeSet)
  {
    vtkSmartPointer<vtkImageThreshold> segmentInverter = vtkSmartPointer<vtkImageThreshold>::New();
    segmentInverter->SetInputData(segment->GetRepresentation(segmentationNode->GetSegmentation()->GetSourceRepresentationName()));
    segmentInverter->SetInValue(m_EraseValue);
    segmentInverter->SetOutValue(VTK_UNSIGNED_CHAR_MAX);
    segmentInverter->ReplaceInOn();
    segmentInverter->ThresholdBetween(segment->GetLabelValue(), segment->GetLabelValue());
    segmentInverter->SetOutputScalarTypeToUnsignedChar();
    segmentInverter->Update();
    vtkNew<vtkOrientedImageData> invertedModifierLabelmap;
    invertedModifierLabelmap->ShallowCopy(segmentInverter->GetOutput());
    vtkNew<vtkMatrix4x4> imageToWorldMatrix;
    modifierLabelmap->GetImageToWorldMatrix(imageToWorldMatrix.GetPointer());
    invertedModifierLabelmap->SetGeometryFromImageToWorldMatrix(imageToWorldMatrix.GetPointer());
    if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(
          invertedModifierLabelmap.GetPointer(), segmentationNode, segmentID, vtkSlicerSegmentationsModuleLogic::MODE_MERGE_MIN, nullptr, false, segmentIDsToOverwrite))
    {
      vtkErrorMacro("Failed to remove modifier labelmap from selected segment");
    }
    if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(
          modifierLabelmap, segmentationNode, segmentID, vtkSlicerSegmentationsModuleLogic::MODE_MERGE_MASK, extent, false, segmentIDsToOverwrite))
    {
      vtkErrorMacro("Failed to add modifier labelmap to selected segment");
    }
  }
  else if (modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeAdd)
  {
    if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(
          modifierLabelmap, segmentationNode, segmentID, vtkSlicerSegmentationsModuleLogic::MODE_MERGE_MASK, extent, false, segmentIDsToOverwrite))
    {
      vtkErrorMacro("Failed to add modifier labelmap to selected segment");
    }
  }
  else if (modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeRemove || modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeRemoveAll)
  {
    inverter->Update();
    vtkNew<vtkOrientedImageData> invertedModifierLabelmap;
    invertedModifierLabelmap->ShallowCopy(inverter->GetOutput());
    vtkNew<vtkMatrix4x4> imageToWorldMatrix;
    modifierLabelmap->GetImageToWorldMatrix(imageToWorldMatrix.GetPointer());
    invertedModifierLabelmap->SetGeometryFromImageToWorldMatrix(imageToWorldMatrix.GetPointer());
    bool minimumOfAllSegments = modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeRemoveAll;
    if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(invertedModifierLabelmap.GetPointer(),
                                                                       segmentationNode,
                                                                       segmentID,
                                                                       vtkSlicerSegmentationsModuleLogic::MODE_MERGE_MIN,
                                                                       extent,
                                                                       minimumOfAllSegments,
                                                                       segmentIDsToOverwrite))
    {
      vtkErrorMacro("Failed to remove modifier labelmap from selected segment");
    }
  }

  if (segment)
  {
    if (vtkSlicerSegmentationsModuleLogic::GetSegmentStatus(segment) == vtkSlicerSegmentationsModuleLogic::NotStarted)
    {
      vtkSlicerSegmentationsModuleLogic::SetSegmentStatus(segment, vtkSlicerSegmentationsModuleLogic::InProgress);
    }
  }

  std::vector<std::string> sharedSegmentIDs;
  segmentationNode->GetSegmentation()->GetSegmentIDsSharingBinaryLabelmapRepresentation(segmentID, sharedSegmentIDs, false);

  std::vector<std::string> segmentsToErase;
  for (std::string segmentIDToOverwrite : segmentIDsToOverwrite)
  {
    std::vector<std::string>::iterator foundSegmentIDIt = std::find(sharedSegmentIDs.begin(), sharedSegmentIDs.end(), segmentIDToOverwrite);
    if (foundSegmentIDIt == sharedSegmentIDs.end())
    {
      segmentsToErase.push_back(segmentIDToOverwrite);
    }
  }

  if (!segmentsToErase.empty()
      && (modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeSet || modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeAdd
          || modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeRemoveAll))
  {
    inverter->Update();
    vtkNew<vtkOrientedImageData> invertedModifierLabelmap;
    invertedModifierLabelmap->ShallowCopy(inverter->GetOutput());
    vtkNew<vtkMatrix4x4> imageToWorldMatrix;
    modifierLabelmap->GetImageToWorldMatrix(imageToWorldMatrix.GetPointer());
    invertedModifierLabelmap->SetGeometryFromImageToWorldMatrix(imageToWorldMatrix.GetPointer());

    std::map<vtkDataObject*, bool> erased;
    for (std::string eraseSegmentID : segmentsToErase)
    {
      vtkSegment* currentSegment = segmentationNode->GetSegmentation()->GetSegment(eraseSegmentID);
      vtkDataObject* dataObject = currentSegment->GetRepresentation(vtkSegmentationConverter::GetBinaryLabelmapRepresentationName());
      if (erased[dataObject])
      {
        continue;
      }
      erased[dataObject] = true;

      vtkOrientedImageData* currentLabelmap = vtkOrientedImageData::SafeDownCast(dataObject);

      std::vector<std::string> dontOverwriteIDs;
      std::vector<std::string> currentSharedIDs;
      segmentationNode->GetSegmentation()->GetSegmentIDsSharingBinaryLabelmapRepresentation(eraseSegmentID, currentSharedIDs, true);
      for (std::string sharedSegmentID : currentSharedIDs)
      {
        if (std::find(segmentsToErase.begin(), segmentsToErase.end(), sharedSegmentID) == segmentsToErase.end())
        {
          dontOverwriteIDs.push_back(sharedSegmentID);
        }
      }

      vtkSmartPointer<vtkOrientedImageData> invertedModifierLabelmap2 = invertedModifierLabelmap;
      if (dontOverwriteIDs.size() > 0)
      {
        invertedModifierLabelmap2 = vtkSmartPointer<vtkOrientedImageData>::New();
        invertedModifierLabelmap2->DeepCopy(invertedModifierLabelmap);

        vtkNew<vtkOrientedImageData> maskImage;
        maskImage->CopyDirections(currentLabelmap);
        for (std::string dontOverwriteID : dontOverwriteIDs)
        {
          vtkSegment* dontOverwriteSegment = segmentationNode->GetSegmentation()->GetSegment(dontOverwriteID);
          vtkNew<vtkImageThreshold> threshold;
          threshold->SetInputData(currentLabelmap);
          threshold->ThresholdBetween(dontOverwriteSegment->GetLabelValue(), dontOverwriteSegment->GetLabelValue());
          threshold->SetInValue(1);
          threshold->SetOutValue(0);
          threshold->SetOutputScalarTypeToUnsignedChar();
          threshold->Update();
          maskImage->ShallowCopy(threshold->GetOutput());
          vtkOrientedImageDataResample::ApplyImageMask(invertedModifierLabelmap2, maskImage, VTK_UNSIGNED_CHAR_MAX, true);
        }
      }

      if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(
            invertedModifierLabelmap2, segmentationNode, eraseSegmentID, vtkSlicerSegmentationsModuleLogic::MODE_MERGE_MIN, extent, true, segmentIDsToOverwrite))
      {
        vtkErrorMacro("Failed to set modifier labelmap to segment " + eraseSegmentID);
      }
    }
  }
  else if (modificationMode == vtkSegmentEditorAbstractEffect::ModificationModeRemove
           && this->parameterSetNode()->GetMaskMode() == vtkMRMLSegmentationNode::EditAllowedInsideSingleSegment && this->parameterSetNode()->GetMaskSegmentID()
           && strcmp(this->parameterSetNode()->GetMaskSegmentID(), segmentID) != 0)
  {
    // In general, we don't try to "add back" areas to other segments when an area is removed from the selected segment.
    // The only exception is when we draw inside one specific segment. In that case erasing adds to the mask segment. It is useful
    // for splitting a segment into two by painting.
    if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(modifierLabelmap,
                                                                       segmentationNode,
                                                                       this->parameterSetNode()->GetMaskSegmentID(),
                                                                       vtkSlicerSegmentationsModuleLogic::MODE_MERGE_MASK,
                                                                       extent,
                                                                       false,
                                                                       segmentIDsToOverwrite))
    {
      vtkErrorMacro("Failed to add back modifier labelmap to segment " + std::string(this->parameterSetNode()->GetMaskSegmentID()));
    }
  }

  // Make sure the segmentation node is under the same parent as the source volume
  vtkMRMLScalarVolumeNode* sourceVolumeNode = m_ParameterSetNode->GetSourceVolumeNode();
  if (sourceVolumeNode)
  {
    vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(m_ParameterSetNode->GetScene());
    if (shNode)
    {
      vtkIdType segmentationId = shNode->GetItemByDataNode(segmentationNode);
      vtkIdType sourceVolumeShId = shNode->GetItemByDataNode(sourceVolumeNode);
      if (segmentationId && sourceVolumeShId)
      {
        shNode->SetItemParent(segmentationId, shNode->GetItemParent(sourceVolumeShId));
      }
      else
      {
        vtkErrorMacro("Subject hierarchy items not found for segmentation or source volume");
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::selectEffect(std::string effectName)
{
  InvokeEvent(SelectEffectEvent, static_cast<void*>(&effectName));
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::addViewProp(vtkRenderWindow* renderWindow, vtkMRMLAbstractViewNode* viewNode, vtkProp* actor)
{
  vtkRenderer* renderer = vtkSegmentEditorAbstractEffect::renderer(renderWindow);
  if (renderer)
  {
    renderer->AddViewProp(actor);
    viewNode->ScheduleRender();
  }
  else
  {
    vtkErrorMacro("Failed to get renderer for view widget");
  }
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::removeViewProp(vtkRenderWindow* renderWindow, vtkMRMLAbstractViewNode* viewNode, vtkProp* actor)
{
  vtkRenderer* renderer = vtkSegmentEditorAbstractEffect::renderer(renderWindow);
  if (renderer)
  {
    renderer->RemoveViewProp(actor);
    viewNode->ScheduleRender();
  }
  else
  {
    vtkErrorMacro("Failed to get renderer for view widget");
  }
}

//-----------------------------------------------------------------------------
vtkMRMLScene* vtkSegmentEditorAbstractEffect::scene()
{
  if (!m_ParameterSetNode)
  {
    return nullptr;
  }

  return m_ParameterSetNode->GetScene();
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentEditorNode* vtkSegmentEditorAbstractEffect::parameterSetNode()
{

  return m_ParameterSetNode.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setParameterSetNode(vtkMRMLSegmentEditorNode* node)
{

  m_ParameterSetNode = node;
}

//-----------------------------------------------------------------------------
std::string vtkSegmentEditorAbstractEffect::parameter(std::string name)
{
  if (!m_ParameterSetNode)
  {
    return std::string();
  }

  // Get effect-specific prefixed parameter first
  std::string attributeName = getAttributeName(name);
  const char* value = m_ParameterSetNode->GetAttribute(attributeName.c_str());
  // Look for common parameter if effect-specific one is not found
  if (!value)
  {
    value = m_ParameterSetNode->GetAttribute(name.c_str());
  }
  if (!value)
  {
    vtkErrorMacro("Parameter named " + name + " cannot be found for effect " + this->name());
    return std::string();
  }

  return std::string(value);
}

//-----------------------------------------------------------------------------
int vtkSegmentEditorAbstractEffect::integerParameter(std::string name)
{
  if (!m_ParameterSetNode)
  {
    return 0;
  }

  try
  {
    return std::stoi(this->parameter(name));
  }
  catch (...)
  {
    vtkErrorMacro("Parameter named " + name + " cannot be converted to integer!");
    return 0;
  }
}

//-----------------------------------------------------------------------------
double vtkSegmentEditorAbstractEffect::doubleParameter(std::string name)
{
  if (!m_ParameterSetNode)
  {
    return 0.0;
  }

  try
  {
    return std::stod(this->parameter(name));
  }
  catch (...)
  {
    vtkErrorMacro("Parameter named " + name + " cannot be converted to floating point number!");
    return 0;
  }
}

//-----------------------------------------------------------------------------
vtkMRMLNode* vtkSegmentEditorAbstractEffect::nodeReference(std::string name)
{
  if (!m_ParameterSetNode)
  {
    return nullptr;
  }

  // Get effect-specific prefixed parameter first
  vtkMRMLNode* node = m_ParameterSetNode->GetNodeReference(getAttributeName(name).c_str());
  // Look for common parameter if effect-specific one is not found
  if (!node)
  {
    node = m_ParameterSetNode->GetNodeReference(name.c_str());
  }
  return node;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setParameter(std::string name, std::string value)
{
  if (!m_ParameterSetNode)
  {
    vtkErrorMacro("Invalid segment editor parameter set node set to effect " + this->name());
    return;
  }

  // Set parameter as attribute
  this->setCommonParameter(getAttributeName(name), value);
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorAbstractEffect::parameterDefined(std::string name)
{
  return this->commonParameterDefined(getAttributeName(name));
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorAbstractEffect::commonParameterDefined(std::string name)
{
  if (!m_ParameterSetNode)
  {
    return false;
  }
  const char* existingValue = m_ParameterSetNode->GetAttribute(name.c_str());
  return (existingValue != nullptr && strlen(existingValue) > 0);
}

//-----------------------------------------------------------------------------
int vtkSegmentEditorAbstractEffect::confirmCurrentSegmentVisible()
{
  if (!this->parameterSetNode())
  {
    // no parameter set node - do not prevent operation
    return ConfirmedWithoutDialog;
  }
  vtkMRMLSegmentationNode* segmentationNode = this->parameterSetNode()->GetSegmentationNode();
  if (!segmentationNode)
  {
    // no segmentation node - do not prevent operation
    return ConfirmedWithoutDialog;
  }
  char* segmentID = this->parameterSetNode()->GetSelectedSegmentID();
  if (!segmentID || strlen(segmentID) == 0)
  {
    // no selected segment, probably this effect operates on the entire segmentation - do not prevent operation
    return ConfirmedWithoutDialog;
  }

  // If segment visibility is already confirmed for this segment then we don't need to ask again
  // (important for effects that are interrupted when painting/drawing on the image, because displaying a popup
  // interferes with painting on the image)
  vtkSegment* selectedSegment = nullptr;
  if (segmentationNode->GetSegmentation())
  {
    selectedSegment = segmentationNode->GetSegmentation()->GetSegment(segmentID);
  }
  if (this->m_AlreadyConfirmedSegmentVisible == selectedSegment)
  {
    return ConfirmedWithoutDialog;
  }

  int numberOfDisplayNodes = segmentationNode->GetNumberOfDisplayNodes();
  for (int displayNodeIndex = 0; displayNodeIndex < numberOfDisplayNodes; displayNodeIndex++)
  {
    vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetNthDisplayNode(displayNodeIndex));
    if (displayNode && displayNode->GetVisibility() && displayNode->GetSegmentVisibility(segmentID))
    {
      // segment already visible
      return ConfirmedWithoutDialog;
    }
  }

  vtkApplication* app = vtkApplication::application();
  QWidget* mainWindow = app ? app->mainWindow() : nullptr;

  ctkMessageBox* confirmCurrentSegmentVisibleMsgBox = new ctkMessageBox(mainWindow);
  confirmCurrentSegmentVisibleMsgBox->setAttribute(Qt::WA_DeleteOnClose);
  confirmCurrentSegmentVisibleMsgBox->setWindowTitle(tr("Operate on invisible segment?"));
  confirmCurrentSegmentVisibleMsgBox->setText(tr("The currently selected segment is hidden. Would you like to make it visible?"));

  confirmCurrentSegmentVisibleMsgBox->addButton(QMessageBox::Yes);
  confirmCurrentSegmentVisibleMsgBox->addButton(QMessageBox::No);
  confirmCurrentSegmentVisibleMsgBox->addButton(QMessageBox::Cancel);

  confirmCurrentSegmentVisibleMsgBox->setDontShowAgainVisible(true);
  confirmCurrentSegmentVisibleMsgBox->setDontShowAgainSettingsKey("Segmentations/ConfirmEditHiddenSegment");
  confirmCurrentSegmentVisibleMsgBox->addDontShowAgainButtonRole(QMessageBox::YesRole);
  confirmCurrentSegmentVisibleMsgBox->addDontShowAgainButtonRole(QMessageBox::NoRole);

  confirmCurrentSegmentVisibleMsgBox->setIcon(QMessageBox::Question);

  QSettings settings;
  int savedButtonSelection = settings.value(confirmCurrentSegmentVisibleMsgBox->dontShowAgainSettingsKey(), static_cast<int>(QMessageBox::InvalidRole)).toInt();

  int resultCode = confirmCurrentSegmentVisibleMsgBox->exec();

  // Cancel means that user did not let the operation to proceed
  if (resultCode == QMessageBox::Cancel)
  {
    return NotConfirmed;
  }

  // User chose to show the current segment
  if (resultCode == QMessageBox::Yes)
  {
    vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
    if (displayNode)
    {
      displayNode->SetVisibility(true);
      displayNode->SetSegmentVisibility(segmentID, true);
    }
  }
  else
  {
    // User confirmed that it is OK to work on this invisible segment
    this->m_AlreadyConfirmedSegmentVisible = selectedSegment;
  }

  // User confirmed that the operation can go on (did not click Cancel)
  if (savedButtonSelection == static_cast<int>(QMessageBox::InvalidRole))
  {
    return ConfirmedWithDialog;
  }
  else
  {
    return ConfirmedWithoutDialog;
  }
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setParameterDefault(std::string name, std::string value)
{
  if (this->parameterDefined(name))
  {
    return;
  }
  this->setParameter(name, value);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setCommonParameter(std::string name, std::string value)
{
  if (!m_ParameterSetNode)
  {
    vtkErrorMacro("Invalid segment editor parameter set node set to effect " + this->name());
    return;
  }

  const char* oldValue = m_ParameterSetNode->GetAttribute(name.c_str());
  if (oldValue == nullptr && value.empty())
  {
    // no change
    return;
  }
  if (value == std::string(oldValue))
  {
    // no change
    return;
  }

  // Disable full modified events in all cases (observe EffectParameterModified instead if necessary)
  int disableState = m_ParameterSetNode->GetDisableModifiedEvent();
  m_ParameterSetNode->SetDisableModifiedEvent(1);

  // Set parameter as attribute
  m_ParameterSetNode->SetAttribute(name.c_str(), value.c_str());

  // Re-enable full modified events for parameter node
  m_ParameterSetNode->SetDisableModifiedEvent(disableState);

  // Emit parameter modified event if requested
  // Don't pass parameter name as char pointer, as custom modified events may be compressed and invoked after EndModify()
  // and by that time the pointer may not be valid anymore.
  m_ParameterSetNode->InvokeCustomModifiedEvent(vtkMRMLSegmentEditorNode::EffectParameterModified);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setCommonParameterDefault(std::string name, std::string value)
{
  if (this->commonParameterDefined(name))
  {
    return;
  }
  this->setCommonParameter(name, value);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setParameter(std::string name, int value)
{
  this->setParameter(name, std::to_string(value));
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setParameterDefault(std::string name, int value)
{
  if (this->parameterDefined(name))
  {
    return;
  }
  this->setParameter(name, value);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setCommonParameter(std::string name, int value)
{
  this->setCommonParameter(name, std::to_string(value));
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setCommonParameterDefault(std::string name, int value)
{
  if (this->commonParameterDefined(name))
  {
    return;
  }
  this->setCommonParameter(name, value);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setParameter(std::string name, double value)
{
  this->setParameter(name, std::to_string(value));
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setParameterDefault(std::string name, double value)
{
  if (this->parameterDefined(name))
  {
    return;
  }
  this->setParameter(name, value);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setCommonParameter(std::string name, double value)
{
  this->setCommonParameter(name, std::to_string(value));
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setCommonParameterDefault(std::string name, double value)
{
  if (this->commonParameterDefined(name))
  {
    return;
  }
  this->setCommonParameter(name, value);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setNodeReference(std::string name, vtkMRMLNode* node)
{
  if (!m_ParameterSetNode)
  {
    vtkErrorMacro("Invalid segment editor parameter set node set to effect " + this->name());
    return;
  }

  // Set parameter as attribute
  std::string attributeName = getAttributeName(name);
  this->setCommonNodeReference(attributeName, node);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setCommonNodeReference(std::string name, vtkMRMLNode* node)
{
  if (!m_ParameterSetNode)
  {
    vtkErrorMacro("Invalid segment editor parameter set node set to effect " + this->name());
    return;
  }

  vtkMRMLNode* oldNode = m_ParameterSetNode->GetNodeReference(name.c_str());
  if (node == oldNode)
  {
    // no change
    return;
  }

  // Set parameter as attribute
  m_ParameterSetNode->SetNodeReferenceID(name.c_str(), node ? node->GetID() : nullptr);

  // Emit parameter modified event
  // Don't pass parameter name as char pointer, as custom modified events may be compressed and invoked after EndModify()
  // and by that time the pointer may not be valid anymore.
  m_ParameterSetNode->InvokeCustomModifiedEvent(vtkMRMLSegmentEditorNode::EffectParameterModified);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setVolumes(vtkOrientedImageData* alignedSourceVolume,
                                                vtkOrientedImageData* modifierLabelmap,
                                                vtkOrientedImageData* maskLabelmap,
                                                vtkOrientedImageData* selectedSegmentLabelmap,
                                                vtkOrientedImageData* referenceGeometryImage)
{
  m_ModifierLabelmap = modifierLabelmap;
  m_MaskLabelmap = maskLabelmap;
  m_AlignedSourceVolume = alignedSourceVolume;
  m_SelectedSegmentLabelmap = selectedSegmentLabelmap;
  m_ReferenceGeometryImage = referenceGeometryImage;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* vtkSegmentEditorAbstractEffect::defaultModifierLabelmap()
{
  bool success = false;
  emit d->updateVolumeSignal(d->ModifierLabelmap.GetPointer(), success); // this resets the labelmap and clears it
  if (!success)
  {
    return nullptr;
  }
  return d->ModifierLabelmap;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* vtkSegmentEditorAbstractEffect::modifierLabelmap()
{
  return m_ModifierLabelmap;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* vtkSegmentEditorAbstractEffect::maskLabelmap()
{
  bool success = false;
  emit d->updateVolumeSignal(d->MaskLabelmap.GetPointer(), success);
  if (!success)
  {
    return nullptr;
  }
  return m_MaskLabelmap;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* vtkSegmentEditorAbstractEffect::sourceVolumeImageData()
{
  bool success = false;
  emit d->updateVolumeSignal(d->AlignedSourceVolume.GetPointer(), success);
  if (!success)
  {
    return nullptr;
  }
  return m_AlignedSourceVolume;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* vtkSegmentEditorAbstractEffect::selectedSegmentLabelmap()
{
  bool success = false;
  emit d->updateVolumeSignal(d->SelectedSegmentLabelmap.GetPointer(), success);
  if (!success)
  {
    return nullptr;
  }
  return m_SelectedSegmentLabelmap;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* vtkSegmentEditorAbstractEffect::referenceGeometryImage()
{
  bool success = false;
  emit d->updateVolumeSignal(d->ReferenceGeometryImage.GetPointer(), success); // this resets the labelmap and clears it
  if (!success)
  {
    return nullptr;
  }
  return m_ReferenceGeometryImage;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::saveStateForUndo()
{
  InvokeEvent(SaveStateForUndoEvent);
}

//-----------------------------------------------------------------------------
vtkRenderer* vtkSegmentEditorAbstractEffect::renderer(vtkRenderWindow* renderWindow)
{
  if (!renderWindow)
  {
    return nullptr;
  }

  return vtkRenderer::SafeDownCast(renderWindow->GetRenderers()->GetItemAsObject(0));
}

//-----------------------------------------------------------------------------
std::array<int, 2> vtkSegmentEditorAbstractEffect::rasToXy(double ras[3], vtkMRMLSliceNode* sliceNode)
{
  std::array<int, 2> xy{};

  if (!sliceNode)
  {
    vtkErrorWithObjectMacro(nullptr, "Failed to get slice node!");
    return xy;
  }

  double rast[4] = { ras[0], ras[1], ras[2], 1.0 };
  double xyzw[4] = { 0.0, 0.0, 0.0, 1.0 };
  vtkSmartPointer<vtkMatrix4x4> rasToXyMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  rasToXyMatrix->DeepCopy(sliceNode->GetXYToRAS());
  rasToXyMatrix->Invert();
  rasToXyMatrix->MultiplyPoint(rast, xyzw);

  xy[0] = xyzw[0];
  xy[1] = xyzw[1];
  return xy;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::xyzToRas(double inputXyz[3], double outputRas[3], vtkMRMLSliceNode* sliceNode)
{
  outputRas[0] = outputRas[1] = outputRas[2] = 0.0;

  if (!sliceNode)
  {
    vtkErrorWithObjectMacro(nullptr, "Failed to get slice node!");
    return;
  }

  // x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the
  // slice you're looking at), hence xyToRAS is really performing xyzToRAS. RAS is patient world
  // coordinate system. Note the 1 is because the transform uses homogeneous coordinates.
  double xyzw[4] = { inputXyz[0], inputXyz[1], inputXyz[2], 1.0 };
  double rast[4] = { 0.0, 0.0, 0.0, 1.0 };
  sliceNode->GetXYToRAS()->MultiplyPoint(xyzw, rast);
  outputRas[0] = rast[0];
  outputRas[1] = rast[1];
  outputRas[2] = rast[2];
}

//-----------------------------------------------------------------------------
std::array<double, 3> vtkSegmentEditorAbstractEffect::xyzToRas(double inputXyz[3], vtkMRMLSliceNode* sliceNode)
{
  std::array<double, 3> outputRas = { 0.0, 0.0, 0.0 };
  xyzToRas(inputXyz, outputRas.data(), sliceNode);
  return outputRas;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::xyToRas(int xy[2], double outputRas[3], vtkMRMLSliceNode* sliceNode)
{
  double xyz[3] = { static_cast<double>(xy[0]), static_cast<double>(xy[1]), 0.0 };

  vtkSegmentEditorAbstractEffect::xyzToRas(xyz, outputRas, sliceNode);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::xyToRas(double xy[2], double outputRas[3], vtkMRMLSliceNode* sliceNode)
{
  double xyz[3] = { xy[0], xy[1], 0.0 };
  vtkSegmentEditorAbstractEffect::xyzToRas(xyz, outputRas, sliceNode);
}

//-----------------------------------------------------------------------------
std::array<double, 3> vtkSegmentEditorAbstractEffect::xyToRas(int xy[2], vtkMRMLSliceNode* sliceNode)
{
  std::array<double, 3> outputRas = { 0.0, 0.0, 0.0 };
  vtkSegmentEditorAbstractEffect::xyToRas(xy, outputRas.data(), sliceNode);
  return outputRas;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::xyzToIjk(double inputXyz[3],
                                              int outputIjk[3],
                                              vtkMRMLSliceNode* sliceNode,
                                              vtkOrientedImageData* image,
                                              vtkMRMLTransformNode* parentTransformNode /*=nullptr*/)
{
  outputIjk[0] = outputIjk[1] = outputIjk[2] = 0;

  if (!sliceNode || !image)
  {
    return;
  }

  // Convert from XY to RAS first
  double ras[3] = { 0.0, 0.0, 0.0 };
  vtkSegmentEditorAbstractEffect::xyzToRas(inputXyz, ras, sliceNode);

  // Move point from world to same transform as image
  if (parentTransformNode)
  {
    if (parentTransformNode->IsTransformToWorldLinear())
    {
      vtkNew<vtkMatrix4x4> worldToParentTransform;
      parentTransformNode->GetMatrixTransformFromWorld(worldToParentTransform);
      double worldPos[4] = { ras[0], ras[1], ras[2], 1.0 };
      double parentPos[4] = { 0.0 };
      worldToParentTransform->MultiplyPoint(worldPos, parentPos);
      ras[0] = parentPos[0];
      ras[1] = parentPos[1];
      ras[2] = parentPos[2];
    }
    else
    {
      vtkErrorWithObjectMacro(nullptr, "Parent transform is non-linear, which cannot be handled! Skipping.");
    }
  }

  // Convert RAS to image IJK
  double rast[4] = { ras[0], ras[1], ras[2], 1.0 };
  double ijkl[4] = { 0.0, 0.0, 0.0, 1.0 };
  vtkSmartPointer<vtkMatrix4x4> rasToIjkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  image->GetImageToWorldMatrix(rasToIjkMatrix);
  rasToIjkMatrix->Invert();
  rasToIjkMatrix->MultiplyPoint(rast, ijkl);

  outputIjk[0] = (int)(ijkl[0] + 0.5);
  outputIjk[1] = (int)(ijkl[1] + 0.5);
  outputIjk[2] = (int)(ijkl[2] + 0.5);
}

//-----------------------------------------------------------------------------
std::array<int, 3> vtkSegmentEditorAbstractEffect::xyzToIjk(double inputXyz[3],
                                                            vtkMRMLSliceNode* sliceNode,
                                                            vtkOrientedImageData* image,
                                                            vtkMRMLTransformNode* parentTransformNode /*=nullptr*/)
{
  std::array<int, 3> outputIjk = { 0, 0, 0 };
  vtkSegmentEditorAbstractEffect::xyzToIjk(inputXyz, outputIjk.data(), sliceNode, image, parentTransformNode);
  return outputIjk;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::xyToIjk(int xy[2],
                                             int outputIjk[3],
                                             vtkMRMLSliceNode* sliceNode,
                                             vtkOrientedImageData* image,
                                             vtkMRMLTransformNode* parentTransformNode /*=nullptr*/)
{
  double xyz[3] = { static_cast<double>(xy[0]), static_cast<double>(xy[1]), 0.0 };
  vtkSegmentEditorAbstractEffect::xyzToIjk(xyz, outputIjk, sliceNode, image, parentTransformNode);
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::xyToIjk(double xy[2],
                                             int outputIjk[3],
                                             vtkMRMLSliceNode* sliceNode,
                                             vtkOrientedImageData* image,
                                             vtkMRMLTransformNode* parentTransformNode /*=nullptr*/)
{
  double xyz[3] = { xy[0], xy[0], 0.0 };
  vtkSegmentEditorAbstractEffect::xyzToIjk(xyz, outputIjk, sliceNode, image, parentTransformNode);
}

//-----------------------------------------------------------------------------
std::array<int, 3> vtkSegmentEditorAbstractEffect::xyToIjk(int xy[2],
                                                           vtkMRMLSliceNode* sliceNode,
                                                           vtkOrientedImageData* image,
                                                           vtkMRMLTransformNode* parentTransformNode /*=nullptr*/)
{
  std::array<int, 3> outputIjk = { 0, 0, 0 };
  vtkSegmentEditorAbstractEffect::xyToIjk(xy, outputIjk.data(), sliceNode, image, parentTransformNode);
  return outputIjk;
}

//----------------------------------------------------------------------------
double vtkSegmentEditorAbstractEffect::sliceSpacing(vtkMRMLSliceNode* sliceNode)
{
  if (!sliceNode)
  {
    return 1.0;
  }

  // Implementation copied from vtkMRMLSliceViewInteractorStyle::GetSliceSpacing()
  double spacing = 1.0;
  if (sliceNode->GetSliceSpacingMode() == vtkMRMLSliceNode::PrescribedSliceSpacingMode)
  {
    spacing = sliceNode->GetPrescribedSliceSpacing()[2];
  }
  else
  {
    spacing = sliceNode->sliceLogic()->GetLowestVolumeSliceSpacing()[2];
  }
  return spacing;
}

//----------------------------------------------------------------------------
bool vtkSegmentEditorAbstractEffect::showEffectCursorInSliceView()
{
  return m_ShowEffectCursorInSliceView;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setShowEffectCursorInSliceView(bool show)
{
  this->m_ShowEffectCursorInSliceView = show;
}

//----------------------------------------------------------------------------
bool vtkSegmentEditorAbstractEffect::showEffectCursorInThreeDView()
{
  return m_ShowEffectCursorInThreeDView;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setShowEffectCursorInThreeDView(bool show)
{
  this->m_ShowEffectCursorInThreeDView = show;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::interactionNodeModified(vtkMRMLInteractionNode* interactionNode)
{
  if (interactionNode == nullptr)
  {
    return;
  }
  // Deactivate the effect if user switched to markup placement mode
  // to avoid double effect (e.g., paint & place fiducial at the same time)
  if (interactionNode->GetCurrentInteractionMode() != vtkMRMLInteractionNode::ViewTransform)
  {
    this->selectEffect("");
  }
}

//-----------------------------------------------------------------------------
bool vtkSegmentEditorAbstractEffect::segmentationDisplayableInView(vtkMRMLAbstractViewNode* viewNode)
{
  if (!viewNode)
  {
    vtkWarningMacro("failed. Invalid viewNode.");
    return false;
  }

  vtkMRMLSegmentEditorNode* parameterSetNode = this->parameterSetNode();
  if (!parameterSetNode)
  {
    return false;
  }

  vtkMRMLSegmentationNode* segmentationNode = parameterSetNode->GetSegmentationNode();
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
std::string vtkSegmentEditorAbstractEffect::getAttributeName(const std::string& name)
{
  return this->name() + "." + name;
}

//-----------------------------------------------------------------------------
vtkMouseCursor vtkSegmentEditorAbstractEffect::getMouseCursor() const { return m_SavedCursor; }

//-----------------------------------------------------------------------------
void vtkSegmentEditorAbstractEffect::setMouseCursor(vtkSmartPointer<vtkMouseCursor> cursor)
{
  if (m_SavedCursor == cursor)
  {
    return;
  }
  m_SavedCursor = cursor;
  InvokeEvent(MouseCursorChangedEvent);
}