set(MRML_APPLICATION_NAME "SlicerLib" CACHE STRING "Current MRML scene creator application name" )
mark_as_advanced(MRML_APPLICATION_NAME)

# The format is: 0xXXYYZZ where XX is the major version, YY the minor and
# ZZ the patch version.
# This format is chosen to allow easy comparison in C++ precompiler macros.
set(MRML_APPLICATION_VERSION 0x040000 CACHE STRING "Current MRML application version" )
mark_as_advanced(MRML_APPLICATION_VERSION)

# Revision can be used to identify version with finer granularity than version.
# It is an integer number that is incremented with each new software revision.
set(MRML_APPLICATION_REVISION 0 CACHE STRING "Current MRML application revision number" )
mark_as_advanced(MRML_APPLICATION_REVISION)

# MRML scenes that are older than MRML_APPLICATION_SUPPORT_VERSION won't be supported.
# Or said differently, MRML_APPLICATION_SUPPORT_VERSION is the earliest version supported
# for a MRML scene.
# The format is: 0xXXYYZZ where XX is the major version, YY the minor and
# ZZ the patch version.
# For example, set MRML_APPLICATION_SUPPORT_VERSION to 0x040000 if you don't want to
# support scenes of Slicer 3.
# Supports Slicer3 and Slicer4 scenes by default.
set(MRML_APPLICATION_SUPPORT_VERSION 0x030000 CACHE STRING "MRML application version to support" )
mark_as_advanced(MRML_APPLICATION_SUPPORT_VERSION)

option(MRML_USE_vtkTeem "Build MRML with vtkTeem support." ON)
mark_as_advanced(MRML_USE_vtkTeem)

# --------------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------------

#
# ITK
#
set(${PROJECT_NAME}_ITK_COMPONENTS
  ITKCommon
  ITKIOImageBase
  ITKIONRRD
  ITKIOSpatialObjects
  ITKIOTransformBase
  ITKMesh
  ITKTransform
  )
find_package(ITK 4.6 COMPONENTS ${${PROJECT_NAME}_ITK_COMPONENTS} REQUIRED)
include(${ITK_USE_FILE})

#
# LibArchive
#
find_package(LibArchive REQUIRED MODULE)

#
# vtkAddon
#
find_package(vtkAddon REQUIRED)

# --------------------------------------------------------------------------
# Configure headers
# --------------------------------------------------------------------------
set(configure_header_file vtkMRMLConfigure.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${configure_header_file}.in
  ${CMAKE_CURRENT_BINARY_DIR}/${configure_header_file}
  )

# --------------------------------------------------------------------------
# Sources
# --------------------------------------------------------------------------
set(classes
  vtkArchive
  vtkCodedEntry
  vtkEventBroker
  vtkDataFileFormatHelper
  vtkMRMLI18N
  vtkMRMLMeasurement
  vtkMRMLStaticMeasurement
  vtkMRMLLogic
  vtkMRMLAbstractLayoutNode
  vtkMRMLAbstractViewNode
  vtkMRMLCameraNode
  vtkMRMLClipModelsNode
  vtkMRMLColorNode
  vtkMRMLColors
  vtkMRMLColorTableNode
  vtkMRMLColorTableStorageNode
  vtkMRMLCoreTestingUtilities
  vtkMRMLCrosshairNode
  vtkMRMLDiffusionTensorDisplayPropertiesNode
  vtkMRMLDiffusionWeightedVolumeDisplayNode
  vtkMRMLDiffusionWeightedVolumeNode
  vtkMRMLDisplayNode
  vtkMRMLDisplayableNode
  vtkMRMLFolderDisplayNode
  vtkMRMLHierarchyNode
  vtkMRMLHierarchyStorageNode
  vtkMRMLDisplayableHierarchyNode
  vtkMRMLInteractionNode
  vtkMRMLLabelMapVolumeDisplayNode
  vtkMRMLLabelMapVolumeNode
  vtkMRMLLinearTransformNode
  vtkMRMLLinearTransformSequenceStorageNode
  vtkMRMLMarkupsStorageNode
  vtkMRMLMessageCollection
  vtkMRMLModelDisplayNode
  vtkMRMLModelHierarchyNode
  vtkMRMLModelNode
  vtkMRMLModelStorageNode
  vtkMRMLNode
  vtkMRMLParser
  vtkMRMLPlotChartNode
  vtkMRMLPlotSeriesNode
  vtkMRMLPlotViewNode
  vtkMRMLProceduralColorNode
  vtkMRMLProceduralColorStorageNode
  vtkMRMLROIListNode
  vtkMRMLROINode
  vtkMRMLScalarVolumeDisplayNode
  vtkMRMLScalarVolumeNode
  vtkMRMLScene
  vtkMRMLSceneViewNode
  vtkMRMLSceneViewStorageNode
  vtkMRMLScriptedModuleNode
  vtkMRMLSegmentationNode
  vtkMRMLSegmentationDisplayNode
  vtkMRMLSegmentationStorageNode
  vtkMRMLSequenceNode
  vtkMRMLSequenceStorageNode
  vtkMRMLSelectionNode
  vtkMRMLSliceCompositeNode
  vtkMRMLSliceDisplayNode
  vtkMRMLSliceNode
  vtkMRMLSnapshotClipNode
  vtkMRMLStorableNode
  vtkMRMLStorageNode
  vtkMRMLSubjectHierarchyNode
  vtkMRMLSubjectHierarchyLegacyNode
  vtkMRMLTableNode
  vtkMRMLTableStorageNode
  vtkMRMLTableSQLiteStorageNode
  vtkMRMLTableViewNode
  vtkMRMLTextNode
  vtkMRMLTextStorageNode
  vtkMRMLTransformNode
  vtkMRMLTransformStorageNode
  vtkMRMLTransformDisplayNode
  vtkMRMLTransformableNode
  vtkMRMLUnitNode
  vtkMRMLVectorVolumeDisplayNode
  vtkMRMLViewNode
  vtkMRMLVolumeArchetypeStorageNode
  vtkMRMLVolumeDisplayNode
  vtkMRMLGlyphableVolumeDisplayNode
  vtkMRMLGlyphableVolumeSliceDisplayNode
  vtkMRMLVolumeHeaderlessStorageNode
  vtkMRMLVolumeNode
  vtkMRMLVolumeSequenceStorageNode
  vtkObservation
  vtkObserverManager
  vtkMRMLLayoutNode
  # Classes for remote data handling:
  vtkCacheManager
  vtkDataIOManager
  vtkDataTransfer
  vtkMRMLBSplineTransformNode
  vtkMRMLGridTransformNode
  vtkPermissionPrompter
  vtkURIHandler
  vtkTagTableCollection
  vtkTagTable
  vtkMRMLdGEMRICProceduralColorNode
  vtkMRMLPETProceduralColorNode
  # vtkTeem
  vtkMRMLDiffusionImageVolumeNode
  vtkMRMLDiffusionTensorVolumeDisplayNode
  vtkMRMLDiffusionTensorVolumeNode
  vtkMRMLDiffusionTensorVolumeSliceDisplayNode
  vtkMRMLNRRDStorageNode
  vtkMRMLTensorVolumeNode
  vtkMRMLVectorVolumeNode
  vtkMRMLStreamingVolumeNode
  )

set(headers
  vtkMRMLTranslator.h
  vtkMRMLSubjectHierarchyConstants.h
  ${CMAKE_CURRENT_BINARY_DIR}/${configure_header_file})

set_source_files_properties(
  vtkMRMLCoreTestingUtilities.cxx
  WRAP_EXCLUDE
  )

# Abstract/pure virtual classes

set_source_files_properties(
  vtkMRMLNode.cxx
  vtkMRMLVolumeNode.cxx
  vtkMRMLTransformNode.cxx
  vtkMRMLDisplayNode.cxx
  vtkMRMLDisplayableNode.cxx
  vtkMRMLVolumeDisplayNode.cxx
  ABSTRACT
  )

#-----------------------------------------------------------------------------
# Wrapping hints used by all Tcl, Python and Java wrapping.
# It can also be used by external wrapping tools.
# --------------------------------------------------------------------------
set(VTK_WRAP_HINTS ${CMAKE_CURRENT_SOURCE_DIR}/Wrapping/Tcl/hints)

# --------------------------------------------------------------------------
# Build library
# --------------------------------------------------------------------------

vtk_module_add_module(VTK::MRMLCore
  EXPORT_MACRO_PREFIX VTK_MRML
  CLASSES ${classes}
  HEADERS ${headers}
)

vtk_module_link(VTK::MRMLCore PUBLIC ${ITK_LIBRARIES} ${LibArchive_LIBRARY} vtkAddon)
vtk_module_include(VTK::MRMLCore PUBLIC $<BUILD_INTERFACE:${ITK_INCLUDE_DIRS} ${LibArchive_INCLUDE_DIRS} ${vtkAddon_INCLUDE_DIRS}>)
