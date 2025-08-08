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

#ifndef __vtkSegmentEditorLogic_h
#define __vtkSegmentEditorLogic_h

#include "vtkSlicerSegmentationsModuleLogicExport.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <set>
#include <vtkObject.h>
#include <vtkVariant.h>
#include <vtkWeakPointer.h>
#include <vtkSmartPointer.h>
#include <vtkCommand.h>

class vtkMRMLNode;
class vtkMRMLScene;
class vtkMRMLInteractionNode;
class vtkMRMLSegmentationNode;
class vtkMRMLSegmentEditorNode;
class vtkMRMLVolumeNode;
class vtkSegmentEditorAbstractEffect;
class vtkCollection;
class vtkSegmentationHistory;
class vtkOrientedImageData;
class vtkMRMLTransformNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLAbstractViewNode;
class vtkMatrix4x4;
class vtkSegmentation;
class vtkRenderWindow;
class vtkSegmentEditorEventCallbackCommand;
class vtkMRMLSliceNode;
class vtkMRMLViewNode;
struct SegmentEditorEventObservation;

/// \brief Qt widget for editing a segment from a segmentation using Editor effects.
///
/// Widget for editing segmentations that can be reused in any module.
///
/// IMPORTANT: The embedding module is responsible for setting the MRML scene and the
///   management of the \sa vtkMRMLSegmentEditorNode parameter set node.
///   The empty parameter set node should be set before the MRML scene, so that the
///   default selections can be stored in the parameter set node. Also, re-creation
///   of the parameter set node needs to be handled after scene close, and usage of
///   occasional existing parameter set nodes after scene import.
///
class VTK_SLICER_SEGMENTATIONS_LOGIC_EXPORT vtkSegmentEditorLogic : public vtkObject
{
public:
  static vtkSegmentEditorLogic* New();
  vtkTypeMacro(vtkSegmentEditorLogic, vtkObject);

  enum Events
  {
    SegmentationNodeChangedEvent = vtkCommand::UserEvent + 1, // //FIXME: rename to segment editor node
    SegmentationHistoryChangedEvent,                          //
    UpdateSliceRotateWarningEvent,                            //
    PauseRenderEvent,                                         //
    ResumeRenderEvent,                                        //
    EffectParameterModified                                   // FIXME: forward ParameterSetNode, vtkMRMLSegmentEditorNode::EffectParameterModified
  };

  /// Get the segment editor parameter set node
  vtkMRMLSegmentEditorNode* GetSegmentEditorNode() const;

  /// Get currently selected segmentation MRML node
  vtkMRMLSegmentationNode* GetSegmentationNode() const;

  /// Get ID of currently selected segmentation node
  std::string GetSegmentationNodeID() const;
  /// Get currently selected source volume MRML node
  vtkMRMLScalarVolumeNode* GetSourceVolumeNode() const;
  /// Get ID of currently selected source volume node
  std::string GetSourceVolumeNodeID() const;
  /// Get segment ID of selected segment
  std::string GetCurrentSegmentID();

  bool IsSegmentIDValid(const std::string& segmentId);

  bool CanAddSegments();

  bool CanRemoveSegments();

  bool IsLocked() const;

  vtkSegmentation* GetSegmentation();

  /// Return active effect if selected, nullptr otherwise
  /// \sa m_ActiveEffect, setActiveEffect()
  vtkSegmentEditorAbstractEffect* GetActiveEffect() const;
  /// Set active effect
  /// \sa m_ActiveEffect, activeEffect()
  void SetActiveEffect(vtkSegmentEditorAbstractEffect* effect);

  void SelectEffect(const std::string& effect)
  {
    // FIXME
    throw std::runtime_error("NOT IMPLEMENTED");
  }

  //{@
  /// Create observations between slice view interactor and the widget.
  /// The captured events are propagated to the active effect if any.
  /// NOTE: This method should be called from the enter function of the
  ///   embedding module widget so that the events are correctly processed.
  void SetupViewObservations();
  void SetupViewObservations(const std::vector<vtkMRMLAbstractViewNode*>& viewNodes);
  //@}

  /// Remove observations
  /// NOTE: This method should be called from the exit function of the
  ///   embedding module widget so that events are not processed unnecessarily.
  void RemoveViewObservations();

  /// Get maximum number of saved undo/redo states.
  int GetMaximumNumberOfUndoStates() const;

  /// Get whether widget is read-only
  bool IsReadOnly() const;

  /// Get current interaction node.
  /// \sa SetInteractionNode()
  vtkMRMLInteractionNode* GetInteractionNode() const;

  /// Set the MRML \a scene associated with the widget
  void SetScene(vtkMRMLScene* newScene);

  /// Set the segment editor parameter set node
  void SetSegmentEditorNode(vtkMRMLSegmentEditorNode* newSegmentEditorNode);

  /// Set segmentation MRML node
  void SetSegmentationNode(vtkMRMLNode* node);

  /// Set segmentation MRML node by its ID
  void SetSegmentationNodeID(const std::string& nodeID);

  /// Set source volume MRML node.
  /// If source volume has multiple scalar components
  /// then only the first scalar component is used.
  void SetSourceVolumeNode(vtkMRMLNode* node);

  /// Set source volume MRML node by its ID
  void SetSourceVolumeNodeID(const std::string& nodeID);

  /// Set selected segment by its ID
  void SetCurrentSegmentID(const std::string segmentID);

  /// Save current segmentation before performing an edit operation
  /// to allow reverting to the current state by using undo
  void SaveStateForUndo();

  /// Update modifierLabelmap, maskLabelmap, or alignedSourceVolumeNode
  void UpdateVolume(void* volumePtr, bool& success);

  void CreateAndSetBlankSourceVolumeIfNeeded();

  /// Undo/redo enabled.
  void ClearUndoState();

  /// Set maximum number of saved undo/redo states.
  void SetMaximumNumberOfUndoStates(int);

  /// Set whether the widget is read-only
  void SetReadOnly(bool aReadOnly);

  /// Enable/disable masking using source volume intensity
  void ToggleSourceVolumeIntensityMaskEnabled();

  /// Restores previous saved state of the segmentation
  void Undo();

  /// Restores next saved state of the segmentation
  void Redo();

  /// Convenience method to turn off lightbox view in all slice viewers.
  /// Segment editor is not compatible with lightbox view layouts.
  /// Returns true if there were lightbox views.
  bool TurnOffLightboxes();

  /// Unselect labelmap layer in all slice views in the active layout
  void HideLabelLayer();

  /// Show source volume in slice views by hiding foreground and label volumes.
  /// \param forceShowInBackground If set to false then views will only change
  ///   if source volume is not selected as either foreground or background volume.
  /// \param fitSlice Reset field of view to include all volumes.
  void ShowSourceVolumeInSliceViewers(bool forceShowInBackground = false, bool fitSlice = false);

  /// Set node used to notify active effect about interaction node changes.
  /// \sa interactionNode()
  void SetInteractionNode(vtkMRMLInteractionNode* interactionNode);

  /// Select the segment above the currently selected one in the table (skipping segments that are not visible)
  void SelectPreviousSegment();

  /// Select the segment below the currently selected one in the table (skipping segments that are not visible)
  void SelectNextSegment();

  void SelectFirstSegment();

  /// Select the segment offset from the currently selected one in the table (skipping segments that are not visible)
  /// Positive offset will move down the table
  /// Negative offset will move up the table
  void SelectSegmentAtOffset(int offset);

  vtkMRMLScene* GetScene() const;

  bool HasSelectedSegmentID(int offset = 0);
  std::string GetSelectedSegmentID(int offset = 0);

  std::vector<std::string> GetVisibleSegmentIDs();
  bool IsSegmentIDVisible(const std::string& segmentID);
  std::vector<std::string> GetSegmentIDs();

  /// Handles mouse mode changes (view / place markups)
  static void OnInteractionNodeModified(vtkObject* caller, unsigned long eid, void* clientData, void* callData);
  void OnInteractionNodeModified();

  /// Handles changing of current segmentation MRML node
  void OnSegmentationNodeChanged(vtkMRMLNode* node);
  /// Handles changing of the current source volume MRML node
  void OnSourceVolumeNodeChanged(vtkMRMLNode* node);

  /// Add empty segment
  void OnAddSegment(std::string emptySegmentName, int firstVisibleStatus);

  /// Remove selected segment
  void OnRemoveSegment();

  /// Create/remove closed surface model for the segmentation that is automatically updated when editing
  void OnCreateSurfaceToggled(bool on);

  /// Handle display node view ID changes
  void OnSegmentationDisplayModified(); // FIXME: To rename

  /// Enable/disable threshold when checkbox is toggled
  void OnSourceVolumeIntensityMaskChecked(bool checked);

  /// Handles threshold values changed event
  void OnSourceVolumeIntensityMaskRangeChanged(double low, double high);

  /// Update undo/redo button states
  bool IsUndoEnabled() const;
  bool IsRedoEnabled() const;

  /// Export segment color and terminology information to a new color table
  void OnExportToColorTableActionClicked();

  /// Callback function invoked when interaction happens
  static void ProcessEvents(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  /// Switches the source representation to binary labelmap.
  bool SetSourceRepresentationToBinaryLabelmapWithoutConfirmation();

  bool IsSegmentationNodeValid() const { return SegmentationNode != nullptr; }

  bool CanTriviallyConvertSourceRepresentationToBinaryLabelMap();

  bool TrivialSetSourceRepresentationToBinaryLabelmap();

  bool ConvertSourceRepresentationToBinaryLabelmap();

  /// \brief Trigger the PauseRenderEvent
  void PauseRender();

  /// \brief Trigger the ResumeRenderEvent
  void ResumeRender();

  void RefreshObservedViewNodeIds();

  /// Updates default modifier labelmap based on reference geometry (to set origin, spacing, and directions)
  /// and existing segments (to set extents). If reference geometry conversion parameter is empty
  /// then existing segments are used for determining origin, spacing, and directions and the resulting
  /// geometry is written to reference geometry conversion parameter.
  bool ResetModifierLabelmapToDefault();

  /// Updates selected segment labelmap in a geometry aligned with default modifierLabelmap.
  bool UpdateSelectedSegmentLabelmap();

  /// Updates a resampled source volume in a geometry aligned with default modifierLabelmap.
  bool UpdateAlignedSourceVolume();

  /// Updates mask labelmap.
  /// Geometry of mask will be the same as current modifierLabelmap.
  /// This mask only considers segment-based regions (and ignores masking based on
  /// source volume intensity).
  bool UpdateMaskLabelmap();

  bool UpdateReferenceGeometryImage();

  static std::string GetReferenceImageGeometryStringFromSegmentation(vtkSegmentation* segmentation);
  std::string GetReferenceImageGeometryString();

  bool IsSegmentationDisplayableInView(vtkMRMLAbstractViewNode* viewNode);

  /// Return segmentation node's internal labelmap IJK to renderer world coordinate transform.
  /// If cannot be retrieved (segmentation is not defined, non-linearly transformed, etc.)
  /// then false is returned;
  bool GetSegmentationIJKToRAS(vtkMatrix4x4* ijkToRas);

  vtkOrientedImageData* GetAlignedSourceVolume() const;
  vtkOrientedImageData* GetModifierLabelmap() const;
  vtkOrientedImageData* GetMaskLabelmap() const;
  vtkOrientedImageData* GetSelectedSegmentLabelmap() const;
  vtkOrientedImageData* GetReferenceGeometryImage() const;
  vtkSmartPointer<vtkSegmentationHistory> GetSegmentationHistory() const;
  bool AreViewsObserved() const;
  void SetLocked(bool isLocked);

protected:
  vtkSegmentEditorLogic();
  ~vtkSegmentEditorLogic() override;

private:
  vtkSmartPointer<vtkSegmentEditorEventCallbackCommand> CreateViewInteractionCallbackCommand(vtkRenderWindow* renderWindow, vtkMRMLAbstractViewNode* viewNode);
  SegmentEditorEventObservation CreateSlicePoseModifiedEventObservation(const vtkSmartPointer<vtkSegmentEditorEventCallbackCommand>& interactionCallbackCommand);
  SegmentEditorEventObservation CreateViewNodeModifiedEventObservation(const vtkSmartPointer<vtkSegmentEditorEventCallbackCommand>& interactionCallbackCommand);
  SegmentEditorEventObservation CreateInteractorEventObservation(const vtkSmartPointer<vtkSegmentEditorEventCallbackCommand>& interactionCallbackCommand);

  /// Segment editor parameter set node containing all selections and working images
  vtkWeakPointer<vtkMRMLSegmentEditorNode> SegmentEditorNode;

  vtkWeakPointer<vtkMRMLSegmentationNode> SegmentationNode;
  vtkSmartPointer<vtkSegmentationHistory> SegmentationHistory;

  vtkWeakPointer<vtkMRMLScalarVolumeNode> SourceVolumeNode;

  // Observe InteractionNode to detect when mouse mode is changed
  vtkWeakPointer<vtkMRMLInteractionNode> InteractionNode;

  /// Lock widget to make segmentation read-only.
  // In the future locked state may be read from the Segmentation node.
  bool Locked;

  /// Ordering of effects
  std::vector<std::string> EffectNameOrder;
  bool UnorderedEffectsVisible;
  int EffectColumnCount;

  bool MaskingSectionVisible{ true };
  bool SpecifyGeometryButtonVisible{ true };

  /// List of registered effect instances
  std::vector<vtkSegmentEditorAbstractEffect*> RegisteredEffects;

  /// Active effect
  vtkSegmentEditorAbstractEffect* ActiveEffect;
  /// Last active effect
  /// Stored to allow quick toggling between no effect/last active effect.
  vtkSegmentEditorAbstractEffect* LastActiveEffect;

  /// Structure containing necessary objects for each slice and 3D view handling interactions
  std::vector<SegmentEditorEventObservation> EventObservations;

  /// List of view node IDs where custom cursor is set
  std::set<std::string> CustomCursorInViewNodeIDs;

  /// Indicates if views and layouts are observed
  /// (essentially, the widget is active).
  bool ViewsObserved;

  /// List of view node IDs in display nodes, which were specified when views observation was set up.
  /// If node IDs change (segmentation node is shown/hidden in a specific view) then view observations has to be refreshed.
  std::map<std::string, std::vector<std::string>> ObservedViewNodeIDs; // <SegmentationDisplayNodeID, ViewNodeIDs>

  bool AutoShowSourceVolumeNode;

  /// These volumes are owned by this widget and a pointer is given to each effect
  /// so that they can access and modify it
  vtkSmartPointer<vtkOrientedImageData> AlignedSourceVolume;
  /// Modifier labelmap that is kept in memory to avoid memory reallocations on each editing operation.
  /// When update of this labelmap is requested its geometry is reset and its content is cleared.
  vtkSmartPointer<vtkOrientedImageData> ModifierLabelmap;
  vtkSmartPointer<vtkOrientedImageData> SelectedSegmentLabelmap;
  vtkSmartPointer<vtkOrientedImageData> MaskLabelmap;
  /// Image that contains reference geometry. Scalars are not allocated.
  vtkSmartPointer<vtkOrientedImageData> ReferenceGeometryImage;

  /// Input data that is used for computing AlignedSourceVolume.
  /// It is stored so that it can be determined that the source volume has to be updated
  vtkMRMLVolumeNode* AlignedSourceVolumeUpdateSourceVolumeNode;
  vtkMRMLTransformNode* AlignedSourceVolumeUpdateSourceVolumeNodeTransform;
  vtkMRMLTransformNode* AlignedSourceVolumeUpdateSegmentationNodeTransform;

  // When segmentation node selector is visible then rotate warning is displayed in that row.
  // However, if the node selector is hidden then the rotation warning button would take
  // and entire row.
  // To prevent this, rotation warning button is displayed in the add/remove/etc. segment
  // button row when segmentation node selector is hidden.
  // Qt does not have an API to check if a widget is in a layout, therefore we store this
  // information in this flag.
  bool RotateWarningInNodeSelectorLayout;

  std::string DefaultTerminologyEntrySettingsKey;
  std::string DefaultTerminologyEntry;

  vtkMRMLScene* MRMLScene;

  unsigned long InteractionNodeObs;
};

#endif
