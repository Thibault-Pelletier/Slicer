
set(proj vtkAddon)

# Set dependency list
set(${proj}_DEPENDENCIES)
if(SLICERLIB_PYTHON_BUILD)
  # when building slicerlib with scikit-build-core, VTK is provided by vtk-sdk wheel!
  # vtk is found using CMAKE_PREFIX_PATH
  list(APPEND vtkAddon_CMAKE_CACHE_ARGS "-DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}")
  cmake_path(CONVERT Python_EXECUTABLE TO_CMAKE_PATH_LIST Python_EXECUTABLE NORMALIZE)
  list(APPEND vtkAddon_CMAKE_CACHE_ARGS "-DvtkAddon_WRAP_PYTHON:BOOL=ON")
else()
  list(APPEND ${proj}_DEPENDENCIES "VTK")
  list(APPEND vtkAddon_CMAKE_CACHE_ARGS "-DvtkAddon_WRAP_PYTHON:BOOL=${Slicer_USE_PYTHONQT}")
endif()

list(APPEND vtkAddon_CMAKE_CACHE_ARGS "-DPYTHON_EXECUTABLE:FILEPATH=${Python_EXECUTABLE}")

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(NOT DEFINED vtkAddon_DIR AND NOT Slicer_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/AlexyPellegrini/vtkAddon.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_TAG
    "win32-export"
    QUIET
    )

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${Slicer_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${Slicer_${proj}_GIT_TAG}"
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DBUILD_TESTING:BOOL=OFF
      -DvtkAddon_USE_UTF8:BOOL=ON
      -DvtkAddon_CMAKE_DIR:PATH=${EP_SOURCE_DIR}/CMake
      -DvtkAddon_LAUNCH_COMMAND:STRING=${Slicer_LAUNCH_COMMAND}
      -DVTK_DIR=${VTK_DIR}
      ${vtkAddon_CMAKE_CACHE_ARGS}
      ${EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS}
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

  #ExternalProject_GenerateProjectDescription_Step(${proj})

  set(vtkAddon_DIR ${EP_BINARY_DIR})

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

mark_as_superbuild(
  VARS vtkAddon_DIR:PATH
  LABELS "FIND_PACKAGE"
  )
