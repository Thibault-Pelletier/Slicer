set(classes vtkSlicerVolumeRenderingLogic)

vtk_module_add_module(VTK::SlicerVolumeRenderingModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_VOLUMERENDERING_MODULE_LOGIC
  CLASSES ${classes}
)
