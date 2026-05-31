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

#include "vtkMRMLModelDMPipelineCreatorLogic.h"

// Module includes
#include "vtkMRMLModelDMPipeline.h"
#include "vtkMRMLModelSliceDMPipeline.h"

// LayerDM includes
#include "vtkMRMLLayerDMPipelineCreateHelper.h"
#include "vtkMRMLLayerDMPipelineCreatorI.h"
#include "vtkMRMLLayerDMPipelineFactory.h"
#include "vtkSlicerLayerDMLogic.h"

// MRML includes
#include "vtkMRMLModelDisplayNode.h"
#include "vtkMRMLSliceNode.h"
#include "vtkMRMLViewNode.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

vtkStandardNewMacro(vtkMRMLModelDMPipelineCreatorLogic);

void vtkMRMLModelDMPipelineCreatorLogic::RegisterPipelines()
{
  static vtkSmartPointer<vtkMRMLLayerDMPipelineCreatorI> creator{ nullptr };
  if (creator)
  {
    return;
  }

  creator = vtkMRMLLayerDMPipelineFactory::GetInstance()->AddPipelineCreator(
    [](vtkMRMLAbstractViewNode* viewNode, vtkMRMLNode* displayNode)
    {
      return layer_dm::TryCreate<vtkMRMLSliceNode,            // Slice view DM pipeline
                                 vtkMRMLModelDisplayNode,     //
                                 vtkMRMLModelSliceDMPipeline, //
                                 vtkMRMLViewNode,             // 3D view DM pipeline
                                 vtkMRMLModelDisplayNode,     //
                                 vtkMRMLModelDMPipeline       //
                                 >(viewNode, displayNode);
    });
}
