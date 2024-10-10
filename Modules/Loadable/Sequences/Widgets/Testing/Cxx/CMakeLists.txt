set(KIT ${PROJECT_NAME})

#-----------------------------------------------------------------------------
set(INPUT ${CMAKE_SOURCE_DIR}/Libs/MRML/Core/Testing)

#-----------------------------------------------------------------------------
set(KIT_TEST_SRCS
  qMRMLSequenceBrowserWidgetsTest1.cxx
  qMRMLSequenceEditWidgetTest1.cxx
  )

#-----------------------------------------------------------------------------
slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
simple_test( qMRMLSequenceBrowserWidgetsTest1 )
simple_test( qMRMLSequenceEditWidgetTest1 )
