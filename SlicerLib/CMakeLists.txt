set(classes vtkSlicerLibrary)

vtk_module_add_module(VTK::SlicerLibrary
  EXPORT_MACRO_PREFIX VTK_SLICER_LIBRARY
  CLASSES ${classes}
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../CMake)

set(Slicer_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(Slicer_PYTHON_SOURCE_DIR ${Slicer_SOURCE_DIR}/Base/Python)

set(Slicer_PYTHON_MODULES_CONFIG [['mrml','vtkAddon','vtkSegmentationCore']])
configure_file(${Slicer_SOURCE_DIR}/Base/Python/slicer/kits.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/slicer/kits.py
)

# Install slicer scripts comptatible with slicerlib
# These scripts enable users to use slicer in python as-if they were using SlicerPython
# Example: `import slicer; slicer.vtkMRMLNode()`
# Everything related to ctk, qt, etc is not supported, so calling some function of util.py will
# fail at runtime.
install(FILES
  ${Slicer_PYTHON_SOURCE_DIR}/slicer/__init__.py
  ${Slicer_PYTHON_SOURCE_DIR}/slicer/util.py
  ${Slicer_PYTHON_SOURCE_DIR}/slicer/logic.py
  ${CMAKE_CURRENT_BINARY_DIR}/slicer/kits.py
  ${Slicer_PYTHON_SOURCE_DIR}/mrml.py
  ${Slicer_PYTHON_SOURCE_DIR}/vtkAddon.py
  ${Slicer_PYTHON_SOURCE_DIR}/vtkSegmentationCore.py
  DESTINATION slicer
)

# Support consuming SlicerLibrary build tree as a python package combined to
# an external VTK build tree.
# To use VTK and SlicerLibrary as a build tree from python, PYTHONPATH environment variable
# must reference SlicerLibrary's site-packages BEFORE VTK's site-packages because VTK does not
# include this pkgutil trick to load multiple vtkmodules folder into a single package!
# Example: PYTHONPATH=SlicerLibrary-build/lib/site-packages;VTK-build/lib/site-packages
file(WRITE ${CMAKE_BINARY_DIR}/lib/site-packages/vtkmodules/__init__.py "
__path__ = __import__(\"pkgutil\").extend_path(__path__, __name__)
__version__ = \"${VTK_VERSION_MAJOR}.${VTK_VERSION_MINOR}.${VTK_VERSION_PATCH}\"
")

# Install dependencies!
# Different modules have different dependencies, but let's install them all...
if(WIN32)
  find_package(LibArchive REQUIRED)
  string(REPLACE ".lib" ".dll" LibArchive_DLL ${LibArchive_LIBRARY})
  string(REPLACE "/lib/" "/bin/" LibArchive_DLL ${LibArchive_DLL})

  find_package(JsonCpp REQUIRED)
  string(REPLACE ".lib" ".dll" JsonCpp_DLL ${JsonCpp_LIBRARY})
  string(REPLACE "/lib/" "/bin/" JsonCpp_DLL ${JsonCpp_DLL})
endif()

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
