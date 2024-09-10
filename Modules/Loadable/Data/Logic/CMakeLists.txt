set(classes vtkSlicerDataModuleLogic)

vtk_module_add_module(VTK::SlicerDataModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_DATA_MODULE_LOGIC
  CLASSES ${classes}
)
