project(vtkSlicerTerminologiesModuleLogic)

find_package(RapidJSON REQUIRED)

set(classes
  vtkSlicerTerminologiesModuleLogic
  vtkSlicerTerminologyCategory
  vtkSlicerTerminologyType
  vtkSlicerTerminologyEntry
  )

vtk_module_add_module(VTK::SlicerTerminologiesModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_TERMINOLOGIES_LOGIC
  CLASSES ${classes}
)

vtk_module_include(VTK::SlicerTerminologiesModuleLogic PUBLIC ${RapidJSON_INCLUDE_DIR})
