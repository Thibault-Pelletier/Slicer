set(classes
  vtkMRMLAnnotationSnapshotNode
  vtkMRMLAnnotationSnapshotStorageNode
  vtkMRMLAnnotationHierarchyNode
  vtkMRMLAnnotationNode
  vtkMRMLAnnotationStorageNode
  vtkMRMLAnnotationDisplayNode
  vtkMRMLAnnotationTextDisplayNode
  vtkMRMLAnnotationControlPointsNode
  vtkMRMLAnnotationControlPointsStorageNode
  vtkMRMLAnnotationPointDisplayNode
  vtkMRMLAnnotationLinesNode
  vtkMRMLAnnotationLinesStorageNode
  vtkMRMLAnnotationLineDisplayNode
  vtkMRMLAnnotationRulerNode
  vtkMRMLAnnotationRulerStorageNode
  vtkMRMLAnnotationFiducialNode
  vtkMRMLAnnotationFiducialsStorageNode
  vtkMRMLAnnotationROINode
)

vtk_module_add_module(VTK::SlicerAnnotationsModuleMRML
  EXPORT_MACRO_PREFIX VTK_SLICER_ANNOTATIONS_MODULE_MRML
  CLASSES ${classes}
)
