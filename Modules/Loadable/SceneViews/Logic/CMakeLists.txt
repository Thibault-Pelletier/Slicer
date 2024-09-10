set(classes vtkSlicerSceneViewsModuleLogic)

vtk_module_add_module(VTK::SlicerSceneViewsModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_SCENEVIEWS_MODULE_LOGIC
  CLASSES ${classes}
)