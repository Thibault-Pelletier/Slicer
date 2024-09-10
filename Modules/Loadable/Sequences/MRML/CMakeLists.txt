set(classes vtkMRMLSequenceBrowserNode)

vtk_module_add_module(VTK::SlicerSequencesModuleMRML
  EXPORT_MACRO_PREFIX VTK_SLICER_SEQUENCES_MODULE_MRML
  CLASSES ${classes}
)
