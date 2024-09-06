
set(proj teem)

# Set dependency list
set(${proj}_DEPENDENCIES zlib)
if(NOT Slicer_USE_SYSTEM_teem)
  list(APPEND teem_DEPENDENCIES ${VTK_EXTERNAL_NAME})
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(Slicer_USE_SYSTEM_${proj})
  unset(Teem_DIR CACHE)
  find_package(Teem REQUIRED NO_MODULE)
endif()

# Sanity checks
if(DEFINED Teem_DIR AND NOT EXISTS ${Teem_DIR})
  message(FATAL_ERROR "Teem_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED Teem_DIR AND NOT Slicer_USE_SYSTEM_${proj})
  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/Slicer/teem"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_TAG
    "e4746083c0e1dc0c137124c41eca5d23adf73bfa"
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
      # Not needed -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DBUILD_TESTING:BOOL=OFF
      -DBUILD_SHARED_LIBS:BOOL=OFF
      -DTeem_USE_LIB_INSTALL_SUBDIR:BOOL=ON
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
      -DTeem_PTHREAD:BOOL=OFF
      -DTeem_BZIP2:BOOL=OFF
      -DTeem_ZLIB:BOOL=ON
      -DTeem_PNG:BOOL=OFF
      -DZLIB_ROOT:PATH=${ZLIB_ROOT}
      -DZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR}
      -DZLIB_LIBRARY:FILEPATH=${ZLIB_LIBRARY}
      -DTeem_VTK_MANGLE:BOOL=OFF ## NOT NEEDED FOR EXTERNAL ZLIB outside of vtk
      -DPNG_PNG_INCLUDE_DIR:PATH=${PNG_INCLUDE_DIR}
      -DTeem_PNG_DLLCONF_IPATH:PATH=${VTK_DIR}/Utilities
      ${EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS}
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

  set(Teem_DIR ${EP_BINARY_DIR})
else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

mark_as_superbuild(
  VARS Teem_DIR:PATH
  LABELS "FIND_PACKAGE"
  )
