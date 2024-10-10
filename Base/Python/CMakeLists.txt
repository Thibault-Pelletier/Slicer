
set(Slicer_PYTHON_SCRIPTS
  slicer/__init__
  slicer/i18n
  slicer/logic
  slicer/parameterNodeWrapper/__init__
  slicer/parameterNodeWrapper/default
  slicer/parameterNodeWrapper/guiCreation
  slicer/parameterNodeWrapper/guiConnectors
  slicer/parameterNodeWrapper/parameterInfo
  slicer/parameterNodeWrapper/parameterPack
  slicer/parameterNodeWrapper/serializers
  slicer/parameterNodeWrapper/types
  slicer/parameterNodeWrapper/util
  slicer/parameterNodeWrapper/validators
  slicer/parameterNodeWrapper/wrapper
  slicer/ScriptedLoadableModule
  slicer/slicerqt
  slicer/testing
  slicer/util
  mrml
  vtkAddon
  vtkITK
  vtkSegmentationCore
  vtkTeem
  compat/MRMLCLIPython.py
  compat/MRMLCorePython.py
  compat/MRMLDisplayableManagerPython.py
  compat/MRMLLogicPython.py
  compat/vtkITKPython.py
  compat/vtkSegmentationCorePython.py
  compat/vtkSlicerAnnotationsModuleLogicPython.py
  compat/vtkSlicerAnnotationsModuleMRMLPython.py
  compat/vtkSlicerBaseLogicPython.py
  compat/vtkSlicerCamerasModuleLogicPython.py
  compat/vtkSlicerColorsModuleLogicPython.py
  compat/vtkSlicerColorsModuleMRMLDisplayableManagerPython.py
  compat/vtkSlicerColorsModuleMRMLPython.py
  compat/vtkSlicerColorsModuleVTKWidgetsPython.py
  compat/vtkSlicerCropVolumeModuleLogicPython.py
  compat/vtkSlicerCropVolumeModuleMRMLPython.py
  compat/vtkSlicerDataModuleLogicPython.py
  compat/vtkSlicerMarkupsModuleLogicPython.py
  compat/vtkSlicerMarkupsModuleMRMLDisplayableManagerPython.py
  compat/vtkSlicerMarkupsModuleMRMLPython.py
  compat/vtkSlicerMarkupsModuleVTKWidgetsPython.py
  compat/vtkSlicerModelsModuleLogicPython.py
  compat/vtkSlicerPlotsModuleLogicPython.py
  compat/vtkSlicerReformatModuleLogicPython.py
  compat/vtkSlicerSceneViewsModuleLogicPython.py
  compat/vtkSlicerSegmentationsModuleLogicPython.py
  compat/vtkSlicerSegmentationsModuleMRMLDisplayableManagerPython.py
  compat/vtkSlicerSegmentationsModuleMRMLPython.py
  compat/vtkSlicerSequencesModuleLogicPython.py
  compat/vtkSlicerSequencesModuleMRMLPython.py
  compat/vtkSlicerSubjectHierarchyModuleLogicPython.py
  compat/vtkSlicerTablesModuleLogicPython.py
  compat/vtkSlicerTerminologiesModuleLogicPython.py
  compat/vtkSlicerTextsModuleLogicPython.py
  compat/vtkSlicerTransformsModuleLogicPython.py
  compat/vtkSlicerTransformsModuleMRMLDisplayableManagerPython.py
  compat/vtkSlicerUnitsModuleLogicPython.py
  compat/vtkSlicerViewControllersModuleLogicPython.py
  compat/vtkSlicerVolumeRenderingModuleLogicPython.py
  compat/vtkSlicerVolumeRenderingModuleMRMLDisplayableManagerPython.py
  compat/vtkSlicerVolumeRenderingModuleMRMLPython.py
  compat/vtkSlicerVolumesModuleLogicPython.py
  compat/vtkTeemPython.py
  )

set(Slicer_PYTHON_MODULES_CONFIG "
# slicer libs
'mrml',
'vtkAddon',
'vtkSegmentationCore',
'qMRMLWidgetsPythonQt',
# slicer base libs
'logic',
'qSlicerBaseQTCorePythonQt',
'qSlicerBaseQTGUIPythonQt',
'qSlicerBaseQTAppPythonQt'"
  )

if(Slicer_BUILD_CLI_SUPPORT)
  list(APPEND Slicer_PYTHON_SCRIPTS
    slicer/cli
    )
  set(Slicer_PYTHON_MODULES_CONFIG "${Slicer_PYTHON_MODULES_CONFIG},
# CLI logic (Slicer_BUILD_CLI_SUPPORT:ON)
'qSlicerBaseQTCLIPython',
'qSlicerBaseQTCLIPythonQt',
"
  )
endif()

configure_file(
  slicer/kits.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/slicer/kits.py
  @ONLY
  )

list(APPEND Slicer_PYTHON_SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/slicer/kits.py)

set(Slicer_PYTHON_RESOURCES
  )

if(Slicer_USE_SimpleITK)
  list(APPEND Slicer_PYTHON_SCRIPTS
    sitkUtils
    )
endif()

ctkMacroCompilePythonScript(
  TARGET_NAME SlicerBase
  SCRIPTS "${Slicer_PYTHON_SCRIPTS}"
  RESOURCES "${Slicer_PYTHON_RESOURCES}"
  DESTINATION_DIR ${Slicer_BINARY_DIR}/bin/Python
  INSTALL_DIR ${Slicer_INSTALL_BIN_DIR}
  )
