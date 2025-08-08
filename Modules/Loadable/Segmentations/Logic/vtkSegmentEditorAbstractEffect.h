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

#include "vtkSlicerSegmentationsModuleLogicExport.h"

#include <array>
#include <vector>
#include <string>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>
#include <vtkObject.h>

class vtkMRMLAbstractViewNode;
class vtkMRMLInteractionNode;
class vtkMRMLNode;
class vtkMRMLScene;
class vtkMRMLSegmentEditorNode;
class vtkMRMLSegmentationNode;
class vtkMRMLSliceNode;
class vtkMRMLTransformNode;
class vtkOrientedImageData;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkSegment;
class vtkProp;
class vtkSegmentEditorLogic;

/// \brief Abstract class for segment editor effects
class VTK_SLICER_SEGMENTATIONS_LOGIC_EXPORT vtkSegmentEditorAbstractEffect : public vtkObject
{
  // API: Methods that are to be reimplemented in the effect subclasses
public:
  static vtkSegmentEditorAbstractEffect* New();
  vtkTypeMacro(vtkSegmentEditorAbstractEffect, vtkObject);

  enum ModificationMode
  {
    ModificationModeSet,
    ModificationModeAdd,
    ModificationModeRemove,
    ModificationModeRemoveAll
  };

  /// Get help text for effect to be displayed in the help box
  virtual const std::string GetHelpText() const { return {}; };

  /// Get icon segment editor icon path
  virtual const std::string GetIcon() const { return {}; };

  /// Clone editor effect. Override to return a new instance of the effect sub-class
  virtual vtkSegmentEditorAbstractEffect* Clone() { throw std::runtime_error("Unimplemented method "); }

  /// Perform actions to activate the effect (show options frame, etc.)
  /// NOTE: Base class implementation needs to be called BEFORE the effect-specific implementation
  virtual void Activate();

  /// Perform actions to deactivate the effect (hide options frame, destroy actors, etc.)
  /// NOTE: Base class implementation needs to be called BEFORE the effect-specific implementation
  virtual void Deactivate();

  /// Returns true if the effect is currently active (activated and has not deactivated since then)
  bool IsActive();

  void ModifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap, ModificationMode modificationMode, const int modificationExtent[6], bool bypassMasking = false);
  void ModifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap, ModificationMode modificationMode, bool bypassMasking = false);
  void ModifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap, ModificationMode modificationMode, const std::vector<int>& extent, bool bypassMasking = false);
  void ModifySegmentByLabelmap(vtkMRMLSegmentationNode* segmentationNode,
                               const char* segmentID,
                               vtkOrientedImageData* modifierLabelmap,
                               ModificationMode modificationMode,
                               bool bypassMasking = false);
  void ModifySegmentByLabelmap(vtkMRMLSegmentationNode* segmentationNode,
                               const char* segmentID,
                               vtkOrientedImageData* modifierLabelmap,
                               ModificationMode modificationMode,
                               const int modificationExtent[6],
                               bool bypassMasking = false);

  /// Apply mask image on an input image.
  /// This method is kept here for backward compatibility only and will be removed in the future.
  /// Use vtkOrientedImageDataResample::ApplyImageMask method instead.
  static void ApplyImageMask(vtkOrientedImageData* input, vtkOrientedImageData* mask, double fillValue, bool notMask = false);

  /// Callback function invoked when interaction happens
  /// \param callerInteractor Interactor object that was observed to catch the event
  /// \param eid Event identifier
  /// \param viewWidget Widget of the Slicer layout view. Can be \sa qMRMLSliceWidget or \sa qMRMLThreeDWidget
  /// \return return true to abort the event (prevent other views to receive the event)
  virtual bool ProcessInteractionEvents(vtkRenderWindowInteractor* callerInteractor, unsigned long eid, vtkRenderWindow* renderWindow, vtkMRMLAbstractViewNode* viewNode)
  {
    (void)callerInteractor;
    (void)eid;
    (void)renderWindow;
    (void)viewNode;
    return false;
  };

  /// Callback function invoked when view node is modified
  /// \param callerViewNode View node that was observed to catch the event. Can be either \sa vtkMRMLSliceNode or \sa vtkMRMLViewNode
  /// \param eid Event identifier
  virtual void ProcessViewNodeEvents(vtkMRMLAbstractViewNode* callerViewNode, unsigned long eid, vtkRenderWindow* renderWindow)
  {
    (void)callerViewNode;
    (void)eid;
  };

  /// Set default parameters in the parameter MRML node
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void SetMRMLDefaults() {}

  /// Simple mechanism to let the effects know that reference geometry has changed
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation.
  virtual void ReferenceGeometryChanged() {};
  /// Simple mechanism to let the effects know that source volume has changed
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void SourceVolumeNodeChanged() {};
  /// Simple mechanism to let the effects know that the layout has changed
  virtual void LayoutChanged() {};
  /// Let the effect know that the interaction node is modified.
  /// Default behavior is to deactivate the effect if not in view mode.
  virtual void InteractionNodeModified(vtkMRMLInteractionNode* interactionNode);
  /// Clean up resources, event observers before deletion.
  ///
  /// This ensures proper object destruction, as active signal/slot connections
  /// can prevent the object from being deleted. Subclasses should override this
  /// method to handle additional cleanup as needed.
  ///
  /// For more details, see: https://github.com/Slicer/Slicer/issues/7392
  virtual void Cleanup() {}

  /// Get segment editor parameter set node
  vtkMRMLSegmentEditorNode* GetSegmentEditorNode();

  /// Set segment editor parameter set node
  void SetSegmentEditorNode(vtkMRMLSegmentEditorNode* node);

  /// Get MRML scene (from parameter set node)
  vtkMRMLScene* GetScene();

  /// Add actor to the renderer of the view widget. The effect is responsible for
  /// removing the actor when the effect is deactivated.
  void AddViewProp(vtkMRMLAbstractViewNode* viewNode, vtkProp* actor);

  /// Remove actor from the renderer of the widget.
  void RemoveViewProp(vtkMRMLAbstractViewNode* viewNode, vtkProp* actor);

  /// Get name of effect.
  /// This name is used by various modules for accessing an effect.
  /// This string is not displayed on the user interface and must not be translated.
  virtual std::string GetName() const;

  /// Set the name of the effect.
  /// NOTE: name must be defined in constructor in C++ effects, this can only be used in python scripted ones
  virtual void SetName(const std::string& name);

  /// Get title of effect.
  /// This string is displayed on the application GUI and it is translated.
  /// Returns the effect's name when the title is empty.
  virtual std::string GetTitle() const;
  /// Set the title of the effect
  virtual void SetTitle(std::string title);

  /// Get flag indicating whether effect operates on segments (true) or the whole segmentation (false).
  virtual bool IsPerSegment() const;
  /// Set flag indicating whether effect operates on segments (true) or the whole segmentation (false).
  /// NOTE: name must be defined in constructor in C++ effects, this can only be used in python scripted ones
  virtual void SetPerSegment(bool perSegment);

  /// If this property is set to true then this effect is enabled only when the segmentation has segment(s) in it.
  virtual bool RequireSegments() const;
  /// If this property is set to true then this effect is enabled only when the segmentation has segment(s) in it.
  virtual void SetRequireSegments(bool requireSegments);

  /// Emit signal that causes active effect to be changed to the specified one.
  /// If the effect name is empty, then the active effect is de-selected.
  void SelectEffect(std::string effectName);

  /// Called by the editor widget.
  void SetVolumes(vtkOrientedImageData* alignedSourceVolume,
                  vtkOrientedImageData* modifierLabelmap,
                  vtkOrientedImageData* maskLabelmap,
                  vtkOrientedImageData* selectedSegmentLabelmap,
                  vtkOrientedImageData* referenceGeometryImage);

  // Effect parameter functions
  /// Get effect-specific or common string type parameter from effect parameter set node.
  std::string GetParameter(std::string name);

  /// Get effect-specific or common integer type parameter from effect parameter set node.
  int GetIntegerParameter(std::string name);

  /// Get effect-specific or common double type parameter from effect parameter set node.
  double GetDoubleParameter(std::string name);

  /// Get effect-specific or common node reference type parameter from effect parameter set node.
  vtkMRMLNode* GetNodeReference(std::string name);

  /// Set effect parameter in effect parameter set node. This function is called by both convenience functions.
  /// \param name Parameter name string
  /// \param value Parameter value string
  void SetParameter(std::string name, std::string value);
  /// Set parameter only if it is not defined already.
  /// \sa setParameter
  void SetParameterDefault(std::string name, std::string value);
  /// Set parameters that are common for multiple effects. Typically used by base class effects, such
  ///   as label \sa setParameter
  /// By default the parameter names are prefixed for each effect, so they are unique for effects.
  /// This method does not prefix the parameter, so can be the same for multiple effects.
  /// Note: Parameter getter functions look for effect parameters first, then common parameter if the
  ///   effect-specific is not found.
  void SetCommonParameter(std::string name, std::string value);
  /// Set parameter only if it is not defined already.
  /// \sa setCommonParameter
  void SetCommonParameterDefault(std::string name, std::string value);

  /// Convenience function to set integer parameter
  /// \param name Parameter name string
  /// \param value Parameter value integer
  void SetParameter(std::string name, int value);
  /// Set parameter only if it is not defined already.
  /// \sa setParameter
  void SetParameterDefault(std::string name, int value);
  /// Convenience function to set integer common parameter \sa setCommonParameter
  void SetCommonParameter(std::string name, int value);
  /// Set parameter only if it is not defined already.
  /// \sa setCommonParameter
  void SetCommonParameterDefault(std::string name, int value);

  /// Convenience function to set double parameter
  /// \param name Parameter name string
  /// \param value Parameter value double
  void SetParameter(std::string name, double value);
  /// Set parameter only if it is not defined already.
  /// \sa setParameter
  void SetParameterDefault(std::string name, double value);
  /// Convenience function to set double common parameter \sa setCommonParameter
  void SetCommonParameter(std::string name, double value);
  /// Set parameter only if it is not defined already.
  /// \sa setCommonParameter
  void SetCommonParameterDefault(std::string name, double value);

  /// Convenience function to set node reference parameter
  /// \param name Parameter name string
  /// \param value Parameter node reference
  void SetNodeReference(std::string name, vtkMRMLNode* node);
  /// Convenience function to set node reference common parameter \sa setCommonParameter
  void SetCommonNodeReference(std::string name, vtkMRMLNode* node);

  // Utility functions
  /// Returns true if the effect-specific parameter is already defined.
  bool IsParameterDefined(std::string name);

  /// Returns true if the common parameter is already defined.
  bool IsCommonParameterDefined(std::string name);

  vtkSegment* GetSelectedSegment();
  bool IsSelectedSegmentVisible();
  void ConfirmWorkOnSelectedSegment(bool doShowSegment);

  vtkOrientedImageData* GetModifierLabelmap();

  /// Reset modifier labelmap to default (resets geometry, clears content)
  /// and return it.
  vtkOrientedImageData* GetDefaultModifierLabelmap();

  vtkOrientedImageData* GetMaskLabelmap();

  vtkOrientedImageData* GetSelectedSegmentLabelmap();

  vtkOrientedImageData* GetReferenceGeometryImage();

  void SetShowEffectCursorInSliceView(bool show);
  void SetShowEffectCursorInThreeDView(bool show);

  bool IsShownEffectCursorInSliceView();
  bool IsShownEffectCursorInThreeDView();

  /// Get image data of source volume aligned with the modifier labelmap.
  /// \return Pointer to the image data
  vtkOrientedImageData* GetSourceVolumeImageData();

  /// Signal to the editor that current state has to be saved (for allowing reverting
  /// to current segmentation state by undo operation)
  void SaveStateForUndo();

  /// Get renderer for view widget
  static vtkRenderer* GetRenderer(vtkRenderWindow* renderWindow);

  /// Convert RAS position to XY in-slice position
  static std::array<int, 2> RasToXy(double ras[3], vtkMRMLSliceNode* sliceNode);
  /// Convert XYZ slice view position to RAS position:
  /// x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the
  /// slice you're looking at), hence xyToRAS is really performing xyzToRAS. RAS is patient world
  /// coordinate system. Note the 1 is because the transform uses homogeneous coordinates.
  static void XyzToRas(double inputXyz[3], double outputRas[3], vtkMRMLSliceNode* sliceNode);
  static std::array<double, 3> XyzToRas(double inputXyz[3], vtkMRMLSliceNode* sliceNode);

  /// Convert XY in-slice position to RAS position
  static void XyToRas(int xy[2], double outputRas[3], vtkMRMLSliceNode* sliceNode);
  /// Convert XY in-slice position to RAS position
  static void XyToRas(double xy[2], double outputRas[3], vtkMRMLSliceNode* sliceNode);
  /// Convert XY in-slice position to RAS position, python accessor method
  static std::array<double, 3> XyToRas(int xy[2], vtkMRMLSliceNode* sliceNode);
  /// Convert XYZ slice view position to image IJK position, \sa xyzToRas
  static void XyzToIjk(double inputXyz[3], int outputIjk[3], vtkMRMLSliceNode* sliceNode, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransform = nullptr);
  /// Convert XYZ slice view position to image IJK position, python accessor method, \sa xyzToRas
  static std::array<int, 3> XyzToIjk(double inputXyz[3], vtkMRMLSliceNode* sliceNode, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransform = nullptr);
  /// Convert XY in-slice position to image IJK position
  static void XyToIjk(int xy[2], int outputIjk[3], vtkMRMLSliceNode* sliceNode, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransform = nullptr);
  /// Convert XY in-slice position to image IJK position
  static void XyToIjk(double xy[2], int outputIjk[3], vtkMRMLSliceNode* sliceNode, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransform = nullptr);
  /// Convert XY in-slice position to image IJK position, python accessor method
  static std::array<int, 3> XyToIjk(int xy[2], vtkMRMLSliceNode* sliceNode, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransform = nullptr);

  static void ForceRender(vtkMRMLAbstractViewNode* viewNode);
  static void ScheduleRender(vtkMRMLAbstractViewNode* viewNode);

  static double GetSliceSpacing(vtkMRMLSliceNode* sliceNode);

  bool IsSegmentationDisplayableInView(vtkMRMLAbstractViewNode* viewNode);

  void SetSegmentEditorLogic(vtkSegmentEditorLogic* logic);
  vtkSegmentEditorLogic* GetSegmentEditorLogic() const;

protected:
  vtkSegmentEditorAbstractEffect();
  ~vtkSegmentEditorAbstractEffect() override;

  vtkOrientedImageData* UpdateVolume(vtkOrientedImageData* volume);

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

private:
  std::string GetAttributeName(const std::string& name);

  /// Segment editor parameter set node
  vtkWeakPointer<vtkMRMLSegmentEditorNode> m_SegmentEditorNode = nullptr;

  /// MRML scene
  vtkMRMLScene* m_Scene = nullptr;

  /// Aligned source volume is a copy of image in source volume node
  /// resampled into the reference image geometry of the segmentation.
  /// If the source volume geometry is the same as the reference image geometry
  /// then only a shallow copy is performed.
  vtkWeakPointer<vtkOrientedImageData> m_AlignedSourceVolume = nullptr;

  /// Active labelmap for editing. Mainly needed because the segment binary labelmaps are shrunk
  /// to the smallest possible extent, but the user wants to draw on the whole source volume.
  /// It also allows modifying a segment by adding/removing regions (and performing inverse
  /// of that on all other segments).
  vtkWeakPointer<vtkOrientedImageData> m_ModifierLabelmap = nullptr;

  /// Mask labelmap containing a merged silhouette of all the segments other than the selected one.
  /// Used if the paint over feature is turned off.
  vtkWeakPointer<vtkOrientedImageData> m_MaskLabelmap = nullptr;

  /// SelectedSegmentLabelmap is a copy of the labelmap of the current segment
  /// resampled into the reference image geometry of the segmentation.
  vtkWeakPointer<vtkOrientedImageData> m_SelectedSegmentLabelmap = nullptr;

  /// Image that holds current reference geometry information. Scalars are not allocated.
  /// Changing it does not change the reference geometry of the segment, it is just a copy,
  /// for convenience.
  vtkWeakPointer<vtkOrientedImageData> m_ReferenceGeometryImage = nullptr;

  vtkWeakPointer<vtkSegmentEditorLogic> m_Logic = nullptr;
};

#endif
