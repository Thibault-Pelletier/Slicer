project(vtkSlicerSegmentationsModuleMRMLModule)

set(classes vtkMRMLSegmentEditorNode)

vtk_module_add_module(VTK::SlicerSegmentationsModuleMRML
  EXPORT_MACRO_PREFIX VTK_SLICER_SEGMENTATIONS_MODULE_MRML
  CLASSES ${classes}
)
