set(classes vtkSlicerCropVolumeLogic)

vtk_module_add_module(VTK::SlicerCropVolumeModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_CROPVOLUME_MODULE_Logic
  CLASSES ${classes}
)

message(FATAL_ERROR "This modules depends on QtCLI :)")
