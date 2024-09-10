set(classes vtkSlicerScalarBarActor)

vtk_module_add_module(VTK::SlicerColorsModuleVTKWidgets
  EXPORT_MACRO_PREFIX VTK_SLICER_COLORS_MODULE_VTKWIDGETS
  CLASSES ${classes}
)
