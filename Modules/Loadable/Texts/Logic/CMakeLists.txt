set(classes vtkSlicerTextsLogic)

vtk_module_add_module(VTK::SlicerTextsModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_TEXTS_MODULE_LOGIC
  CLASSES ${classes}
)
