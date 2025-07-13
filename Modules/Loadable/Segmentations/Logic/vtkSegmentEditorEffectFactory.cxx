/*==============================================================================

  Program: 3D Slicer

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

// Segmentations EditorEffects includes
#include "vtkSegmentEditorEffectFactory.h"
#include "vtkSegmentEditorAbstractEffect.h"

// Qt includes
#include <QDebug>

//----------------------------------------------------------------------------
vtkSegmentEditorEffectFactory* vtkSegmentEditorEffectFactory::m_Instance = nullptr;

//----------------------------------------------------------------------------
class vtkSegmentEditorEffectFactoryCleanup
{
public:
  inline void use() {}

  ~vtkSegmentEditorEffectFactoryCleanup()
  {
    if (vtkSegmentEditorEffectFactory::m_Instance)
    {
      vtkSegmentEditorEffectFactory::cleanup();
    }
  }
};
static vtkSegmentEditorEffectFactoryCleanup vtkSegmentEditorEffectFactoryCleanupGlobal;

//-----------------------------------------------------------------------------
vtkSegmentEditorEffectFactory* vtkSegmentEditorEffectFactory::instance()
{
  if (!vtkSegmentEditorEffectFactory::m_Instance)
  {
    vtkSegmentEditorEffectFactoryCleanupGlobal.use();
    vtkSegmentEditorEffectFactory::m_Instance = new vtkSegmentEditorEffectFactory();
  }
  // Return the instance
  return vtkSegmentEditorEffectFactory::m_Instance;
}

//-----------------------------------------------------------------------------
void vtkSegmentEditorEffectFactory::cleanup()
{
  if (vtkSegmentEditorEffectFactory::m_Instance)
  {
    delete vtkSegmentEditorEffectFactory::m_Instance;
    vtkSegmentEditorEffectFactory::m_Instance = nullptr;
  }
}

//-----------------------------------------------------------------------------
vtkSegmentEditorEffectFactory::vtkSegmentEditorEffectFactory(QObject* parent)
  : QObject(parent)
{
  this->m_RegisteredEffects.clear();
}

//-----------------------------------------------------------------------------
vtkSegmentEditorEffectFactory::~vtkSegmentEditorEffectFactory()
{
  foreach (vtkSegmentEditorAbstractEffect* effect, m_RegisteredEffects)
  {
    delete effect;
  }
  this->m_RegisteredEffects.clear();
}

//---------------------------------------------------------------------------
bool vtkSegmentEditorEffectFactory::registerEffect(vtkSegmentEditorAbstractEffect* effectToRegister)
{
  if (effectToRegister == nullptr)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid effect to register!";
    return false;
  }
  if (effectToRegister->name().isEmpty())
  {
    qCritical() << Q_FUNC_INFO << ": Segment editor effect cannot be registered with empty name!";
    return false;
  }

  // Check if the same effect has already been registered
  vtkSegmentEditorAbstractEffect* currentEffect = nullptr;
  foreach (currentEffect, this->m_RegisteredEffects)
  {
    if (effectToRegister->name().compare(currentEffect->name()) == 0)
    {
      return false;
    }
  }

  // Add the effect to the list
  this->m_RegisteredEffects << effectToRegister;
  emit effectRegistered(effectToRegister->name());
  return true;
}

//---------------------------------------------------------------------------
QList<vtkSegmentEditorAbstractEffect*> vtkSegmentEditorEffectFactory::copyEffects(QList<vtkSegmentEditorAbstractEffect*>& effects)
{
  QList<vtkSegmentEditorAbstractEffect*> copiedEffects;
  foreach (vtkSegmentEditorAbstractEffect* effect, m_RegisteredEffects)
  {
    // If effect is added already then skip it
    bool effectAlreadyAdded = false;
    foreach (vtkSegmentEditorAbstractEffect* existingEffect, effects)
    {
      if (existingEffect->name() == effect->name())
      {
        // already in the list
        effectAlreadyAdded = true;
        break;
      }
    }
    if (effectAlreadyAdded)
    {
      continue;
    }

    // Effect not in the list yet, clone it and add it
    vtkSegmentEditorAbstractEffect* clonedEffect = effect->clone();
    if (!clonedEffect)
    {
      // make sure we don't put a nullptr pointer in the effect list
      qCritical() << Q_FUNC_INFO << " failed to clone effect: " << effect->name();
      continue;
    }
    effects << clonedEffect;
    copiedEffects << clonedEffect;
  }
  return copiedEffects;
}
