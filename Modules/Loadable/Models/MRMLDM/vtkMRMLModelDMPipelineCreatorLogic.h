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

#include "vtkSlicerModelsModuleMRMLDisplayableManagerExport.h"

// VTK includes
#include <vtkObject.h>

class VTK_SLICER_MODELS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLModelDMPipelineCreatorLogic : public vtkObject
{
public:
  static vtkMRMLModelDMPipelineCreatorLogic* New();
  vtkTypeMacro(vtkMRMLModelDMPipelineCreatorLogic, vtkObject);

  static void RegisterPipelines();

protected:
  vtkMRMLModelDMPipelineCreatorLogic() = default;
  ~vtkMRMLModelDMPipelineCreatorLogic() override = default;
};
