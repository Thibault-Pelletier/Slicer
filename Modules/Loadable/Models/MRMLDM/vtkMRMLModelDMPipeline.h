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

class vtkActor;
class vtkAlgorithm;
class vtkCapPolyData;
class vtkGeneralTransform;
class vtkImageActor;
class vtkImplicitFunction;
class vtkMRMLClipNode;
class vtkMRMLDisplayNode;
class vtkMRMLModelDisplayNode;
class vtkMRMLModelNode;
class vtkMapper;
class vtkPolyData;
class vtkProp;
class vtkTransform;
class vtkTransformFilter;

/// \brief Pipeline for displaying a model in a 3D view.
class VTK_SLICER_MODELS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLModelDMPipeline : public vtkMRMLLayerDMPipelineI
{
public:
  static vtkMRMLModelDMPipeline* New();
  vtkTypeMacro(vtkMRMLModelDMPipeline, vtkMRMLLayerDMPipelineI);

  void SetDisplayNode(vtkMRMLNode* displayNode) override;
  vtkMRMLModelDisplayNode* GetModelDisplayNode() const;
  vtkMRMLModelNode* GetModelNode() const;

  /// \brief Returns props managed by the pipeline.
  /// The list of props doesn't change after instantiation of this pipeline.
  std::vector<vtkSmartPointer<vtkProp>> GetManagedProps() const;

  void OnRendererAdded(vtkRenderer* renderer) override;
  void OnRendererRemoved(vtkRenderer* renderer) override;

  void UpdatePipeline() override;

protected:
  vtkMRMLModelDMPipeline();
  ~vtkMRMLModelDMPipeline() override;

  void OnUpdate(vtkObject* obj, unsigned long eventId, void* callData) override;

private:
  vtkMRMLModelDMPipeline(const vtkMRMLModelDMPipeline&) = delete;
  void operator=(const vtkMRMLModelDMPipeline&) = delete;

  /// \returns Hierarchy display node if override is enabled, current display node otherwise.
  vtkMRMLDisplayNode* GetOverrideDisplayNode() const;
  void UpdateModelNode();
  void SetModelNode(vtkMRMLModelNode* node);
  void UpdateClipper();
  bool HasClipping() const;
  void UpdateTransform() const;
  void UpdateMapperConnection();
  void UpdateDisplayProperty() const;
  void UpdateMapperProperties(vtkMapper* mapper) const;
  void UpdateActorProperties(vtkActor* actor, double opacity, bool visibility) const;
  void UpdateCapActorProperties(double opacity, bool visibility) const;

  vtkWeakPointer<vtkMRMLModelNode> ModelNode;
  vtkSmartPointer<vtkTransformFilter> TransformFilter;
  vtkSmartPointer<vtkTransform> Transform;
  vtkSmartPointer<vtkGeneralTransform> NonLinearTransform;
  vtkSmartPointer<vtkAlgorithm> Clipper;
  vtkSmartPointer<vtkCapPolyData> Capper;
  vtkSmartPointer<vtkMapper> Mapper;
  vtkSmartPointer<vtkActor> Actor;
  vtkSmartPointer<vtkImageActor> ImageActor;
  vtkSmartPointer<vtkMapper> CapMapper;
  vtkSmartPointer<vtkActor> CapActor;
};
