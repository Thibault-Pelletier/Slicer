set(classes vtkSlicerUnitsLogic)

vtk_module_add_module(VTK::SlicerUnitsModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_UNITS_MODULE_LOGIC
  CLASSES ${classes}
)
