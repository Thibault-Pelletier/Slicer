find_package(RapidJSON REQUIRED)

set(classes
  vtkMRMLMarkupsAngleNode
  vtkMRMLMarkupsClosedCurveNode
  vtkMRMLMarkupsCurveNode
  vtkMRMLMarkupsDisplayNode
  vtkMRMLMarkupsFiducialDisplayNode
  vtkMRMLMarkupsFiducialStorageNode
  vtkMRMLMarkupsFiducialNode
  vtkMRMLMarkupsJsonElement
  vtkMRMLMarkupsJsonStorageNode
  vtkMRMLMarkupsLineNode
  vtkMRMLMarkupsNode
  vtkMRMLMarkupsPlaneDisplayNode
  vtkMRMLMarkupsPlaneJsonStorageNode
  vtkMRMLMarkupsPlaneNode
  vtkMRMLMarkupsROIDisplayNode
  vtkMRMLMarkupsROIJsonStorageNode
  vtkMRMLMarkupsROINode
  vtkCurveMeasurementsCalculator
  vtkMRMLMeasurementAngle
  vtkMRMLMeasurementArea
  vtkMRMLMeasurementLength
  vtkMRMLMeasurementVolume
  vtkProjectMarkupsCurvePointsFilter
  )

vtk_module_add_module(VTK::SlicerMarkupsModuleMRML
  EXPORT_MACRO_PREFIX VTK_SLICER_MARKUPS_MODULE_MRML
  CLASSES ${classes}
)

vtk_module_include(VTK::SlicerMarkupsModuleMRML PUBLIC $<BUILD_INTERFACE:${RapidJSON_INCLUDE_DIR}>)
