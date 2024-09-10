set(classes vtkSlicerAnnotationModuleLogic)

vtk_module_add_module(VTK::SlicerAnnotationsModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_ANNOTATIONS_MODULE_LOGIC
  CLASSES ${classes}
)
