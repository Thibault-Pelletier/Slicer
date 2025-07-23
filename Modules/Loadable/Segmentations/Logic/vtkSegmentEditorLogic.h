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
class SegmentEditorEventObservation;
class vtkRenderWindow;
class vtkSegmentEditorEventCallbackCommand;

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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum Events
  {
    CurrentSegmentIDChangedEvent, // (const std::string&);
    SourceVolumeNodeChangedEvent, // (vtkMRMLVolumeNode*);
    SegmentationNodeChangedEvent, // (vtkMRMLSegmentationNode*);
    EffectListChangedEvent, //
  };

  /// Get the segment editor parameter set node
  vtkMRMLSegmentEditorNode* mrmlSegmentEditorNode() const;

  /// Get currently selected segmentation MRML node
  vtkMRMLNode* segmentationNode() const;
  /// Get ID of currently selected segmentation node
  std::string segmentationNodeID() const;
  /// Get currently selected source volume MRML node
  vtkMRMLNode* sourceVolumeNode() const;
  /// Get ID of currently selected source volume node
  std::string sourceVolumeNodeID() const;
  /// Get segment ID of selected segment
  std::string currentSegmentID() const;

  /// Return active effect if selected, nullptr otherwise
  /// \sa m_ActiveEffect, setActiveEffect()
  vtkSegmentEditorAbstractEffect* activeEffect() const;
  /// Set active effect
  /// \sa m_ActiveEffect, activeEffect()
  void setActiveEffect(vtkSegmentEditorAbstractEffect* effect);

  /// Get an effect object by name
  /// \return The effect instance if exists, nullptr otherwise
  vtkSegmentEditorAbstractEffect* effectByName(std::string name);

  /// Get list of all registered effect names that can be displayed in the widget.
  std::vector<std::string> availableEffectNames();

  /// Request displaying effects in the specified order.
  /// Effects that are not listed will be hidden if \sa unorderedEffectsVisible is false.
  void setEffectNameOrder(const std::vector<std::string>& effectNames);

  /// Get requested order of effects.
  /// Actually displayed effects can be retrieved by using \sa effectCount and \sa effectByIndex.
  /// \return List of effect names to be shown in the widget.
  std::vector<std::string> effectNameOrder() const;

  /// Show/hide effect names that are not listed in \sa effectNameOrder().
  /// True by default to make effects registered by extensions show up by default.
  /// This can be used to simplify the editor widget to show only a limited number of effects.
  void setUnorderedEffectsVisible(bool visible);

  /// Get visibility status of effect names that are not listed in effectNameOrder().
  /// \sa setEffectNameOrder
  bool unorderedEffectsVisible() const;

  /// Get number of displayed effects
  /// \return Number of effects shown in the widget
  int effectCount();

  /// Get n-th effect shown in the widget. n>=0 and n<effectCount().
  /// \return The effect instance if exists, nullptr otherwise
  vtkSegmentEditorAbstractEffect* effectByIndex(int index);

  //{@
  /// Create observations between slice view interactor and the widget.
  /// The captured events are propagated to the active effect if any.
  /// NOTE: This method should be called from the enter function of the
  ///   embedding module widget so that the events are correctly processed.
  void setupViewObservations(std::vector<vtkRenderWindow*> sliceRenderWindows,
                             std::vector<vtkMRMLAbstractViewNode*> sliceNodes,
                             std::vector<vtkRenderWindow*> threeDViewRenderWindows,
                             std::vector<vtkMRMLAbstractViewNode*> threeDViewNodes);
  void setupViewObservations(std::vector<vtkRenderWindow*> viewRenderWindows, std::vector<vtkMRMLAbstractViewNode*> viewNodes);
  //@}

  /// Remove observations
  /// NOTE: This method should be called from the exit function of the
  ///   embedding module widget so that events are not processed unnecessarily.
  void removeViewObservations();

  /// Show/hide the segmentation node selector widget.
  bool segmentationNodeSelectorVisible() const;
  /// Show/hide the source volume node selector widget.
  bool sourceVolumeNodeSelectorVisible() const;
  /// If autoShowSourceVolumeNode is enabled then source volume is automatically
  /// displayed in slice views when a new source volume is selected or layout is changed.
  /// Enabled by default.
  bool autoShowSourceVolumeNode() const;

  /// Show/hide the masking section
  bool maskingSectionVisible() const;

  /// Show/hide the specify geometry button
  bool specifyGeometryButtonVisible() const;

  /// Show/hide the 3D button
  bool show3DButtonVisible() const;

  /// Show/hide the add/remove segment buttons
  bool addRemoveSegmentButtonsVisible() const;

  /// Show/hide the switch to Segmentations module button
  bool switchToSegmentationsButtonVisible() const;
  /// Undo/redo enabled.
  bool undoEnabled() const;
  /// Get maximum number of saved undo/redo states.
  int maximumNumberOfUndoStates() const;
  /// Get whether widget is read-only
  bool readOnly() const;

  /// Get number of columns being used by the effects.
  /// \return Number of columns being used for effects.
  int effectColumnCount() const;

  /// Get current interaction node.
  /// \sa SetInteractionNode()
  vtkMRMLInteractionNode* interactionNode() const;

  /// Set settings key that stores defaultTerminologyEntry. If set to empty string then
  /// the defaultTerminologyEntry is not saved to/loaded from application settings.
  /// By default it is set to "Segmentations/DefaultTerminologyEntry".
  /// If settings key is changed then the current default terminology entry is not written
  /// into application settings (as it would overwrite its current value in the settings,
  /// which is usually not the expected behavior).
  /// This default can be overridden by default terminology entry specified for the
  /// segmentation node using vtkSlicerTerminologiesModuleLogic::SetDefaultTerminologyEntry()
  void setDefaultTerminologyEntrySettingsKey(const std::string& terminologyEntrySettingsKey);

  /// Get settings key that stores defaultTerminologyEntry.
  /// \sa setDefaultTerminologyEntrySettingsKey
  std::string defaultTerminologyEntrySettingsKey() const;

  /// Set terminology entry string that is used for the first segment by default.
  /// The value is also written to application settings, if defaultTerminologyEntrySettingsKey is not empty.
  void setDefaultTerminologyEntry(const std::string& terminologyEntry);

  /// Get terminology entry string that is used for the first segment by default.
  /// The value is read from application settings, if defaultTerminologyEntrySettingsKey is not empty.
  std::string defaultTerminologyEntry();

  /// Returns true if automatic jump to current segment is enabled.
  bool jumpToSelectedSegmentEnabled() const;

  /// Set the MRML \a scene associated with the widget
  void setMRMLScene(vtkMRMLScene* newScene);

  /// Set the segment editor parameter set node
  void setMRMLSegmentEditorNode(vtkMRMLSegmentEditorNode* newSegmentEditorNode);

  /// Set segmentation MRML node
  void setSegmentationNode(vtkMRMLNode* node);

  /// Set segmentation MRML node by its ID
  void setSegmentationNodeID(const std::string& nodeID);

  /// Set source volume MRML node.
  /// If source volume has multiple scalar components
  /// then only the first scalar component is used.
  void setSourceVolumeNode(vtkMRMLNode* node);

  /// Set source volume MRML node by its ID
  void setSourceVolumeNodeID(const std::string& nodeID);

  /// Set selected segment by its ID
  void setCurrentSegmentID(const std::string segmentID);

  /// Set active effect by name
  void setActiveEffectByName(std::string effectName);

  /// Save current segmentation before performing an edit operation
  /// to allow reverting to the current state by using undo
  void saveStateForUndo();

  /// Update modifierLabelmap, maskLabelmap, or alignedSourceVolumeNode
  void updateVolume(void* volumePtr, bool& success);

  /// Show/hide the segmentation node selector widget.
  void setSegmentationNodeSelectorVisible(bool);

  /// Show/hide the source volume node selector widget.
  void setSourceVolumeNodeSelectorVisible(bool);

  /// If autoShowSourceVolumeNode is enabled then source volume is automatically
  /// displayed in slice views when a new source volume is selected or layout is changed.
  /// Enabled by default.
  void setAutoShowSourceVolumeNode(bool);

  void createAndSetBlankSourceVolumeIfNeeded();

  /// Set masking section visible
  /// If set to false then masking section always remains hidden.
  void setMaskingSectionVisible(bool);

  /// Show/hide the specify geometry button
  /// If set to false then the button is always hidden.
  void setSpecifyGeometryButtonVisible(bool);

  /// Show/hide the 3D button
  /// If set to false then the button is always hidden.
  void setShow3DButtonVisible(bool);

  /// Show/hide the add/remove segment buttons
  /// If set to false then the buttons are always hidden.
  void setAddRemoveSegmentButtonsVisible(bool);

  /// Show/hide the switch to Segmentations module button
  void setSwitchToSegmentationsButtonVisible(bool);

  /// Undo/redo enabled.
  void setUndoEnabled(bool);

  /// Set maximum number of saved undo/redo states.
  void setMaximumNumberOfUndoStates(int);

  /// Set whether the widget is read-only
  void setReadOnly(bool aReadOnly);

  /// Enable/disable masking using source volume intensity
  void toggleSourceVolumeIntensityMaskEnabled();

  /// Restores previous saved state of the segmentation
  void undo();

  /// Restores next saved state of the segmentation
  void redo();

  /// Convenience method to turn off lightbox view in all slice viewers.
  /// Segment editor is not compatible with lightbox view layouts.
  /// Returns true if there were lightbox views.
  bool turnOffLightboxes(vtkCollection* sliceLogics);

  /// Unselect labelmap layer in all slice views in the active layout
  void hideLabelLayer(vtkCollection* sliceLogics);

  /// Show source volume in slice views by hiding foreground and label volumes.
  /// \param forceShowInBackground If set to false then views will only change
  ///   if source volume is not selected as either foreground or background volume.
  /// \param fitSlice Reset field of view to include all volumes.
  void showSourceVolumeInSliceViewers(vtkCollection* sliceLogics, bool forceShowInBackground = false, bool fitSlice = false);

  /// Rotate slice views to be aligned with segmentation node's internal
  /// labelmap representation axes.
  void rotateSliceViewsToSegmentation();

  /// Set node used to notify active effect about interaction node changes.
  /// \sa interactionNode()
  void setInteractionNode(vtkMRMLInteractionNode* interactionNode);

  /// Select the segment above the currently selected one in the table (skipping segments that are not visible)
  void selectPreviousSegment();

  /// Select the segment below the currently selected one in the table (skipping segments that are not visible)
  void selectNextSegment();

  /// Select the segment offset from the currently selected one in the table (skipping segments that are not visible)
  /// Positive offset will move down the table
  /// Negative offset will move up the table
  void selectSegmentAtOffset(int offset);

  /// Jump position of all slice views to show the segment's center.
  /// Segment's center is determined as the center of bounding box.
  void jumpSlices();

  /// Enables automatic jumping to current segment when selection is changed.
  void setJumpToSelectedSegmentEnabled(bool enable);

  vtkMRMLScene* mrmlScene() const;

  bool hasSelectedSegmentID(int offset = 0);
  std::string getSelectedSegmentID(int offset = 0);

  std::vector<std::string> getVisibleSegmentIDs();
  bool isSegmentIDVisible(const std::string& segmentID);
  std::vector<std::string> getSegmentIDs();


protected:
  /// Handles changing of current segmentation MRML node
  void onSegmentationNodeChanged(vtkMRMLNode* node);
  /// Handles changing of the current source volume MRML node
  void onSourceVolumeNodeChanged(vtkMRMLNode* node);

  /// Handles mouse mode changes (view / place markups)
  void onInteractionNodeModified();

  /// Effect selection shortcut is activated.
  /// 0 means deselect active effect.
  /// -1 toggles between no effect/last active effect.
  void onSelectEffectShortcut();

  /// Segment selection shortcut is activated
  void onSelectSegmentShortcut();

  /// Add empty segment
  void onAddSegment();

  /// Remove selected segment
  void onRemoveSegment();

  /// Edit segmentation properties in Segmentations module
  void onSwitchToSegmentations();

  /// Create/remove closed surface model for the segmentation that is automatically updated when editing
  void onCreateSurfaceToggled(bool on);

  /// Called if a segment or representation is added or removed
  void onSegmentAddedRemoved();

  /// Called if source volume image data is changed
  void onSourceVolumeImageDataModified();

  /// Handle layout changes
  void onLayoutChanged(int layoutIndex);

  /// Handle display node view ID changes
  void onSegmentationDisplayModified();

  /// Changed selected editable segment area
  void onMaskModeChanged(int);

  /// Enable/disable threshold when checkbox is toggled
  void onSourceVolumeIntensityMaskChecked(bool checked);

  /// Handles threshold values changed event
  void onSourceVolumeIntensityMaskRangeChanged(double low, double high);

  /// Changed selected overwritable segments
  void onOverwriteModeChanged(int);

  /// Clean up when scene is closed
  void onMRMLSceneEndCloseEvent();

  /// Updates needed after batch processing is done
  void onMRMLSceneEndBatchProcessEvent();

  /// Set default parameters in parameter set node (after setting or closing scene)
  void initializeParameterSetNode();

  /// Update undo/redo button states
  bool isUndoEnabled() const;
  bool isRedoEnabled() const;

  /// Update GUI if segmentation history is changed (e.g., undo/redo button states)
  void onSegmentationHistoryChanged();

  /// Switch to Segmentations module and jump to Import/Export section
  void onImportExportActionClicked();

  /// Open Export to files dialog
  void onExportToFilesActionClicked();

  /// Export segment color and terminology information to a new color table
  void onExportToColorTableActionClicked();

  /// Update masking section on the UI
  void updateMaskingSection();

  /// Show slice rotation warning button if slice views are rotated, hide otherwise
  void updateSliceRotateWarningButtonVisibility();

  /// Show segmentation geometry dialog to specify labelmap geometry
  void showSegmentationGeometryDialog();

protected:
  /// Callback function invoked when interaction happens
  static void processEvents(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  void updateWidgetFromSegmentationNode();
  void updateWidgetFromSourceVolumeNode();
  void updateEffectsSectionFromMRML();

  /// Switches the source representation to binary labelmap. If the source representation
  /// cannot be set to binary labelmap (e.g., the user does not allow it) then false is returned.
  bool setSourceRepresentationToBinaryLabelmap();

  bool isSegmentationNodeValid() const { return SegmentationNode != nullptr; }

  bool isSourceRepresentationBinaryLabelMap() const;

private:
  vtkSmartPointer<vtkSegmentEditorEventCallbackCommand> createViewInteractionCallbackCommand(vtkRenderWindow* renderWindow, vtkMRMLAbstractViewNode* viewNode);
  SegmentEditorEventObservation createSlicePoseModifiedEventObservation(vtkSmartPointer<vtkSegmentEditorEventCallbackCommand> interactionCallbackCommand);
  SegmentEditorEventObservation createViewNodeModifiedEventObservation(vtkSmartPointer<vtkSegmentEditorEventCallbackCommand> interactionCallbackCommand);
  SegmentEditorEventObservation createInteractorEventObservation(vtkSmartPointer<vtkSegmentEditorEventCallbackCommand> interactionCallbackCommand);

  void refreshObservedViewNodeIds();

  /// Simple mechanism to let the effects know that default modifier labelmap has changed
  void notifyEffectsOfReferenceGeometryChange(const std::string& geometry);

  /// Simple mechanism to let the effects know that source volume has changed
  void notifyEffectsOfSourceVolumeNodeChange();

  /// Simple mechanism to let the effects know that layout has changed
  void notifyEffectsOfLayoutChange();

  /// Select first segment in table view
  void selectFirstSegment();

  /// Enable or disable effects and their options based on input selection
  void updateEffectsEnabledFromMRML();

  /// Set cursor for effect. If effect is nullptr then the cursor is reset to default.
  void setEffectCursor(vtkSegmentEditorAbstractEffect* effect);

  /// Updates default modifier labelmap based on reference geometry (to set origin, spacing, and directions)
  /// and existing segments (to set extents). If reference geometry conversion parameter is empty
  /// then existing segments are used for determining origin, spacing, and directions and the resulting
  /// geometry is written to reference geometry conversion parameter.
  bool resetModifierLabelmapToDefault();

  /// Updates selected segment labelmap in a geometry aligned with default modifierLabelmap.
  bool updateSelectedSegmentLabelmap();

  /// Updates a resampled source volume in a geometry aligned with default modifierLabelmap.
  bool updateAlignedSourceVolume();

  /// Updates mask labelmap.
  /// Geometry of mask will be the same as current modifierLabelmap.
  /// This mask only considers segment-based regions (and ignores masking based on
  /// source volume intensity).
  bool updateMaskLabelmap();

  bool updateReferenceGeometryImage();

  static std::string getReferenceImageGeometryFromSegmentation(vtkSegmentation* segmentation);
  std::string referenceImageGeometry();

  bool segmentationDisplayableInView(vtkMRMLAbstractViewNode* viewNode);

  /// Return segmentation node's internal labelmap IJK to renderer world coordinate transform.
  /// If cannot be retrieved (segmentation is not defined, non-linearly transformed, etc.)
  /// then false is returned;
  bool getSegmentationIJKToRAS(vtkMatrix4x4* ijkToRas);

  /// Segment editor parameter set node containing all selections and working images
  vtkWeakPointer<vtkMRMLSegmentEditorNode> ParameterSetNode;

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
  vtkOrientedImageData* AlignedSourceVolume;
  /// Modifier labelmap that is kept in memory to avoid memory reallocations on each editing operation.
  /// When update of this labelmap is requested its geometry is reset and its content is cleared.
  vtkOrientedImageData* ModifierLabelmap;
  vtkOrientedImageData* SelectedSegmentLabelmap;
  vtkOrientedImageData* MaskLabelmap;
  /// Image that contains reference geometry. Scalars are not allocated.
  vtkOrientedImageData* ReferenceGeometryImage;

  /// Input data that is used for computing AlignedSourceVolume.
  /// It is stored so that it can be determined that the source volume has to be updated
  vtkMRMLVolumeNode* AlignedSourceVolumeUpdateSourceVolumeNode;
  vtkMRMLTransformNode* AlignedSourceVolumeUpdateSourceVolumeNodeTransform;
  vtkMRMLTransformNode* AlignedSourceVolumeUpdateSegmentationNodeTransform;

  int MaskModeComboBoxFixedItemsCount;

  /// If reference geometry changes compared to this value then we notify effects and
  /// set this value to the current value. This allows notifying effects when there is a change.
  std::string LastNotifiedReferenceImageGeometry;

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
};

#endif
