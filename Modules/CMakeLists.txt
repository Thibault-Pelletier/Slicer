# "Core" folder is included in the top-level CMakeLists.txt just before "Base/QTApp". Doing so
# make it possible for QTApp to use on the Core module factory.

if(Slicer_BUILD_QTLOADABLEMODULES)
  add_subdirectory(Loadable)

  # Copy to build and install trees the Terminologies module JSON files.
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Loadable/Terminologies/Resources/SegmentationCategoryTypeModifier-DICOM-Master.json
    ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/Terminologies/SegmentationCategoryTypeModifier-DICOM-Master.term.json
    COPYONLY)
  install(
    FILES ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/Terminologies/SegmentationCategoryTypeModifier-DICOM-Master.term.json
    DESTINATION ${Slicer_INSTALL_QTLOADABLEMODULES_SHARE_DIR}/Terminologies COMPONENT Runtime)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Loadable/Terminologies/Resources/SegmentationCategoryTypeModifier-SlicerGeneralAnatomy.json
    ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/Terminologies/SegmentationCategoryTypeModifier-SlicerGeneralAnatomy.term.json
    COPYONLY)
  install(
    FILES ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/Terminologies/SegmentationCategoryTypeModifier-SlicerGeneralAnatomy.term.json
    DESTINATION ${Slicer_INSTALL_QTLOADABLEMODULES_SHARE_DIR}/Terminologies COMPONENT Runtime)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Loadable/Terminologies/Resources/AnatomicRegionAndModifier-DICOM-Master.json
    ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/Terminologies/AnatomicRegionAndModifier-DICOM-Master.term.json
    COPYONLY)
  install(
    FILES ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/Terminologies/AnatomicRegionAndModifier-DICOM-Master.term.json
    DESTINATION ${Slicer_INSTALL_QTLOADABLEMODULES_SHARE_DIR}/Terminologies COMPONENT Runtime)

  # Copy to build and install trees the Volumes module JSON files.
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Loadable/Volumes/Resources/VolumeDisplayPresets.json
    ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/Volumes/VolumeDisplayPresets.json
    COPYONLY)
  install(
    FILES ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/Volumes/VolumeDisplayPresets.json
    DESTINATION ${Slicer_INSTALL_QTLOADABLEMODULES_SHARE_DIR}/Volumes COMPONENT Runtime)
endif()

if(Slicer_BUILD_QTSCRIPTEDMODULES)
  add_subdirectory(Scripted)
endif()

if(Slicer_BUILD_CLI_SUPPORT AND Slicer_BUILD_CLI)
  add_subdirectory(CLI)
endif()
