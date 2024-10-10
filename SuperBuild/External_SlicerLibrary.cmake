
set(proj SlicerLibrary)

# Set dependency list
set(${proj}_DEPENDENCIES
  curl
  ITK
  JsonCpp
  LibArchive
  OpenSSL
  ParameterSerializer
  RapidJSON
  SlicerExecutionModel
  teem
  tbb
  vtkAddon
  VTK
)

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(NOT DEFINED SlicerLibrary_DIR AND NOT Slicer_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/AlexyPellegrini/VTKExternalModule.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_TAG
    "multiple-modules"
    QUIET
    )

  set(EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS)
  if(Slicer_USE_PYTHONQT)
    list(APPEND EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS
      # Required by FindPython3 CMake module used by VTK
      -DPython3_ROOT_DIR:PATH=${Python3_ROOT_DIR}
      -DPython3_INCLUDE_DIR:PATH=${Python3_INCLUDE_DIR}
      -DPython3_LIBRARY:FILEPATH=${Python3_LIBRARY}
      -DPython3_LIBRARY_DEBUG:FILEPATH=${Python3_LIBRARY_DEBUG}
      -DPython3_LIBRARY_RELEASE:FILEPATH=${Python3_LIBRARY_RELEASE}
      -DPython3_EXECUTABLE:FILEPATH=${Python3_EXECUTABLE}
      )
  endif()

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
      -DVTK_MODULE_NAME:STRING=SlicerLibrary
      -DVTK_MODULE_SOURCE_DIR:PATH=${CMAKE_SOURCE_DIR}
      -DVTK_WRAP_PYTHON:BOOL=${Slicer_USE_PYTHONQT}
      -DVTK_INSTALL_RUNTIME_DIR:PATH=${Slicer_INSTALL_BIN_DIR}
      -DVTK_INSTALL_LIBRARY_DIR:PATH=${Slicer_INSTALL_LIB_DIR}
      -DVTK_INSTALL_ARCHIVE_DIR:PATH=${Slicer_INSTALL_LIB_DIR}
      -DSlicer_USE_PYTHONQT_WITH_OPENSSL:BOOL=ON
      # dependencies
      -DCURL_INCLUDE_DIR:PATH=${CURL_INCLUDE_DIR}
      -DCURL_LIBRARY:FILEPATH=${CURL_LIBRARY}
      -DITK_DIR:PATH=${ITK_DIR}
      -DJsonCpp_INCLUDE_DIR:PATH=${JsonCpp_INCLUDE_DIR}
      -DJsonCpp_LIBRARY:PATH=${JsonCpp_LIBRARY}
      -DLibArchive_INCLUDE_DIR:PATH=${LibArchive_INCLUDE_DIR}
      -DLibArchive_LIBRARY:FILEPATH=${LibArchive_LIBRARY}
      -DParameterSerializer:PATH=${ParameterSerializer_DIR}
      -DRapidJSON_INCLUDE_DIR:PATH=${RapidJSON_INCLUDE_DIR}
      -DRapidJSON_DIR:PATH=${RapidJSON_DIR}
      -DSlicerExecutionModel_DIR:PATH=${SlicerExecutionModel_DIR}
      -DTeem_DIR:PATH=${Teem_DIR}
      -DTBB_BIN_DIR:PATH=${TBB_BIN_DIR}
      -DTBB_LIB_DIR:PATH=${TBB_LIB_DIR}
      -DvtkAddon_DIR:PATH=${vtkAddon_DIR}
      -DVTK_DIR:PATH=${VTK_DIR}
      ${EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS}
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

  #ExternalProject_GenerateProjectDescription_Step(${proj})

  set(vtkSlicerLibrary_DIR ${EP_BINARY_DIR})

  # Add path to SlicerLauncherSettings.ini
  set(_library_output_subdir bin)
  if(UNIX)
    set(_library_output_subdir lib)
  endif()
  set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD ${vtkSlicerLibrary_DIR}/${_library_output_subdir}/<CMAKE_CFG_INTDIR>)
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    LABELS "LIBRARY_PATHS_LAUNCHER_BUILD"
    )

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

mark_as_superbuild(
  VARS vtkSlicerLibrary_DIR:PATH
  LABELS "FIND_PACKAGE"
  )
