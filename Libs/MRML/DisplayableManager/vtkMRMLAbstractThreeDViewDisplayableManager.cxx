/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// MRMLDisplayableManager includes
#include "vtkMRMLAbstractThreeDViewDisplayableManager.h"

// MRML includes
#include <vtkMRMLViewNode.h>
#include <vtkMRMLInteractionEventData.h>

// VTK includes
#include <vtkCamera.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyle3D.h>

// STD includes
#include <cassert>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLAbstractThreeDViewDisplayableManager);

//----------------------------------------------------------------------------
// vtkMRMLAbstractThreeDViewDisplayableManager methods

//----------------------------------------------------------------------------
vtkMRMLAbstractThreeDViewDisplayableManager::vtkMRMLAbstractThreeDViewDisplayableManager() = default;

//----------------------------------------------------------------------------
vtkMRMLAbstractThreeDViewDisplayableManager::~vtkMRMLAbstractThreeDViewDisplayableManager() = default;

//----------------------------------------------------------------------------
void vtkMRMLAbstractThreeDViewDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkMRMLAbstractThreeDViewDisplayableManager::OnMRMLDisplayableNodeModifiedEvent(vtkObject* caller)
{
  assert(vtkMRMLViewNode::SafeDownCast(caller));
#ifndef _DEBUG
  (void)caller;
#endif
  this->OnMRMLViewNodeModifiedEvent();
}

//---------------------------------------------------------------------------
vtkMRMLViewNode* vtkMRMLAbstractThreeDViewDisplayableManager::GetMRMLViewNode()
{
  return vtkMRMLViewNode::SafeDownCast(this->GetMRMLDisplayableNode());
}

//---------------------------------------------------------------------------
void vtkMRMLAbstractThreeDViewDisplayableManager::PassThroughInteractorStyleEvent(int eventid)
{
  vtkInteractorStyle3D* interactorStyle = vtkInteractorStyle3D::SafeDownCast(this->GetInteractor()->GetInteractorStyle());

  if (interactorStyle)
  {

    switch (eventid)
    {
      case vtkCommand::ExposeEvent: interactorStyle->OnExpose(); break;

      case vtkCommand::ConfigureEvent: interactorStyle->OnConfigure(); break;

      case vtkCommand::EnterEvent: interactorStyle->OnEnter(); break;

      case vtkCommand::LeaveEvent: interactorStyle->OnLeave(); break;

      case vtkCommand::TimerEvent: interactorStyle->OnTimer(); break;

      case vtkCommand::MouseMoveEvent: interactorStyle->OnMouseMove(); break;

      case vtkCommand::LeftButtonPressEvent: interactorStyle->OnLeftButtonDown(); break;

      case vtkCommand::LeftButtonReleaseEvent: interactorStyle->OnLeftButtonUp(); break;

      case vtkCommand::MiddleButtonPressEvent: interactorStyle->OnMiddleButtonDown(); break;

      case vtkCommand::MiddleButtonReleaseEvent: interactorStyle->OnMiddleButtonUp(); break;

      case vtkCommand::RightButtonPressEvent: interactorStyle->OnRightButtonDown(); break;

      case vtkCommand::RightButtonReleaseEvent: interactorStyle->OnRightButtonUp(); break;

      case vtkCommand::MouseWheelForwardEvent: interactorStyle->OnMouseWheelForward(); break;

      case vtkCommand::MouseWheelBackwardEvent: interactorStyle->OnMouseWheelBackward(); break;

      case vtkCommand::KeyPressEvent:
        interactorStyle->OnKeyDown();
        interactorStyle->OnKeyPress();
        break;

      case vtkCommand::KeyReleaseEvent:
        interactorStyle->OnKeyUp();
        interactorStyle->OnKeyRelease();
        break;

      case vtkCommand::CharEvent: interactorStyle->OnChar(); break;

      case vtkCommand::DeleteEvent: interactorStyle->SetInteractor(nullptr); break;

      case vtkCommand::TDxMotionEvent:
      case vtkCommand::TDxButtonPressEvent:
      case vtkCommand::TDxButtonReleaseEvent: interactorStyle->DelegateTDxEvent(eventid, nullptr); break;
    }

    return;
  }
}

//---------------------------------------------------------------------------
double vtkMRMLAbstractThreeDViewDisplayableManager::GetViewScaleFactorAtPosition(vtkRenderer* renderer, double positionWorld[3], vtkMRMLInteractionEventData* interactionEventData)
{
  double viewScaleFactorMmPerPixel = 1.0;
  if (!renderer || !renderer->GetActiveCamera())
  {
    return viewScaleFactorMmPerPixel;
  }

  vtkCamera* cam = renderer->GetActiveCamera();
  if (cam->GetParallelProjection())
  {
    // Viewport: xmin, ymin, xmax, ymax; range: 0.0-1.0; origin is bottom left
    // Determine the available renderer size in pixels
    double minX = 0;
    double minY = 0;
    renderer->NormalizedDisplayToDisplay(minX, minY);
    double maxX = 1;
    double maxY = 1;
    renderer->NormalizedDisplayToDisplay(maxX, maxY);
    int rendererSizeInPixels[2] = { static_cast<int>(maxX - minX), static_cast<int>(maxY - minY) };
    // Parallel scale: height of the viewport in world-coordinate distances.
    // Larger numbers produce smaller images.
    viewScaleFactorMmPerPixel = (cam->GetParallelScale() * 2.0) / double(rendererSizeInPixels[1]);
  }
  else
  {
    const double cameraFP[] = { positionWorld[0], positionWorld[1], positionWorld[2], 1.0 };
    double cameraViewUp[3] = { 0 };
    cam->GetViewUp(cameraViewUp);
    vtkMath::Normalize(cameraViewUp);

    // these should be const but that doesn't compile under VTK 8
    double topCenterWorld[] = { cameraFP[0] + cameraViewUp[0], cameraFP[1] + cameraViewUp[1], cameraFP[2] + cameraViewUp[2], cameraFP[3] };
    double bottomCenterWorld[] = { cameraFP[0] - cameraViewUp[0], cameraFP[1] - cameraViewUp[1], cameraFP[2] - cameraViewUp[2], cameraFP[3] };

    double topCenterDisplay[4];
    double bottomCenterDisplay[4];

    // the WorldToDisplay in interactionEventData is faster if someone has already
    // called it once
    if (interactionEventData)
    {
      interactionEventData->WorldToDisplay(topCenterWorld, topCenterDisplay);
      interactionEventData->WorldToDisplay(bottomCenterWorld, bottomCenterDisplay);
    }
    else
    {
      std::copy(std::begin(topCenterWorld), std::end(topCenterWorld), std::begin(topCenterDisplay));
      renderer->WorldToDisplay(topCenterDisplay[0], topCenterDisplay[1], topCenterDisplay[2]);
      topCenterDisplay[2] = 0.0;

      std::copy(std::begin(bottomCenterWorld), std::end(bottomCenterWorld), std::begin(bottomCenterDisplay));
      renderer->WorldToDisplay(bottomCenterDisplay[0], bottomCenterDisplay[1], bottomCenterDisplay[2]);
      bottomCenterDisplay[2] = 0.0;
    }

    const double distInPixels = sqrt(vtkMath::Distance2BetweenPoints(topCenterDisplay, bottomCenterDisplay));
    // if render window is not initialized yet then distInPixels == 0.0,
    // in that case just leave the default viewScaleFactorMmPerPixel
    if (distInPixels > 1e-3)
    {
      // 2.0 = 2x length of viewUp vector in mm (because viewUp is unit vector)
      viewScaleFactorMmPerPixel = 2.0 / distInPixels;
    }
  }
  return viewScaleFactorMmPerPixel;
}
