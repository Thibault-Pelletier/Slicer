set(classes vtkSlicerReformatLogic)

vtk_module_add_module(VTK::SlicerReformatModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_REFORMAT_MODULE_LOGIC
  CLASSES ${classes}
)
