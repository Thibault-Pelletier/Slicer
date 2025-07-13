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

#ifndef __vtkSegmentEditorAbstractEffect_h
#define __vtkSegmentEditorAbstractEffect_h

// Segmentations Editor Effects includes
#include "vtkSlicerSegmentationsModuleLogicExport.h"

// Qt includes
#include <QCursor>
#include <QIcon>
#include <array>
#include <vtkVector3d>
#include <vector>
#include <string>

// VTK includes
#include "vtkWeakPointer.h"

class vtkSegmentEditorAbstractEffectPrivate;

class vtkActor2D;
class vtkMRMLInteractionNode;
class vtkMRMLNode;
class vtkMRMLScene;
class vtkMRMLSegmentEditorNode;
class vtkMRMLAbstractViewNode;
class vtkMRMLSegmentationNode;
class vtkMRMLTransformNode;
class vtkSegment;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkOrientedImageData;
class vtkProp3D;
class qMRMLWidget;
class qMRMLSliceWidget;
class QColor;
class QFormLayout;
class QFrame;
class QLayout;

/// \brief Abstract class for segment editor effects
class VTK_SLICER_SEGMENTATIONS_LOGIC_EXPORT vtkSegmentEditorAbstractEffect : public QObject
{
  VTK_OBJECT

public:
  typedef QObject Superclass;
  vtkSegmentEditorAbstractEffect(QObject* parent = nullptr);
  ~vtkSegmentEditorAbstractEffect() override;

  // API: Methods that are to be reimplemented in the effect subclasses
public:
  enum ModificationMode
  {
    ModificationModeSet,
    ModificationModeAdd,
    ModificationModeRemove,
    ModificationModeRemoveAll
  };

  enum ConfirmationResult
  {
    NotConfirmed,
    ConfirmedWithoutDialog,
    ConfirmedWithDialog,
  };

  /// Get icon for effect to be displayed in segment editor
  virtual QIcon icon() { return QIcon(); };

  /// Get help text for effect to be displayed in the help box
  virtual const std::string helpText() const { return std::string(); };

  /// Clone editor effect. Override to return a new instance of the effect sub-class
  virtual vtkSegmentEditorAbstractEffect* clone() = 0;

  /// Perform actions to activate the effect (show options frame, etc.)
  /// NOTE: Base class implementation needs to be called BEFORE the effect-specific implementation
  virtual void activate();

  /// Perform actions to deactivate the effect (hide options frame, destroy actors, etc.)
  /// NOTE: Base class implementation needs to be called BEFORE the effect-specific implementation
  virtual void deactivate();

  /// Returns true if the effect is currently active (activated and has not deactivated since then)
  virtual bool active();

  virtual void modifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap,
                                               ModificationMode modificationMode,
                                               const int modificationExtent[6],
                                               bool bypassMasking = false);
  virtual void modifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap, ModificationMode modificationMode, bool bypassMasking = false);
  virtual void modifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap, ModificationMode modificationMode, std::vector<int> extent, bool bypassMasking = false);
  virtual void modifySegmentByLabelmap(vtkMRMLSegmentationNode* segmentationNode,
                                       const char* segmentID,
                                       vtkOrientedImageData* modifierLabelmap,
                                       ModificationMode modificationMode,
                                       bool bypassMasking = false);
  virtual void modifySegmentByLabelmap(vtkMRMLSegmentationNode* segmentationNode,
                                       const char* segmentID,
                                       vtkOrientedImageData* modifierLabelmap,
                                       ModificationMode modificationMode,
                                       const int modificationExtent[6],
                                       bool bypassMasking = false);

  /// Apply mask image on an input image.
  /// This method is kept here for backward compatibility only and will be removed in the future.
  /// Use vtkOrientedImageDataResample::ApplyImageMask method instead.
  static void applyImageMask(vtkOrientedImageData* input, vtkOrientedImageData* mask, double fillValue, bool notMask = false);

  /// Create options frame widgets, make connections, and add them to the main options frame using \sa addOptionsWidget
  /// NOTE: Base class implementation needs to be called BEFORE the effect-specific implementation
  virtual void setupOptionsFrame() {};

  /// Create a cursor customized for the given effect, potentially for each view
  virtual QCursor createCursor(qMRMLWidget* viewWidget);

  /// Callback function invoked when interaction happens
  /// \param callerInteractor Interactor object that was observed to catch the event
  /// \param eid Event identifier
  /// \param viewWidget Widget of the Slicer layout view. Can be \sa qMRMLSliceWidget or \sa qMRMLThreeDWidget
  /// \return return true to abort the event (prevent other views to receive the event)
  virtual bool processInteractionEvents(vtkRenderWindowInteractor* callerInteractor, unsigned long eid, qMRMLWidget* viewWidget)
  {
    Q_UNUSED(callerInteractor);
    Q_UNUSED(eid);
    Q_UNUSED(viewWidget);
    return false;
  };

  /// Callback function invoked when view node is modified
  /// \param callerViewNode View node that was observed to catch the event. Can be either \sa vtkMRMLSliceNode or \sa vtkMRMLViewNode
  /// \param eid Event identifier
  /// \param viewWidget Widget of the Slicer layout view. Can be \sa qMRMLSliceWidget or \sa qMRMLThreeDWidget
  virtual void processViewNodeEvents(vtkMRMLAbstractViewNode* callerViewNode, unsigned long eid, qMRMLWidget* viewWidget)
  {
    Q_UNUSED(callerViewNode);
    Q_UNUSED(eid);
    Q_UNUSED(viewWidget);
  };

  /// Set default parameters in the parameter MRML node
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void setMRMLDefaults() = 0;

  /// Simple mechanism to let the effects know that reference geometry has changed
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation.
  virtual void referenceGeometryChanged() {};
  /// Simple mechanism to let the effects know that source volume has changed
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void sourceVolumeNodeChanged() {};
  /// Deprecated. Override sourceVolumeNodeChanged() method instead.
  virtual void masterVolumeNodeChanged() {};
  /// Simple mechanism to let the effects know that the layout has changed
  virtual void layoutChanged() {};
  /// Let the effect know that the interaction node is modified.
  /// Default behavior is to deactivate the effect if not in view mode.
  virtual void interactionNodeModified(vtkMRMLInteractionNode* interactionNode);
  /// Clean up resources, event observers, and Qt signal/slot connections before deletion.
  ///
  /// This ensures proper object destruction, as active signal/slot connections
  /// can prevent the object from being deleted. Subclasses should override this
  /// method to handle additional cleanup as needed.
  ///
  /// For more details, see: https://github.com/Slicer/Slicer/issues/7392
  virtual void cleanup() {};

public:
  /// Update user interface from parameter set node
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void updateGUIFromMRML() = 0;

  /// Update parameter set node from user interface
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void updateMRMLFromGUI() = 0;

  // Get/set methods
public:
  /// Get segment editor parameter set node
  vtkMRMLSegmentEditorNode* parameterSetNode();
  /// Set segment editor parameter set node
  void setParameterSetNode(vtkMRMLSegmentEditorNode* node);

  /// Get MRML scene (from parameter set node)
  vtkMRMLScene* scene();

  /// Add actor to the renderer of the view widget. The effect is responsible for
  /// removing the actor when the effect is deactivated.
  void addActor2D(qMRMLWidget* viewWidget, vtkActor2D* actor);

  /// Remove actor from the renderer of the widget.
  void removeActor2D(qMRMLWidget* viewWidget, vtkActor2D* actor);

  /// Add actor to the renderer of the view widget. The effect is responsible for
  /// removing the actor when the effect is deactivated.
  void addActor3D(qMRMLWidget* viewWidget, vtkProp3D* actor);

  /// Remove actor from the renderer of the widget.
  void removeActor3D(qMRMLWidget* viewWidget, vtkProp3D* actor);

  /// Get name of effect.
  /// This name is used by various modules for accessing an effect.
  /// This string is not displayed on the user interface and must not be translated.
  virtual std::string name() const;

  /// Set the name of the effect.
  /// NOTE: name must be defined in constructor in C++ effects, this can only be used in python scripted ones
  virtual void setName(std::string name);

  /// Get title of effect.
  /// This string is displayed on the application GUI and it is translated.
  /// Returns the effect's name when the title is empty.
  virtual std::string title() const;
  /// Set the title of the effect
  virtual void setTitle(std::string title);

  /// Get flag indicating whether effect operates on segments (true) or the whole segmentation (false).
  virtual bool perSegment() const;
  /// Set flag indicating whether effect operates on segments (true) or the whole segmentation (false).
  /// NOTE: name must be defined in constructor in C++ effects, this can only be used in python scripted ones
  virtual void setPerSegment(bool perSegment);

  /// If this property is set to true then this effect is enabled only when the segmentation has segment(s) in it.
  virtual bool requireSegments() const;
  /// If this property is set to true then this effect is enabled only when the segmentation has segment(s) in it.
  virtual void setRequireSegments(bool requireSegments);

  /// Turn off cursor and save cursor to restore later
  void cursorOff(qMRMLWidget* viewWidget);
  /// Restore saved cursor
  void cursorOn(qMRMLWidget* viewWidget);

  /// Emit signal that causes active effect to be changed to the specified one.
  /// If the effect name is empty, then the active effect is de-selected.
  void selectEffect(std::string effectName);

  /// Connect callback signals. Callbacks are called by the editor effect to request operations from the editor widget.
  /// \param selectEffectSlot called from the active effect to initiate switching to another effect (or de-select).
  /// \param updateVolumeSlot called to request update of a volume (modifierLabelmap, alignedSourceVolume, maskLabelmap).
  /// \param saveStateForUndoSlot called to request saving of segmentation state for undo operation
  void setCallbackSlots(QObject* receiver, const char* selectEffectSlot, const char* updateVolumeSlot, const char* saveStateForUndoSlot);

  /// Called by the editor widget.
  void setVolumes(vtkOrientedImageData* alignedSourceVolume,
                  vtkOrientedImageData* modifierLabelmap,
                  vtkOrientedImageData* maskLabelmap,
                  vtkOrientedImageData* selectedSegmentLabelmap,
                  vtkOrientedImageData* referenceGeometryImage);

  // Effect parameter functions
public:
  /// Get effect-specific or common string type parameter from effect parameter set node.
  std::string parameter(std::string name);

  /// Get effect-specific or common integer type parameter from effect parameter set node.
  int integerParameter(std::string name);

  /// Get effect-specific or common double type parameter from effect parameter set node.
  double doubleParameter(std::string name);

  /// Get effect-specific or common node reference type parameter from effect parameter set node.
  vtkMRMLNode* nodeReference(std::string name);

  /// Set effect parameter in effect parameter set node. This function is called by both convenience functions.
  /// \param name Parameter name string
  /// \param value Parameter value string
  void setParameter(std::string name, std::string value);
  /// Set parameter only if it is not defined already.
  /// \sa setParameter
  void setParameterDefault(std::string name, std::string value);
  /// Set parameters that are common for multiple effects. Typically used by base class effects, such
  ///   as label \sa setParameter
  /// By default the parameter names are prefixed for each effect, so they are unique for effects.
  /// This method does not prefix the parameter, so can be the same for multiple effects.
  /// Note: Parameter getter functions look for effect parameters first, then common parameter if the
  ///   effect-specific is not found.
  void setCommonParameter(std::string name, std::string value);
  /// Set parameter only if it is not defined already.
  /// \sa setCommonParameter
  void setCommonParameterDefault(std::string name, std::string value);

  /// Convenience function to set integer parameter
  /// \param name Parameter name string
  /// \param value Parameter value integer
  void setParameter(std::string name, int value);
  /// Set parameter only if it is not defined already.
  /// \sa setParameter
  void setParameterDefault(std::string name, int value);
  /// Convenience function to set integer common parameter \sa setCommonParameter
  void setCommonParameter(std::string name, int value);
  /// Set parameter only if it is not defined already.
  /// \sa setCommonParameter
  void setCommonParameterDefault(std::string name, int value);

  /// Convenience function to set double parameter
  /// \param name Parameter name string
  /// \param value Parameter value double
  void setParameter(std::string name, double value);
  /// Set parameter only if it is not defined already.
  /// \sa setParameter
  void setParameterDefault(std::string name, double value);
  /// Convenience function to set double common parameter \sa setCommonParameter
  void setCommonParameter(std::string name, double value);
  /// Set parameter only if it is not defined already.
  /// \sa setCommonParameter
  void setCommonParameterDefault(std::string name, double value);

  /// Convenience function to set node reference parameter
  /// \param name Parameter name string
  /// \param value Parameter node reference
  void setNodeReference(std::string name, vtkMRMLNode* node);
  /// Convenience function to set node reference common parameter \sa setCommonParameter
  void setCommonNodeReference(std::string name, vtkMRMLNode* node);

  // Utility functions
public:
  /// Returns true if the effect-specific parameter is already defined.
  bool parameterDefined(std::string name);

  /// Returns true if the common parameter is already defined.
  bool commonParameterDefined(std::string name);

  /// If current segment is not visible then asks the user to confirm
  /// that the operation is intended to be performed on the current segment.
  /// Returns NotConfirmed (0) if operation should not proceed with the current segment.
  /// Returns ConfirmedWithoutDialog if operation should proceed with the current segment and dialog was not displayed.
  /// Returns ConfirmedWithDialog if operation should proceed with the current segment and dialog was displayed.
  int confirmCurrentSegmentVisible();

  vtkOrientedImageData* modifierLabelmap();

  /// Reset modifier labelmap to default (resets geometry, clears content)
  /// and return it.
  vtkOrientedImageData* defaultModifierLabelmap();

  vtkOrientedImageData* maskLabelmap();

  vtkOrientedImageData* selectedSegmentLabelmap();

  vtkOrientedImageData* referenceGeometryImage();

  void setShowEffectCursorInSliceView(bool show);
  void setShowEffectCursorInThreeDView(bool show);

  bool showEffectCursorInSliceView();
  bool showEffectCursorInThreeDView();

  /// Get image data of source volume aligned with the modifier labelmap.
  /// \return Pointer to the image data
  vtkOrientedImageData* sourceVolumeImageData();

  /// Deprecated. Use sourceVolumeImageData method instead.
  vtkOrientedImageData* masterVolumeImageData();

  /// Signal to the editor that current state has to be saved (for allowing reverting
  /// to current segmentation state by undo operation)
  void saveStateForUndo();

  /// Get render window for view widget
  static vtkRenderWindow* renderWindow(qMRMLWidget* viewWidget);
  /// Get renderer for view widget
  static vtkRenderer* renderer(qMRMLWidget* viewWidget);
  /// Get node for view widget
  static vtkMRMLAbstractViewNode* viewNode(qMRMLWidget* viewWidget);

  /// Convert RAS position to XY in-slice position
  static std::array<int, 2> rasToXy(double ras[3], qMRMLSliceWidget* sliceWidget);
  /// Convert RAS position to XY in-slice position, python accessor method
  static std::array<int, 2> rasToXy(vtkVector3d ras, qMRMLSliceWidget* sliceWidget);
  /// Convert XYZ slice view position to RAS position:
  /// x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the
  /// slice you're looking at), hence xyToRAS is really performing xyzToRAS. RAS is patient world
  /// coordinate system. Note the 1 is because the transform uses homogeneous coordinates.
  static void xyzToRas(double inputXyz[3], double outputRas[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XYZ slice view position to RAS position, python accessor method
  static vtkVector3d xyzToRas(vtkVector3d inputXyz, qMRMLSliceWidget* sliceWidget);
  /// Convert XY in-slice position to RAS position
  static void xyToRas(std::array<int, 2> xy, double outputRas[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XY in-slice position to RAS position
  static void xyToRas(double xy[2], double outputRas[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XY in-slice position to RAS position, python accessor method
  static vtkVector3d xyToRas(std::array<int, 2> xy, qMRMLSliceWidget* sliceWidget);
  /// Convert XYZ slice view position to image IJK position, \sa xyzToRas
  static void xyzToIjk(double inputXyz[3], int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransform = nullptr);
  /// Convert XYZ slice view position to image IJK position, python accessor method, \sa xyzToRas
  static vtkVector3d xyzToIjk(vtkVector3d inputXyz, qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransform = nullptr);
  /// Convert XY in-slice position to image IJK position
  static void xyToIjk(std::array<int, 2> xy, int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransform = nullptr);
  /// Convert XY in-slice position to image IJK position
  static void xyToIjk(double xy[2], int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransform = nullptr);
  /// Convert XY in-slice position to image IJK position, python accessor method
  static vtkVector3d xyToIjk(std::array<int, 2> xy, qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransform = nullptr);

  static void forceRender(qMRMLWidget* viewWidget);
  static void scheduleRender(qMRMLWidget* viewWidget);

  static double sliceSpacing(qMRMLSliceWidget* sliceWidget);

  bool segmentationDisplayableInView(vtkMRMLAbstractViewNode* viewNode);

protected:
  std::string m_Name;
  bool m_Active{ false };
  std::string m_Title;

  /// Flag indicating whether effect operates on individual segments (true) or the whole segmentation (false).
  /// If the selected effect works on whole segmentation, selection of the segments does not trigger creation
  /// of modifier labelmap, but it is set to empty in the parameter set node.
  /// True by default.
  bool m_PerSegment{ true };

  bool m_RequireSegments{ true };

  bool m_ShowEffectCursorInSliceView{ true };
  bool m_ShowEffectCursorInThreeDView{ false };

  double m_FillValue{ 1.0 };
  double m_EraseValue{ 0.0 };

  /// No confirmation will be displayed for editing this segment.
  /// This is needed to ensure that editing of a hidden segment is only asked once.
  vtkWeakPointer<vtkSegment> m_AlreadyConfirmedSegmentVisible;

protected:
  vtkSmartPointer<vtkSegmentEditorAbstractEffectPrivate> d_ptr;
};

#endif
