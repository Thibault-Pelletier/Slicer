#
# ITK
#
set(${PROJECT_NAME}_ITK_COMPONENTS
  ITKCommon
  )
find_package(ITK 4.6 COMPONENTS ${${PROJECT_NAME}_ITK_COMPONENTS} REQUIRED)
include(${ITK_USE_FILE})

# --------------------------------------------------------------------------
# Configure headers
# --------------------------------------------------------------------------
set(configure_header_file vtkMRMLDisplayableManagerConfigure.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${configure_header_file}.in
  ${CMAKE_CURRENT_BINARY_DIR}/${configure_header_file}
  )

set(classes
  # DisplayableManager associated with ThreeDView
  vtkMRMLCameraDisplayableManager
  vtkMRMLModelDisplayableManager
  vtkMRMLViewDisplayableManager
  vtkMRMLThreeDReformatDisplayableManager

  # DisplayableManager associated with SliceView
  vtkMRMLCrosshairDisplayableManager
  vtkMRMLCrosshairDisplayableManager3D
  vtkMRMLModelSliceDisplayableManager
  vtkMRMLVolumeGlyphSliceDisplayableManager

  # DisplayableManager common between ThreeDView and SliceView
  vtkMRMLOrientationMarkerDisplayableManager
  vtkMRMLRulerDisplayableManager
  vtkMRMLScalarBarDisplayableManager
  vtkMRMLAbstractDisplayableManager
  vtkMRMLDisplayableManagerGroup
  vtkMRMLDisplayableManagerFactory
  vtkMRMLInteractionEventData

  # ThreeDView factory and DisplayableManager
  vtkMRMLAbstractThreeDViewDisplayableManager
  vtkMRMLThreeDViewDisplayableManagerFactory

  # SliceView factory and DisplayableManager
  vtkMRMLAbstractSliceViewDisplayableManager
  vtkMRMLSliceViewDisplayableManagerFactory

  vtkMRMLViewInteractorStyle
  vtkMRMLSliceViewInteractorStyle
  vtkMRMLThreeDViewInteractorStyle

  # Widgets
  vtkMRMLAbstractWidget
  vtkMRMLAbstractWidgetRepresentation
  vtkMRMLCameraWidget
  vtkMRMLInteractionWidget
  vtkMRMLInteractionWidgetRepresentation
  vtkMRMLSliceIntersectionWidget
  vtkMRMLSliceIntersectionRepresentation2D
  vtkMRMLSliceIntersectionInteractionRepresentation
  vtkMRMLSliceIntersectionInteractionRepresentationHelper
  vtkMRMLRubberBandWidgetRepresentation
  vtkMRMLWindowLevelWidget

  # Proxy classes
  vtkMRMLLightBoxRendererManagerProxy
  )

set_source_files_properties(
  vtkMRMLDisplayableManagerGroup.cxx
  vtkMRMLScriptedDisplayableManager.cxx
  WRAP_EXCLUDE
  )

if(VTK_WRAP_PYTHON)
  list(APPEND classes vtkMRMLScriptedDisplayableManager)
  set_source_files_properties(
    vtkMRMLScriptedDisplayableManager.cxx
    WRAP_EXCLUDE
    )
endif()

# --------------------------------------------------------------------------
# Build library
# --------------------------------------------------------------------------

set(factory_classes
  vtkMRMLCameraDisplayableManager
  vtkMRMLModelDisplayableManager
  vtkMRMLViewDisplayableManager
  vtkMRMLThreeDReformatDisplayableManager
  # DisplayableManager associated with SliceView
  vtkMRMLCrosshairDisplayableManager
  vtkMRMLCrosshairDisplayableManager3D
  vtkMRMLModelSliceDisplayableManager
  vtkMRMLVolumeGlyphSliceDisplayableManager
  # DisplayableManager common between ThreeDView and SliceView
  vtkMRMLOrientationMarkerDisplayableManager
  vtkMRMLRulerDisplayableManager
  vtkMRMLScalarBarDisplayableManager
  # ThreeDView DisplayableManager
  vtkMRMLAbstractThreeDViewDisplayableManager
  # SliceView DisplayableManager
  vtkMRMLAbstractSliceViewDisplayableManager
  )
foreach(class IN LISTS factory_classes)
  vtk_object_factory_declare(BASE ${class} OVERRIDE ${class})
endforeach()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "")

set_property(GLOBAL PROPERTY "_vtk_module_autoinit_include" "\"vtkAutoInit.h\"")
vtk_module_add_module(VTK::MRMLDisplayableManager
  EXPORT_MACRO_PREFIX VTK_MRML_DISPLAYABLEMANAGER
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  HEADERS ${headers}
  PRIVATE_HEADERS ${vtk_object_factory_header}
)

if(VTK_WRAP_PYTHON)
  vtk_module_definitions(VTK::MRMLDisplayableManager
    PRIVATE
      MRMLDisplayableManager_USE_PYTHON
      "vtkMRMLDisplayableManager_AUTOINIT=1(vtkMRMLDisplayableManager)"
  )
  vtk_module_include(VTK::MRMLDisplayableManager PUBLIC ${Python3_INCLUDE_DIRS})
  vtk_module_link(VTK::MRMLDisplayableManager PUBLIC Python3::Python VTK::WrappingPythonCore)
endif()

vtk_module_link(VTK::MRMLDisplayableManager PUBLIC ${ITK_LIBRARIES})
vtk_module_include(VTK::MRMLDisplayableManager PUBLIC ${ITK_INCLUDE_DIRS})
