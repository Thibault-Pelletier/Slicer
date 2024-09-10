set(classes vtkSlicerTablesLogic)

vtk_module_add_module(VTK::SlicerTablesModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_TABLES_MODULE_LOGIC
  CLASSES ${classes}
)
