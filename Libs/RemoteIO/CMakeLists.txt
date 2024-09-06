# --------------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------------

find_package(CURL REQUIRED)

if(Slicer_USE_PYTHONQT_WITH_OPENSSL)
  find_package(OpenSSL REQUIRED)
endif()

# --------------------------------------------------------------------------
# Configure headers
# --------------------------------------------------------------------------

set(configure_header_file vtkRemoteIOConfigure.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${configure_header_file}.in
  ${CMAKE_CURRENT_BINARY_DIR}/${configure_header_file}
  )

# --------------------------------------------------------------------------
# Build library
# --------------------------------------------------------------------------

set(classes vtkHTTPHandler)
set(headers ${CMAKE_CURRENT_BINARY_DIR}/${configure_header_file})

vtk_module_add_module(VTK::RemoteIO
  EXPORT_MACRO_PREFIX VTK_RemoteIO
  CLASSES ${classes}
  HEADERS ${headers}
)

set(libs MRMLCore ${CURL_LIBRARIES})
set(includes ${CURL_INCLUDE_DIRS})

if(Slicer_USE_PYTHONQT_WITH_OPENSSL)
  list(APPEND libs ${OPENSSL_LIBRARIES})
  list(APPEND includes ${OPENSSL_INCLUDE_DIRS})
endif()

vtk_module_link(VTK::RemoteIO PUBLIC ${libs})
vtk_module_include(VTK::RemoteIO PUBLIC $<BUILD_INTERFACE:${includes}>)
