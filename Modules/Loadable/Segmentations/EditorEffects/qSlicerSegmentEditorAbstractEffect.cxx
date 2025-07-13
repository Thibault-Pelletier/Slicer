/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "qSlicerSegmentEditorAbstractEffect.h"
#include "qSlicerSegmentEditorAbstractEffect_p.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSegmentEditorNode.h"

#include "vtkSlicerSegmentationsModuleLogic.h"

// SegmentationCore includes
#include <vtkOrientedImageData.h>
#include <vtkOrientedImageDataResample.h>

// Qt includes
#include <QColor>
#include <QDebug>
#include <QFormLayout>
#include <QFrame>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPaintDevice>
#include <QPixmap>
#include <QSettings>

// Slicer includes
#include "qMRMLSliceWidget.h"
#include "qMRMLSliceView.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLThreeDView.h"
#include "qSlicerApplication.h"
#include "qSlicerCoreApplication.h"
#include "vtkMRMLSliceLogic.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLSliceNode.h"
#include "vtkMRMLViewNode.h"
#include "vtkMRMLInteractionNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkMRMLTransformNode.h"

// VTK includes
#include <vtkImageConstantPad.h>
#include <vtkImageShiftScale.h>
#include <vtkImageThreshold.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkProp.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

// CTK includes
#include <ctkMessageBox.h>

#include "vtkSegmentEditorAbstractEffect.h"

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorAbstractEffectPrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffectPrivate::qSlicerSegmentEditorAbstractEffectPrivate(qSlicerSegmentEditorAbstractEffect& object)
  : q_ptr(&object)
  , Scene(nullptr)
  , SavedCursor(QCursor(Qt::ArrowCursor))
  , OptionsFrame(nullptr)
{
  this->OptionsFrame = new QFrame();
  this->OptionsFrame->setFrameShape(QFrame::NoFrame);
  this->OptionsFrame->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));
  QFormLayout* layout = new QFormLayout(this->OptionsFrame);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffectPrivate::~qSlicerSegmentEditorAbstractEffectPrivate()
{
  if (this->OptionsFrame)
  {
    delete this->OptionsFrame;
    this->OptionsFrame = nullptr;
  }
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorAbstractEffect methods

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect::qSlicerSegmentEditorAbstractEffect(const vtkSmartPointer<vtkSegmentEditorAbstractEffect>& effectLogic, QObject* parent)
  : Superclass(parent)
  , m_EffectLogic(effectLogic)
  , d_ptr(new qSlicerSegmentEditorAbstractEffectPrivate(*this))
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect::~qSlicerSegmentEditorAbstractEffect() = default;

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorAbstractEffect::name() const
{
  return QString::fromStdString(m_EffectLogic->name());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setName(QString name)
{
  Q_UNUSED(name);
  qCritical() << Q_FUNC_INFO << ": Cannot set effect name by method, only in constructor!";
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorAbstractEffect::title() const
{
  return QString::fromStdString(m_EffectLogic->title());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setTitle(QString title)
{
  m_EffectLogic->setTitle(title.toStdString());
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorAbstractEffect::perSegment() const
{
  return m_EffectLogic->perSegment();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setPerSegment(bool perSegment)
{
  Q_UNUSED(perSegment);
  qCritical() << Q_FUNC_INFO << ": Cannot set per-segment flag by method, only in constructor!";
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorAbstractEffect::requireSegments() const
{
  return m_EffectLogic->requireSegments();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setRequireSegments(bool requireSegments)
{
  m_EffectLogic->setRequireSegments(requireSegments);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::activate()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  // Show options frame
  d->OptionsFrame->setVisible(true);

  m_EffectLogic->activate();

  this->updateGUIFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::deactivate()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  // Hide options frame
  d->OptionsFrame->setVisible(false);

  m_EffectLogic->deactivate();
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorAbstractEffect::active()
{
  return m_EffectLogic->active();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::applyImageMask(vtkOrientedImageData* input, vtkOrientedImageData* mask, double fillValue, bool notMask /*=false*/)
{
  vtkSegmentEditorAbstractEffect::applyImageMask(input, mask, fillValue, notMask);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::modifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap, ModificationMode modificationMode, bool bypassMasking /*=false*/)
{
  m_EffectLogic->modifySelectedSegmentByLabelmap(modifierLabelmap, modificationMode, bypassMasking);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::modifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap,
                                                                         ModificationMode modificationMode,
                                                                         QList<int> extent,
                                                                         bool bypassMasking /*=false*/)
{
  m_EffectLogic->modifySelectedSegmentByLabelmap(modifierLabelmap, modificationMode, std::vector<int>(extent.begin(), extent.end()), bypassMasking);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::modifySelectedSegmentByLabelmap(vtkOrientedImageData* modifierLabelmap,
                                                                         ModificationMode modificationMode,
                                                                         const int modificationExtent[6],
                                                                         bool bypassMasking /*=false*/)
{
  m_EffectLogic->modifySelectedSegmentByLabelmap(modifierLabelmap, modificationMode, modificationExtent, bypassMasking);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::modifySegmentByLabelmap(vtkMRMLSegmentationNode* segmentationNode,
                                                                 const char* segmentID,
                                                                 vtkOrientedImageData* modifierLabelmap,
                                                                 ModificationMode modificationMode,
                                                                 bool bypassMasking /*=false*/)
{
  m_EffectLogic->modifySelectedSegmentByLabelmap(segmentationNode, segmentID, modifierLabelmap, modificationMode, bypassMasking);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::modifySegmentByLabelmap(vtkMRMLSegmentationNode* segmentationNode,
                                                                 const char* segmentID,
                                                                 vtkOrientedImageData* modifierLabelmapInput,
                                                                 ModificationMode modificationMode,
                                                                 const int modificationExtent[6],
                                                                 bool bypassMasking /*=false*/)
{
  m_EffectLogic->modifySelectedSegmentByLabelmap(segmentationNode, segmentID, modifierLabelmapInput, modificationMode, modificationExtent, bypassMasking);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::selectEffect(QString effectName)
{
  m_EffectLogic->selectEffect(effectName);
}

//-----------------------------------------------------------------------------
QCursor qSlicerSegmentEditorAbstractEffect::createCursor(qMRMLWidget* viewWidget)
{
  Q_UNUSED(viewWidget); // The default cursor is not view-specific, but this method can be overridden

  QImage baseImage(":Icons/NullEffect.png");
  QIcon effectIcon(this->icon());
  if (effectIcon.isNull())
  {
    QPixmap cursorPixmap = QPixmap::fromImage(baseImage);
    return QCursor(cursorPixmap, baseImage.width() / 2, 0);
  }

  QImage effectImage(effectIcon.pixmap(effectIcon.availableSizes()[0]).toImage());
  int width = qMax(baseImage.width(), effectImage.width());
  int height = baseImage.height() + effectImage.height();
  width = height = qMax(width, height);
  int center = width / 2;
  QImage cursorImage(width, height, QImage::Format_ARGB32);
  QPainter painter;
  cursorImage.fill(0);
  painter.begin(&cursorImage);
  QPoint point(center - (baseImage.width() / 2), 0);
  painter.drawImage(point, baseImage);
  int draw_x_start = center - (effectImage.width() / 2);
  int draw_y_start = cursorImage.height() - effectImage.height();
  point.setX(draw_x_start);
  point.setY(draw_y_start);
  painter.drawImage(point, effectImage);
  QRectF rectangle(draw_x_start, draw_y_start, effectImage.width(), effectImage.height() - 1);
  painter.setPen(QColor("white"));
  painter.drawRect(rectangle);
  painter.end();

  QPixmap cursorPixmap = QPixmap::fromImage(cursorImage);
  // NullEffect.png arrow point at 5 pixels right and 2 pixels down from upper left (0,0) location
  int hotX = center - (baseImage.width() / 2) + 5;
  int hotY = 2;
  return QCursor(cursorPixmap, hotX, hotY);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::cursorOff(qMRMLWidget* viewWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  d->SavedCursor = QCursor(viewWidget->cursor());
  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget && sliceWidget->sliceView())
  {
    sliceWidget->sliceView()->setDefaultViewCursor(QCursor(Qt::BlankCursor));
  }
  else if (threeDWidget && threeDWidget->threeDView())
  {
    threeDWidget->threeDView()->setDefaultViewCursor(QCursor(Qt::BlankCursor));
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::cursorOn(qMRMLWidget* viewWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget && sliceWidget->sliceView())
  {
    sliceWidget->sliceView()->setDefaultViewCursor(d->SavedCursor);
  }
  else if (threeDWidget && threeDWidget->threeDView())
  {
    threeDWidget->threeDView()->setDefaultViewCursor(d->SavedCursor);
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::addActor3D(qMRMLWidget* viewWidget, vtkProp3D* actor)
{
  m_EffectLogic->addViewProp(viewWidget->mrmlViewNode(), actor);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::addActor2D(qMRMLWidget* viewWidget, vtkActor2D* actor)
{
  m_EffectLogic->addViewProp(viewWidget->mrmlViewNode(), actor);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::removeActor3D(qMRMLWidget* viewWidget, vtkProp3D* actor)
{
  m_EffectLogic->removeViewProp(viewWidget->mrmlViewNode(), actor);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::removeActor2D(qMRMLWidget* viewWidget, vtkActor2D* actor)
{
  m_EffectLogic->removeViewProp(viewWidget->mrmlViewNode(), actor);
}

//-----------------------------------------------------------------------------
QFrame* qSlicerSegmentEditorAbstractEffect::optionsFrame()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  return d->OptionsFrame;
}

//-----------------------------------------------------------------------------
QFormLayout* qSlicerSegmentEditorAbstractEffect::optionsLayout()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  QFormLayout* formLayout = qobject_cast<QFormLayout*>(d->OptionsFrame->layout());
  return formLayout;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::addOptionsWidget(QWidget* newOptionsWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  newOptionsWidget->setParent(d->OptionsFrame);
  newOptionsWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));
  this->optionsLayout()->addRow(newOptionsWidget);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::addOptionsWidget(QLayout* newOptionsWidget)
{
  this->optionsLayout()->addRow(newOptionsWidget);
}

//-----------------------------------------------------------------------------
QWidget* qSlicerSegmentEditorAbstractEffect::addLabeledOptionsWidget(QString label, QWidget* newOptionsWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  QLabel* labelWidget = new QLabel(label);
  newOptionsWidget->setParent(d->OptionsFrame);
  newOptionsWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));
  this->optionsLayout()->addRow(labelWidget, newOptionsWidget);
  return labelWidget;
}

//-----------------------------------------------------------------------------
QWidget* qSlicerSegmentEditorAbstractEffect::addLabeledOptionsWidget(QString label, QLayout* newOptionsWidget)
{
  QLabel* labelWidget = new QLabel(label);
  if (dynamic_cast<QHBoxLayout*>(newOptionsWidget) == nullptr)
  {
    // for multiline layouts, align label to the top
    labelWidget->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  }
  this->optionsLayout()->addRow(labelWidget, newOptionsWidget);
  return labelWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLScene* qSlicerSegmentEditorAbstractEffect::scene()
{
  m_EffectLogic->scene();
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentEditorNode* qSlicerSegmentEditorAbstractEffect::parameterSetNode()
{
  m_EffectLogic->parameterSetNode();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameterSetNode(vtkMRMLSegmentEditorNode* node)
{
  m_EffectLogic->setParameterSetNode(node);
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorAbstractEffect::parameter(QString name)
{
  return QString::fromStdString(m_EffectLogic->parameter(name.toStdString()));
}

//-----------------------------------------------------------------------------
int qSlicerSegmentEditorAbstractEffect::integerParameter(QString name)
{
  return m_EffectLogic->integerParameter(name.toStdString());
}

//-----------------------------------------------------------------------------
double qSlicerSegmentEditorAbstractEffect::doubleParameter(QString name)
{
  return m_EffectLogic->doubleParameter(name.toStdString());
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qSlicerSegmentEditorAbstractEffect::nodeReference(QString name)
{
  return m_EffectLogic->nodeReference(name.toStdString());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameter(QString name, QString value)
{
  m_EffectLogic->setParameter(name.toStdString(), value.toStdString());
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorAbstractEffect::parameterDefined(QString name)
{
  return m_EffectLogic->parameterDefined(name.toStdString());
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorAbstractEffect::commonParameterDefined(QString name)
{
  return m_EffectLogic->commonParameterDefined(name.toStdString());
}

//-----------------------------------------------------------------------------
int qSlicerSegmentEditorAbstractEffect::confirmCurrentSegmentVisible()
{
  if (m_EffectLogic->isSelectedSegmentVisible())
  {
    // no parameter set node - do not prevent operation
    return ConfirmedWithoutDialog;
  }

  qSlicerApplication* app = qSlicerApplication::application();
  QWidget* mainWindow = app ? app->mainWindow() : nullptr;

  ctkMessageBox* confirmCurrentSegmentVisibleMsgBox = new ctkMessageBox(mainWindow);
  confirmCurrentSegmentVisibleMsgBox->setAttribute(Qt::WA_DeleteOnClose);
  confirmCurrentSegmentVisibleMsgBox->setWindowTitle(tr("Operate on invisible segment?"));
  confirmCurrentSegmentVisibleMsgBox->setText(tr("The currently selected segment is hidden. Would you like to make it visible?"));

  confirmCurrentSegmentVisibleMsgBox->addButton(QMessageBox::Yes);
  confirmCurrentSegmentVisibleMsgBox->addButton(QMessageBox::No);
  confirmCurrentSegmentVisibleMsgBox->addButton(QMessageBox::Cancel);

  confirmCurrentSegmentVisibleMsgBox->setDontShowAgainVisible(true);
  confirmCurrentSegmentVisibleMsgBox->setDontShowAgainSettingsKey("Segmentations/ConfirmEditHiddenSegment");
  confirmCurrentSegmentVisibleMsgBox->addDontShowAgainButtonRole(QMessageBox::YesRole);
  confirmCurrentSegmentVisibleMsgBox->addDontShowAgainButtonRole(QMessageBox::NoRole);

  confirmCurrentSegmentVisibleMsgBox->setIcon(QMessageBox::Question);

  QSettings settings;
  int savedButtonSelection = settings.value(confirmCurrentSegmentVisibleMsgBox->dontShowAgainSettingsKey(), static_cast<int>(QMessageBox::InvalidRole)).toInt();

  int resultCode = confirmCurrentSegmentVisibleMsgBox->exec();

  // Cancel means that user did not let the operation to proceed
  if (resultCode == QMessageBox::Cancel)
  {
    return NotConfirmed;
  }

  m_EffectLogic->confirmWorkOnSelectedSegment(resultCode == QMessageBox::Yes);
  return ConfirmedWithDialog;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameterDefault(QString name, QString value)
{
  m_EffectLogic->setParameterDefault(name.toStdString(), value.toStdString());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCommonParameter(QString name, QString value)
{
  m_EffectLogic->setCommonParameter(name.toStdString(), value.toStdString());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCommonParameterDefault(QString name, QString value)
{
  m_EffectLogic->setCommonParameterDefault(name.toStdString(), value.toStdString());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameter(QString name, int value)
{
  m_EffectLogic->setParameter(name.toStdString(), value);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameterDefault(QString name, int value)
{
  m_EffectLogic->setParameterDefault(name.toStdString(), value);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCommonParameter(QString name, int value)
{
  m_EffectLogic->setCommonParameter(name.toStdString(), value);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCommonParameterDefault(QString name, int value)
{
  m_EffectLogic->setCommonParameterDefault(name.toStdString(), value);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameter(QString name, double value)
{
  m_EffectLogic->setParameter(name.toStdString(), value);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameterDefault(QString name, double value)
{
  m_EffectLogic->setParameterDefault(name.toStdString(), value);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCommonParameter(QString name, double value)
{
  m_EffectLogic->setCommonParameter(name.toStdString(), value);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCommonParameterDefault(QString name, double value)
{
  m_EffectLogic->setCommonParameterDefault(name.toStdString(), value);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setNodeReference(QString name, vtkMRMLNode* node)
{
  m_EffectLogic->setNodeReference(name.toStdString(), node);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCommonNodeReference(QString name, vtkMRMLNode* node)
{
  m_EffectLogic->setCommonNodeReference(name.toStdString(), node);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setVolumes(vtkOrientedImageData* alignedSourceVolume,
                                                    vtkOrientedImageData* modifierLabelmap,
                                                    vtkOrientedImageData* maskLabelmap,
                                                    vtkOrientedImageData* selectedSegmentLabelmap,
                                                    vtkOrientedImageData* referenceGeometryImage)
{
  m_EffectLogic->setVolumes(alignedSourceVolume, modifierLabelmap, maskLabelmap, selectedSegmentLabelmap, referenceGeometryImage);
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::defaultModifierLabelmap()
{
  return m_EffectLogic->defaultModifierLabelmap();
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::modifierLabelmap()
{
  return m_EffectLogic->modifierLabelmap();
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::maskLabelmap()
{
  return m_EffectLogic->maskLabelmap();
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::sourceVolumeImageData()
{
  return m_EffectLogic->sourceVolumeImageData();
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::masterVolumeImageData()
{
  qWarning("qSlicerSegmentEditorAbstractEffect::masterVolumeImageData() method is deprecated,"
           " use sourceVolumeImageData method instead");
  return this->sourceVolumeImageData();
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::selectedSegmentLabelmap()
{
  return m_EffectLogic->selectedSegmentLabelmap();
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::referenceGeometryImage()
{
  return m_EffectLogic->referenceGeometryImage();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::saveStateForUndo()
{
  m_EffectLogic->saveStateForUndo();
}

//-----------------------------------------------------------------------------
vtkRenderWindow* qSlicerSegmentEditorAbstractEffect::renderWindow(qMRMLWidget* viewWidget)
{
  if (!viewWidget)
  {
    return nullptr;
  }

  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget)
  {
    if (!sliceWidget->sliceView())
    {
      // probably the application is closing
      return nullptr;
    }
    return sliceWidget->sliceView()->renderWindow();
  }
  else if (threeDWidget)
  {
    if (!threeDWidget->threeDView())
    {
      // probably the application is closing
      return nullptr;
    }
    return threeDWidget->threeDView()->renderWindow();
  }

  qCritical() << Q_FUNC_INFO << ": Unsupported view widget type!";
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkRenderer* qSlicerSegmentEditorAbstractEffect::renderer(qMRMLWidget* viewWidget)
{
  return vtkSegmentEditorAbstractEffect::renderer(qSlicerSegmentEditorAbstractEffect::renderWindow(viewWidget));
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractViewNode* qSlicerSegmentEditorAbstractEffect::viewNode(qMRMLWidget* viewWidget)
{
  if (!viewWidget)
  {
    return nullptr;
  }

  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget)
  {
    return sliceWidget->sliceLogic()->GetSliceNode();
  }
  else if (threeDWidget)
  {
    return threeDWidget->mrmlViewNode();
  }

  qCritical() << Q_FUNC_INFO << ": Unsupported view widget type!";
  return nullptr;
}

//-----------------------------------------------------------------------------
QPoint qSlicerSegmentEditorAbstractEffect::rasToXy(double ras[3], qMRMLSliceWidget* sliceWidget)
{
  auto xy = vtkSegmentEditorAbstractEffect::rasToXy(ras, viewNode(sliceWidget));
  return { xy[0], xy[1] };
}

//-----------------------------------------------------------------------------
QPoint qSlicerSegmentEditorAbstractEffect::rasToXy(QVector3D rasVector, qMRMLSliceWidget* sliceWidget)
{
  double ras[3] = { rasVector.x(), rasVector.y(), rasVector.z() };
  return qSlicerSegmentEditorAbstractEffect::rasToXy(ras, sliceWidget);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyzToRas(double inputXyz[3], double outputRas[3], qMRMLSliceWidget* sliceWidget)
{
  vtkSegmentEditorAbstractEffect::xyzToRas(inputXyz, outputRas, viewNode(sliceWidget));
}

//-----------------------------------------------------------------------------
QVector3D qSlicerSegmentEditorAbstractEffect::xyzToRas(QVector3D inputXyzVector, qMRMLSliceWidget* sliceWidget)
{
  double inputXyz[3] = { inputXyzVector.x(), inputXyzVector.y(), inputXyzVector.z() };
  double outputRas[3] = { 0.0, 0.0, 0.0 };
  qSlicerSegmentEditorAbstractEffect::xyzToRas(inputXyz, outputRas, sliceWidget);
  QVector3D outputVector(outputRas[0], outputRas[1], outputRas[2]);
  return outputVector;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToRas(QPoint xy, double outputRas[3], qMRMLSliceWidget* sliceWidget)
{
  double xyz[3] = { static_cast<double>(xy.x()), static_cast<double>(xy.y()), 0.0 };

  qSlicerSegmentEditorAbstractEffect::xyzToRas(xyz, outputRas, sliceWidget);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToRas(double xy[2], double outputRas[3], qMRMLSliceWidget* sliceWidget)
{
  double xyz[3] = { xy[0], xy[1], 0.0 };
  qSlicerSegmentEditorAbstractEffect::xyzToRas(xyz, outputRas, sliceWidget);
}

//-----------------------------------------------------------------------------
QVector3D qSlicerSegmentEditorAbstractEffect::xyToRas(QPoint xy, qMRMLSliceWidget* sliceWidget)
{
  double outputRas[3] = { 0.0, 0.0, 0.0 };
  qSlicerSegmentEditorAbstractEffect::xyToRas(xy, outputRas, sliceWidget);
  QVector3D outputVector(outputRas[0], outputRas[1], outputRas[2]);
  return outputVector;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyzToIjk(double inputXyz[3],
                                                  int outputIjk[3],
                                                  qMRMLSliceWidget* sliceWidget,
                                                  vtkOrientedImageData* image,
                                                  vtkMRMLTransformNode* parentTransformNode /*=nullptr*/)
{
  vtkSegmentEditorAbstractEffect::xyzToIjk(inputXyz, outputIjk, viewNode(sliceWidget), image, parentTransformNode);
}

//-----------------------------------------------------------------------------
QVector3D qSlicerSegmentEditorAbstractEffect::xyzToIjk(QVector3D inputXyzVector,
                                                       qMRMLSliceWidget* sliceWidget,
                                                       vtkOrientedImageData* image,
                                                       vtkMRMLTransformNode* parentTransformNode /*=nullptr*/)
{
  double inputXyz[3] = { inputXyzVector.x(), inputXyzVector.y(), inputXyzVector.z() };
  int outputIjk[3] = { 0, 0, 0 };
  qSlicerSegmentEditorAbstractEffect::xyzToIjk(inputXyz, outputIjk, sliceWidget, image, parentTransformNode);

  QVector3D outputVector(outputIjk[0], outputIjk[1], outputIjk[2]);
  return outputVector;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint xy,
                                                 int outputIjk[3],
                                                 qMRMLSliceWidget* sliceWidget,
                                                 vtkOrientedImageData* image,
                                                 vtkMRMLTransformNode* parentTransformNode /*=nullptr*/)
{
  double xyz[3] = { static_cast<double>(xy.x()), static_cast<double>(xy.y()), 0.0 };
  qSlicerSegmentEditorAbstractEffect::xyzToIjk(xyz, outputIjk, sliceWidget, image, parentTransformNode);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToIjk(double xy[2],
                                                 int outputIjk[3],
                                                 qMRMLSliceWidget* sliceWidget,
                                                 vtkOrientedImageData* image,
                                                 vtkMRMLTransformNode* parentTransformNode /*=nullptr*/)
{
  double xyz[3] = { xy[0], xy[0], 0.0 };
  qSlicerSegmentEditorAbstractEffect::xyzToIjk(xyz, outputIjk, sliceWidget, image, parentTransformNode);
}

//-----------------------------------------------------------------------------
QVector3D qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint xy, qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image, vtkMRMLTransformNode* parentTransformNode /*=nullptr*/)
{
  int outputIjk[3] = { 0, 0, 0 };
  qSlicerSegmentEditorAbstractEffect::xyToIjk(xy, outputIjk, sliceWidget, image, parentTransformNode);
  QVector3D outputVector(outputIjk[0], outputIjk[1], outputIjk[2]);
  return outputVector;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::forceRender(qMRMLWidget* viewWidget)
{
  viewNode(viewWidget)->ForceRender();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::scheduleRender(qMRMLWidget* viewWidget)
{
  viewNode(viewWidget)->ScheduleRender();
}

//----------------------------------------------------------------------------
double qSlicerSegmentEditorAbstractEffect::sliceSpacing(qMRMLSliceWidget* sliceWidget)
{
  return vtkSegmentEditorAbstractEffect::sliceSpacing(viewNode(sliceWidget));
}

//----------------------------------------------------------------------------
bool qSlicerSegmentEditorAbstractEffect::showEffectCursorInSliceView()
{
  return m_EffectLogic->showEffectCursorInSliceView();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setShowEffectCursorInSliceView(bool show)
{
  m_EffectLogic->setShowEffectCursorInSliceView(show);
}

//----------------------------------------------------------------------------
bool qSlicerSegmentEditorAbstractEffect::showEffectCursorInThreeDView()
{
  return m_EffectLogic->showEffectCursorInThreeDView();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setShowEffectCursorInThreeDView(bool show)
{
  m_EffectLogic->setShowEffectCursorInThreeDView(show);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::interactionNodeModified(vtkMRMLInteractionNode* interactionNode)
{
  m_EffectLogic->interactionNodeModified(interactionNode);
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorAbstractEffect::segmentationDisplayableInView(vtkMRMLAbstractViewNode* viewNode)
{
  return m_EffectLogic->segmentationDisplayableInView(viewNode);
}
