/*==============================================================================

  Program: 3D Slicer

  Copyright (c)

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#pragma once

// Export
#include "vtkSlicerModelsModuleMRMLDisplayableManagerExport.h"

// Layer DM includes
#include "vtkMRMLLayerDMPipelineI.h"

// VTK includes
#include <vtkSmartPointer.h>

class vtkActor2D;
class vtkDataSetSurfaceFilter;
class vtkGeneralTransform;
class vtkGeometryFilter;
class vtkLookupTable;
class vtkMRMLDisplayNode;
class vtkMRMLModelDisplayNode;
class vtkMRMLModelNode;
class vtkMapper;
class vtkPlane;
class vtkPlaneCutter;
class vtkPolyDataMapper2D;
class vtkProp;
class vtkProperty2D;
class vtkSampleImplicitFunctionFilter;
class vtkTransform;
class vtkTransformFilter;
class vtkTransformPolyDataFilter;

/// \brief Pipeline for displaying a model in a 2D slice view.
///
/// This pipeline handles the display of model meshes on slice planes, supporting three modes:
/// - Slice intersection (cut mesh along slice plane)
/// - Slice projection (project mesh onto slice plane)
/// - Distance-encoded projection (encode slice distance into scalar values)
class VTK_SLICER_MODELS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLModelSliceDMPipeline : public vtkMRMLLayerDMPipelineI
{
public:
  static vtkMRMLModelSliceDMPipeline* New();
  vtkTypeMacro(vtkMRMLModelSliceDMPipeline, vtkMRMLLayerDMPipelineI);

  void SetDisplayNode(vtkMRMLNode* displayNode) override;
  vtkMRMLModelDisplayNode* GetModelDisplayNode() const;
  vtkMRMLModelNode* GetModelNode() const;

  std::vector<vtkSmartPointer<vtkProp>> GetManagedProps() const;

  void OnRendererAdded(vtkRenderer* renderer) override;
  void OnRendererRemoved(vtkRenderer* renderer) override;

  void UpdatePipeline() override;

protected:
  vtkMRMLModelSliceDMPipeline();
  ~vtkMRMLModelSliceDMPipeline() override;

  void OnUpdate(vtkObject* obj, unsigned long eventId, void* callData) override;

private:
  vtkMRMLModelSliceDMPipeline(const vtkMRMLModelSliceDMPipeline&) = delete;
  void operator=(const vtkMRMLModelSliceDMPipeline&) = delete;

  template <typename T, typename U>
  T* EnsureTypeAs(vtkSmartPointer<U>& object)
  {
    if (!T::SafeDownCast(object))
    {
      object = T::New();
    }
    return T::SafeDownCast(object);
  }

  void UpdateModelNode();
  void SetModelNode(vtkMRMLModelNode* node);

  /// \brief Sets the slice plane from the current slice node's XY-to-RAS matrix.
  void UpdateSlicePlaneFromMatrix();

  /// \brief Returns true if slice intersection (not projection) mode is active.
  bool IsIntersectionMode() const;

  /// \brief Computes and sets the RAS-to-slice-plane transform for projection mode.
  void UpdateProjectionTransform() const;

  /// \brief Sets actor2D properties (color, opacity, thickness) based on display properties.
  void UpdateActor2DProperties(vtkProperty2D* prop, double opacity, bool visibility, vtkMRMLModelDisplayNode* modelDisplayNode) const;

  vtkWeakPointer<vtkMRMLModelNode> ModelNode;

  // Slice-specific VTK pipeline components
  vtkSmartPointer<vtkPlane> Plane;
  vtkSmartPointer<vtkGeneralTransform> NodeToWorld;
  vtkSmartPointer<vtkTransform> TransformToSlice;
  vtkSmartPointer<vtkTransformPolyDataFilter> Transformer;
  vtkSmartPointer<vtkDataSetSurfaceFilter> SurfaceExtractor;
  vtkSmartPointer<vtkTransformFilter> ModelWarper;
  vtkSmartPointer<vtkPlaneCutter> Cutter;
  vtkSmartPointer<vtkGeometryFilter> GeometryFilter;
  vtkSmartPointer<vtkSampleImplicitFunctionFilter> SliceDistance;
  vtkSmartPointer<vtkPolyDataMapper2D> Mapper;
  vtkSmartPointer<vtkActor2D> Actor;
};
