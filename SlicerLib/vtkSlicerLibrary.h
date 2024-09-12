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

==============================================================================*/

#ifndef __vtkSlicerLibrary_h
#define __vtkSlicerLibrary_h

#include "vtkSlicerLibraryModule.h"

#include <vtkObject.h> // for base class

class VTK_SLICER_LIBRARY_EXPORT vtkSlicerLibrary : public vtkObject
{
public:
  static vtkSlicerLibrary* New();
  vtkTypeMacro(vtkSlicerLibrary, vtkObject);

  vtkSlicerLibrary();
  ~vtkSlicerLibrary();

  static int GetVersionMajor();
  static int GetVersionMinor();
  static int GetVersionPatch();
  static const char* GetVersionString();
};

#endif
