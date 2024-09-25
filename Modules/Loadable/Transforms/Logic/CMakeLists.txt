find_package(ITK 5.0 REQUIRED COMPONENTS ITKTransform)

set(classes vtkSlicerTransformLogic)

vtk_module_add_module(VTK::SlicerTransformsModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_TRANSFORMS_MODULE_LOGIC
  CLASSES ${classes}
)

vtk_module_link(VTK::SlicerTransformsModuleLogic PUBLIC ${ITK_LIBRARIES})
vtk_module_include(VTK::SlicerTransformsModuleLogic PUBLIC ${ITK_INCLUDE_DIRS})
