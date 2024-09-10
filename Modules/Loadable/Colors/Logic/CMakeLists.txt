set(classes vtkSlicerColorLogic)

vtk_module_add_module(VTK::SlicerColorsModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_COLORS_MODULE_LOGIC
  CLASSES ${classes}
)
