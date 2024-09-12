set(classes vtkSlicerCropVolumeLogic)

vtk_module_add_module(VTK::SlicerCropVolumeModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_CROPVOLUME_MODULE_LOGIC
  CLASSES ${classes}
)
