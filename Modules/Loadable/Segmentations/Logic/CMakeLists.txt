project(vtkSlicerSegmentationsModuleLogic)

set(classes
  FibHeap
  vtkImageGrowCutSegment
  vtkSegmentEditorLogic
  vtkSlicerSegmentationGeometryLogic
  vtkSlicerSegmentationsModuleLogic
  )

vtk_module_add_module(VTK::SlicerSegmentationsModuleLogic
  EXPORT_MACRO_PREFIX VTK_SLICER_SEGMENTATIONS_LOGIC
  CLASSES ${classes}
)
