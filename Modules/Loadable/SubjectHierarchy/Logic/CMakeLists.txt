project(vtkSlicerSubjectHierarchyModuleLogic)

set(classes vtkSlicerSubjectHierarchyModuleLogic)

vtk_module_add_module(VTK::SlicerSubjectHierarchyModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_SUBJECTHIERARCHY_LOGIC
  CLASSES ${classes}
)
