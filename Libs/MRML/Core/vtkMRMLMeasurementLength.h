/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __vtkMRMLMeasurementLength_h
#define __vtkMRMLMeasurementLength_h

// MRML includes
#include "vtkMRMLMeasurement.h"

// Markups includes
#include "vtkMRMLExport.h"

/// \brief Measurement class calculating curve length
class VTK_MRML_EXPORT vtkMRMLMeasurementLength : public vtkMRMLMeasurement
{
public:
  static vtkMRMLMeasurementLength* New();
  vtkTypeMacro(vtkMRMLMeasurementLength, vtkMRMLMeasurement);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Create a new instance of this measurement type.
  /// Only in C++: The caller must take ownership of the returned object.
  VTK_NEWINSTANCE
  vtkMRMLMeasurement* CreateInstance() const override { return vtkMRMLMeasurementLength::New(); }

  /// Calculate length of \sa InputMRMLNode curve markup node and store the result internally
  void Compute() override;

protected:
  vtkMRMLMeasurementLength();
  ~vtkMRMLMeasurementLength() override;
  vtkMRMLMeasurementLength(const vtkMRMLMeasurementLength&);
  void operator=(const vtkMRMLMeasurementLength&);
};

#endif
