project(vtkSlicerVolumeRenderingModuleMRML)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(JsonCpp REQUIRED)

set(classes
  vtkMRMLCPURayCastVolumeRenderingDisplayNode
  vtkMRMLGPURayCastVolumeRenderingDisplayNode
  vtkMRMLMultiVolumeRenderingDisplayNode
  vtkMRMLShaderPropertyNode
  vtkMRMLShaderPropertyStorageNode
  vtkMRMLVolumePropertyNode
  vtkMRMLVolumePropertyStorageNode
  vtkMRMLVolumeRenderingDisplayNode
  )

vtk_module_add_module(VTK::SlicerVolumeRenderingModuleMRML
  EXPORT_MACRO_PREFIX VTK_SLICER_VOLUMERENDERING_MODULE_MRML
  CLASSES ${classes}
)

vtk_module_include(VTK::SlicerVolumeRenderingModuleMRML PUBLIC $<BUILD_INTERFACE:${JsonCpp_INCLUDE_DIRS}>)
vtk_module_link(VTK::SlicerVolumeRenderingModuleMRML PUBLIC ${JsonCpp_LIBRARY})

string(REPLACE ".lib" ".dll" JsonCpp_DLL ${JsonCpp_LIBRARY})
string(REPLACE "/lib/" "/bin/" JsonCpp_DLL ${JsonCpp_DLL})
install(FILES ${JsonCpp_DLL} DESTINATION vtkmodules)
