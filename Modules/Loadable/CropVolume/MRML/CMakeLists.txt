set(classes vtkMRMLCropVolumeParametersNode)

vtk_module_add_module(VTK::SlicerCropVolumeModuleMRML
  EXPORT_MACRO_PREFIX VTK_SLICER_CROPVOLUME_MODULE_MRML
  CLASSES ${classes}
)
