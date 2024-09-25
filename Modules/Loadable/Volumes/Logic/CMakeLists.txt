find_package(RapidJSON REQUIRED)

set(classes vtkSlicerVolumesLogic)

vtk_module_add_module(VTK::SlicerVolumesModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_VOLUMES_MODULE_LOGIC
  CLASSES ${classes}
)

vtk_module_include(VTK::SlicerVolumesModuleLogic PUBLIC ${RapidJSON_INCLUDE_DIR})
