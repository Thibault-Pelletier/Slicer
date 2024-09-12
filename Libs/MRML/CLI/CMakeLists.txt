# ModuleDescriptionParser
find_package(SlicerExecutionModel REQUIRED ModuleDescriptionParser)

# ITK - Import ITK targets required by ModuleDescriptionParser
find_package(ITK 5.3 COMPONENTS ${ModuleDescriptionParser_ITK_COMPONENTS} REQUIRED)

set(classes vtkMRMLCommandLineModuleNode)

vtk_module_add_module(VTK::MRMLCLI
  EXPORT_MACRO_PREFIX VTK_MRML_CLI
  CLASSES ${classes}
)

vtk_module_link(VTK::MRMLCLI PUBLIC ${ITK_LIBRARIES} ModuleDescriptionParser)
vtk_module_include(VTK::MRMLCLI PUBLIC $<BUILD_INTERFACE:${ITK_INCLUDE_DIRS} ${ModuleDescriptionParser_INCLUDE_DIRS}>)
