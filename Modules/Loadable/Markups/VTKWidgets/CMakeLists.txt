set(classes
  vtkMarkupsGlyphSource2D
  vtkFastSelectVisiblePoints
  vtkSlicerMarkupsInteractionWidget
  vtkSlicerMarkupsInteractionWidgetRepresentation
  vtkSlicerMarkupsWidgetRepresentation
  vtkSlicerMarkupsWidgetRepresentation3D
  vtkSlicerMarkupsWidgetRepresentation2D
  vtkSlicerMarkupsWidget
  vtkSlicerAngleRepresentation2D
  vtkSlicerAngleRepresentation3D
  vtkSlicerAngleWidget
  vtkSlicerCurveRepresentation2D
  vtkSlicerCurveRepresentation3D
  vtkSlicerCurveWidget
  vtkSlicerLineRepresentation3D
  vtkSlicerLineRepresentation2D
  vtkSlicerLineWidget
  vtkSlicerPointsRepresentation3D
  vtkSlicerPointsRepresentation2D
  vtkSlicerPointsWidget
  vtkSlicerPlaneRepresentation2D
  vtkSlicerPlaneRepresentation3D
  vtkSlicerPlaneWidget
  vtkSlicerROIRepresentation2D
  vtkSlicerROIRepresentation3D
  vtkSlicerROIWidget
)

vtk_module_add_module(VTK::SlicerMarkupsModuleVTKWidgets
  EXPORT_MACRO_PREFIX VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS
  CLASSES ${classes}
)
