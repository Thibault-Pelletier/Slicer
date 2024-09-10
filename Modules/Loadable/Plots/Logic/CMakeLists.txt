set(classes vtkSlicerPlotsLogic)

vtk_module_add_module(VTK::SlicerPlotsModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_PLOTS_MODULE_LOGIC
  CLASSES ${classes}
)
