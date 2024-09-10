set(classes vtkSlicerMarkupsLogic)

vtk_module_add_module(VTK::SlicerMarkupsModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_MARKUPS_MODULE_LOGIC
  CLASSES ${classes}
)
