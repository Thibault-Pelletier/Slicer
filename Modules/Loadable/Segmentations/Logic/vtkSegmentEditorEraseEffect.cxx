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
#include "vtkSegmentEditorEraseEffect.h"

//-----------------------------------------------------------------------------
class vtkSegmentEditorEraseEffectPrivate : public QObject
{
  Q_DECLARE_PUBLIC(vtkSegmentEditorEraseEffect);

protected:
  vtkSegmentEditorEraseEffect* const q_ptr;

public:
  vtkSegmentEditorEraseEffectPrivate(vtkSegmentEditorEraseEffect& object);
  ~vtkSegmentEditorEraseEffectPrivate() override;

public:
  QIcon EraseIcon;
};

//-----------------------------------------------------------------------------
vtkSegmentEditorEraseEffectPrivate::vtkSegmentEditorEraseEffectPrivate(vtkSegmentEditorEraseEffect& object)
  : q_ptr(&object)
{
  this->EraseIcon = QIcon(":Icons/Erase.png");
}

//-----------------------------------------------------------------------------
vtkSegmentEditorEraseEffectPrivate::~vtkSegmentEditorEraseEffectPrivate() = default;

//-----------------------------------------------------------------------------
// vtkSegmentEditorEraseEffect methods

//----------------------------------------------------------------------------
vtkSegmentEditorEraseEffect::vtkSegmentEditorEraseEffect(QObject* parent)
  : Superclass(parent)
  , d_ptr(new vtkSegmentEditorEraseEffectPrivate(*this))
{
  this->m_Name = QString(/*no tr*/ "Erase");
  this->m_Title = tr("Erase");
  this->m_AlwaysErase = true;
  this->m_ShowEffectCursorInThreeDView = true;
}

//----------------------------------------------------------------------------
vtkSegmentEditorEraseEffect::~vtkSegmentEditorEraseEffect() = default;

//---------------------------------------------------------------------------
QIcon vtkSegmentEditorEraseEffect::icon()
{
  Q_D(vtkSegmentEditorEraseEffect);

  return d->EraseIcon;
}

//---------------------------------------------------------------------------
const QString vtkSegmentEditorEraseEffect::helpText() const
{
  return QString("<html>")
         + tr("Erase from current segment with a round brush<br>."
              "<p><ul style=\"margin: 0\">"
              "<li><b>Left-button drag-and-drop:</b> erase from segment around the mouse pointer."
              "<li><b>Shift + mouse wheel</b> or <b>+/- keys:</b> adjust brush size."
              "<li><b>Ctrl + mouse wheel:</b> slice view zoom in/out."
              "</ul><p>"
              "Editing is available both in slice and 3D views."
              "<p>");
}

//-----------------------------------------------------------------------------
vtkSegmentEditorAbstractEffect* vtkSegmentEditorEraseEffect::clone()
{
  return new vtkSegmentEditorEraseEffect();
}
