set(classes vtkSlicerSequencesLogic)

vtk_module_add_module(VTK::SlicerSequencesModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_SEQUENCES_MODULE_LOGIC
  CLASSES ${classes}
)
