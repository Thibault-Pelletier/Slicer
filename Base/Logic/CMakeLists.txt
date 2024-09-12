# --------------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------------

#
# ModuleDescriptionParser
#
find_package(SlicerExecutionModel REQUIRED ModuleDescriptionParser)

#
# ITK
#
find_package(ITK 4.6 COMPONENTS
  ITKCommon
  ITKIOTransformBase
  ITKTransform
  ${ModuleDescriptionParser_ITK_COMPONENTS}
  REQUIRED
)
include(${ITK_USE_FILE})

#
# LibArchive
#
find_package(LibArchive REQUIRED MODULE)

# --------------------------------------------------------------------------
# Subdirectories
# --------------------------------------------------------------------------
add_subdirectory(Resources)

#--------------------------------------------------------------------
# Configure headers
# --------------------------------------------------------------------------
set(configure_header_file vtkSlicerBaseLogicConfigure.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${configure_header_file}.in
  ${CMAKE_CURRENT_BINARY_DIR}/${configure_header_file}
  )

# --------------------------------------------------------------------------
# Build library
# --------------------------------------------------------------------------

set(classes
  # Logic classes (data management and calculation)
  vtkSlicerApplicationLogic
  vtkSlicerModuleLogic
  vtkSlicerTask
  vtkDataIOManagerLogic
  # Slicer's VTK extensions (filters)
  vtkTransformVisualizerGlyph3D
  vtkImageRectangularSource
  vtkSystemInformation
  vtkImageFillROI
  vtkSlicerCLIModuleLogic
)

set(headers ${CMAKE_CURRENT_BINARY_DIR}/${configure_header_file})

vtk_module_add_module(VTK::SlicerBaseLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_BASE_LOGIC
  CLASSES ${classes}
  HEADERS ${headers}
)

vtk_module_link(VTK::SlicerBaseLogic PUBLIC
  ${ITK_LIBRARIES} ${LibArchive_LIBRARY} ModuleDescriptionParser
)

vtk_module_include(VTK::SlicerBaseLogic PUBLIC
  $<BUILD_INTERFACE:${ITK_INCLUDE_DIRS} ${LibArchive_INCLUDE_DIRS} ${ModuleDescriptionParser_INCLUDE_DIRS}>
)
