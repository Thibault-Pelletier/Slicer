/*=========================================================================

 Copyright (c) ProxSim ltd., Kwun Tong, Hong Kong. All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 This file was originally developed by Davide Punzo, punzodavide@hotmail.it,
 and development was supported by ProxSim ltd.

=========================================================================*/

/**
 * @class   vtkSlicerLineRepresentation3D
 * @brief   Default representation for the line widget
 *
 * This class provides the default concrete representation for the
 * vtkMRMLAbstractWidget. See vtkMRMLAbstractWidget
 * for details.
 * @sa
 * vtkSlicerMarkupsWidgetRepresentation3D vtkMRMLAbstractWidget
 */

#ifndef vtkSlicerLineRepresentation3D_h
#define vtkSlicerLineRepresentation3D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerMarkupsWidgetRepresentation3D.h"

class vtkActor;
class vtkPolyDataMapper;
class vtkPolyData;
class vtkTubeFilter;

class vtkMRMLInteractionEventData;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerLineRepresentation3D : public vtkSlicerMarkupsWidgetRepresentation3D
{
public:
  /// Instantiate this class.
  static vtkSlicerLineRepresentation3D* New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerLineRepresentation3D, vtkSlicerMarkupsWidgetRepresentation3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Subclasses of vtkMRMLAbstractWidgetRepresentation must implement these methods. These
  /// are the methods that the widget and its representation use to
  /// communicate with each other.
  void UpdateFromMRMLInternal(vtkMRMLNode* caller, unsigned long event, void* callData = nullptr) override;

  void CanInteract(vtkMRMLInteractionEventData* interactionEventData, int& foundComponentType, int& foundComponentIndex, double& closestDistance2) override;

  /// Methods to make this class behave as a vtkProp.
  void GetActors(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport* viewport) override;
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;

  /// Return the bounds of the representation
  double* GetBounds() VTK_SIZEHINT(6) override;

protected:
  vtkSlicerLineRepresentation3D();
  ~vtkSlicerLineRepresentation3D() override;

  vtkSmartPointer<vtkPolyData> Line;
  vtkSmartPointer<vtkTubeFilter> TubeFilter;

  vtkSmartPointer<vtkPolyDataMapper> LineMapper;
  vtkSmartPointer<vtkPolyDataMapper> LineOccludedMapper;

  vtkSmartPointer<vtkActor> LineActor;
  vtkSmartPointer<vtkActor> LineOccludedActor;

private:
  vtkSlicerLineRepresentation3D(const vtkSlicerLineRepresentation3D&) = delete;
  void operator=(const vtkSlicerLineRepresentation3D&) = delete;
};

#endif
