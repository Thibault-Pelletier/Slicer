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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerVolumesReader_h
#define __qSlicerVolumesReader_h

// Slicer includes
#include "qSlicerFileReader.h"
class qSlicerVolumesReaderPrivate;
class vtkSlicerVolumesLogic;

//-----------------------------------------------------------------------------
class qSlicerVolumesReader : public qSlicerFileReader
{
  Q_OBJECT
public:
  typedef qSlicerFileReader Superclass;
  qSlicerVolumesReader(QObject* parent = nullptr);
  qSlicerVolumesReader(vtkSlicerVolumesLogic* logic, QObject* parent = nullptr);
  ~qSlicerVolumesReader() override;

  vtkSlicerVolumesLogic* logic() const;
  void setLogic(vtkSlicerVolumesLogic* logic);

  QString description() const override;
  IOFileType fileType() const override;
  QStringList extensions() const override;
  qSlicerIOOptions* options() const override;

  /// Returns a positive number (>0) if the reader can load this file.
  /// In case the file uses a generic file extension (such as .nrrd) then the confidence value is adjusted based on
  /// the file content: if the file contains a dwmri nrrd file then confidence is increased to 0.7
  double canLoadFileConfidence(const QString& file) const override;

  bool load(const IOProperties& properties) override;

  /// Implements the file list examination for the corresponding method in the core
  /// IO manager.
  /// \sa qSlicerCoreIOManager
  bool examineFileInfoList(QFileInfoList& fileInfoList, QFileInfo& archetypeFileInfo, qSlicerIO::IOProperties& ioProperties) const override;

protected:
  QScopedPointer<qSlicerVolumesReaderPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerVolumesReader);
  Q_DISABLE_COPY(qSlicerVolumesReader);
};

#endif
