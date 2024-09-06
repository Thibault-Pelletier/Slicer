set(classes
  vtkMRMLSegmentationsDisplayableManager3D
  vtkMRMLSegmentationsDisplayableManager2D
  )

foreach(class IN LISTS classes)
  vtk_object_factory_declare(BASE ${class} OVERRIDE ${class})
endforeach()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "")

vtk_module_add_module(VTK::SlicerSegmentationsModuleMRMLDisplayableManager
  EXPORT_MACRO_PREFIX VTK_SLICER_SEGMENTATION_MODULE_MRMLDISPLAYABLEMANAGER
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header}
)

vtk_module_definitions(VTK::SlicerSegmentationsModuleMRMLDisplayableManager PRIVATE
  "vtkSlicerSegmentationsModuleMRMLDisplayableManager_AUTOINIT=1(vtkSlicerSegmentationsModuleMRMLDisplayableManager)")
