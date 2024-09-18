set(classes vtkSlicerLibrary)

vtk_module_add_module(VTK::SlicerLibrary
  EXPORT_MACRO_PREFIX VTK_SLICER_LIBRARY
  CLASSES ${classes}
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../CMake)

# Install dependencies!
# Different modules have different dependencies, but let's install them all...
if(WIN32)
  find_package(LibArchive REQUIRED)
  string(REPLACE ".lib" ".dll" LibArchive_DLL ${LibArchive_LIBRARY})
  string(REPLACE "/lib/" "/bin/" LibArchive_DLL ${LibArchive_DLL})

  find_package(JsonCpp REQUIRED)
  string(REPLACE ".lib" ".dll" JsonCpp_DLL ${JsonCpp_LIBRARY})
  string(REPLACE "/lib/" "/bin/" JsonCpp_DLL ${JsonCpp_DLL})

  find_package(SlicerExecutionModel REQUIRED ModuleDescriptionParser)
  get_target_property(ModuleDescriptionParser_dll ModuleDescriptionParser IMPORTED_LOCATION_RELEASE)

  find_package(vtkAddon REQUIRED)
  get_target_property(vtkAddon_dll vtkAddon IMPORTED_LOCATION_RELEASE)
  get_target_property(vtkAddonPython_dll vtkAddonPython IMPORTED_LOCATION_RELEASE)

  install(FILES
    ${LibArchive_DLL}
    ${JsonCpp_DLL}
    ${ModuleDescriptionParser_dll}
    ${vtkAddon_dll}
    ${vtkAddonPython_dll}
    DESTINATION vtkmodules
  )
endif()
