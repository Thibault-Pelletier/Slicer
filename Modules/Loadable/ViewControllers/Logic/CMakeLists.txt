set(classes vtkSlicerViewControllersLogic)

vtk_module_add_module(VTK::SlicerViewControllersModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_VIEWCONTROLLERS_MODULE_LOGIC
  CLASSES ${classes}
)
