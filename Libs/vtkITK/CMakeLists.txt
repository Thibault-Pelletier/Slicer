# --------------------------------------------------------------------------
# Options
# --------------------------------------------------------------------------
if(NOT DEFINED VTKITK_BUILD_DICOM_SUPPORT)
  option(VTKITK_BUILD_DICOM_SUPPORT "Build with DICOM support" ON)
endif()

# --------------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------------

#
# ITK
#
set(itk_components
  GrowCut
  ITKAnisotropicSmoothing
  ITKCommon
  ITKConnectedComponents
  ITKDistanceMap
  ITKIODCMTK
  ITKIOGDCM
  ITKIOGE
  ITKIOImageBase
  ITKIOMeta
  ITKIONIFTI
  ITKIONRRD
  ITKImageCompose
  ITKImageFilterBase
  ITKImageFunction
  ITKImageGrid
  ITKLabelMap
  ITKGDCM
  ITKPath
  ITKRegionGrowing
  ITKThresholding
  ITKVTK
  ITKVtkGlue
  # Needed for ITKv5 so that SimpleFastMutexLock is an alias to std::mutex in ITKv5
  # once ITKv4 is not supported, then we can simply replace SimpleFastMutexLock with std::mutex
  # the use the DeprecatedLib allows building without many preprocessor conditionals
  # for ITKv5/ITKv4 differences.
  ITKDeprecated
  MorphologicalContourInterpolation
)

find_package(ITK 5.0 COMPONENTS ${itk_components} REQUIRED)
include(${ITK_USE_FILE})

#
# vtkAddon
#
find_package(vtkAddon REQUIRED)

# --------------------------------------------------------------------------
# Configure headers
# --------------------------------------------------------------------------
set(configure_header_file vtkITKConfigure.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${configure_header_file}.in
  ${CMAKE_CURRENT_BINARY_DIR}/${configure_header_file}
  )

set(classes
  vtkITKArchetypeDiffusionTensorImageReaderFile
  vtkITKArchetypeImageSeriesReader
  vtkITKArchetypeImageSeriesScalarReader
  vtkITKArchetypeImageSeriesVectorReaderFile
  vtkITKArchetypeImageSeriesVectorReaderSeries
  vtkITKImageThresholdCalculator
  vtkITKImageWriter
  vtkITKGradientAnisotropicDiffusionImageFilter
  vtkITKGrowCut
  vtkITKDistanceTransform
  vtkITKLabelShapeStatistics
  vtkITKLevelTracingImageFilter
  vtkITKLevelTracing3DImageFilter
  vtkITKIslandMath
  vtkITKImageMargin
  vtkITKMorphologicalContourInterpolator
)

set(sources
  vtkITKNumericTraits.cxx
  vtkITKImageToImageFilter.h # put as source to ensure their symbols are defined
  vtkITKImageToImageFilterFF.h
  vtkITKImageToImageFilter2DFF.h
  vtkITKImageToImageFilterFUL.h
  vtkITKImageToImageFilterSS.h
  vtkITKImageToImageFilterULUL.h
  vtkITKImageToImageFilterUSUL.h
  vtkITKImageToImageFilterUSUS.h
  vtkITKImageToImageFilterUSF.h
)

set(headers
  vtkITKNumericTraits.h
  ${CMAKE_CURRENT_BINARY_DIR}/${configure_header_file}
)

vtk_module_add_module(VTK::ITK
  EXPORT_MACRO_PREFIX VTK_ITK
  CLASSES ${classes}
  SOURCES ${sources}
  HEADERS ${headers}
)

set_source_files_properties(
  vtkITKNumericTraits.cxx
  WRAP_EXCLUDE
)

vtk_module_link(VTK::ITK PUBLIC ${ITK_LIBRARIES} vtkAddon)
vtk_module_include(VTK::ITK PUBLIC $<BUILD_INTERFACE:${ITK_INCLUDE_DIRS} ${vtkAddon_INCLUDE_DIRS}>)
