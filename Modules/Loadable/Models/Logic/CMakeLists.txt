set(classes vtkSlicerModelsLogic)

vtk_module_add_module(VTK::SlicerModelsModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_MODELS_MODULE_LOGIC
  CLASSES ${classes}
)
