set(classes vtkMRMLColorLegendDisplayNode)

vtk_module_add_module(VTK::SlicerColorsModuleMRML
  EXPORT_MACRO_PREFIX VTK_SLICER_COLORS_MODULE_MRML
  CLASSES ${classes}
)