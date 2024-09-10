set(classes vtkSlicerCamerasModuleLogic)

vtk_module_add_module(VTK::SlicerCamerasModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_CAMERAS_MODULE_LOGIC
  CLASSES ${classes}
)
