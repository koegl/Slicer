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

// Qt includes
#include <QDebug>
#include <QSettings>
#include <QMessageBox>

// QtGUI includes
#include "qSlicerAbstractCoreModule.h"
#include "qSlicerApplication.h"
#include "qSlicerModuleManager.h"
#include "qSlicerSegmentationsSettingsPanel.h"
#include "qSlicerTerminologySelectorDialog.h"
#include "ui_qSlicerSegmentationsSettingsPanel.h"

// Logic includes
#include <vtkSlicerSegmentationsModuleLogic.h>
#include <vtkSlicerTerminologiesModuleLogic.h>

#include <vtkMRMLSegmentEditorNode.h>

// --------------------------------------------------------------------------
// qSlicerSegmentationsSettingsPanelPrivate

//-----------------------------------------------------------------------------
class qSlicerSegmentationsSettingsPanelPrivate : public Ui_qSlicerSegmentationsSettingsPanel
{
  Q_DECLARE_PUBLIC(qSlicerSegmentationsSettingsPanel);

protected:
  qSlicerSegmentationsSettingsPanel* const q_ptr;

public:
  qSlicerSegmentationsSettingsPanelPrivate(qSlicerSegmentationsSettingsPanel& object);
  void init();

  QString DefaultTerminologyString;

  vtkWeakPointer<vtkSlicerSegmentationsModuleLogic> SegmentationsLogic;
  vtkWeakPointer<vtkSlicerTerminologiesModuleLogic> TerminologiesLogic;
};

// --------------------------------------------------------------------------
// qSlicerSegmentationsSettingsPanelPrivate methods

// --------------------------------------------------------------------------
qSlicerSegmentationsSettingsPanelPrivate::qSlicerSegmentationsSettingsPanelPrivate(qSlicerSegmentationsSettingsPanel& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerSegmentationsSettingsPanelPrivate::init()
{
  Q_Q(qSlicerSegmentationsSettingsPanel);

  this->setupUi(q);

  this->TerminologiesLogic = vtkSlicerTerminologiesModuleLogic::SafeDownCast(qSlicerCoreApplication::application()->moduleLogic("Terminologies"));
  if (!this->TerminologiesLogic)
  {
    qCritical() << Q_FUNC_INFO << ": Terminologies logic is not found";
  }

  // Default values
  this->AutoOpacitiesCheckBox->setChecked(true);
  this->SurfaceSmoothingCheckBox->setChecked(true);
  this->UseTerminologyCheckBox->setChecked(true);

  // Register settings
  q->registerProperty(/*no tr*/ "Segmentations/AutoOpacities",
                      this->AutoOpacitiesCheckBox,
                      "checked",
                      SIGNAL(toggled(bool)),
                      qSlicerSegmentationsSettingsPanel::tr("Automatically set opacities of the segments based on which contains which, "
                                                            "so that no segment obscures another"),
                      ctkSettingsPanel::OptionNone);
  q->registerProperty(/*no tr*/ "Segmentations/DefaultSurfaceSmoothing",
                      this->SurfaceSmoothingCheckBox,
                      "checked",
                      SIGNAL(toggled(bool)),
                      qSlicerSegmentationsSettingsPanel::tr("Enable closed surface representation smoothing by default"),
                      ctkSettingsPanel::OptionNone);
  q->registerProperty(/*no tr*/ "Segmentations/UseTerminologySelector",
                      this->UseTerminologyCheckBox,
                      "checked",
                      SIGNAL(toggled(bool)),
                      qSlicerSegmentationsSettingsPanel::tr("Use standard terminology for segments"),
                      ctkSettingsPanel::OptionNone);
  q->registerProperty(/*no tr*/ "Segmentations/DefaultTerminologyEntry",
                      q,
                      "defaultTerminologyEntry",
                      SIGNAL(defaultTerminologyEntryChanged(QString)),
                      qSlicerSegmentationsSettingsPanel::tr("Default terminology entry"),
                      ctkSettingsPanel::OptionNone);

  this->AllowEditingHiddenSegmentComboBox->addItem(qSlicerSegmentationsSettingsPanel::tr("Ask user"), QMessageBox::InvalidRole);
  this->AllowEditingHiddenSegmentComboBox->addItem(qSlicerSegmentationsSettingsPanel::tr("Always make visible"), QMessageBox::Yes);
  this->AllowEditingHiddenSegmentComboBox->addItem(qSlicerSegmentationsSettingsPanel::tr("Always allow"), QMessageBox::No);
  q->registerProperty("Segmentations/ConfirmEditHiddenSegment", this->AllowEditingHiddenSegmentComboBox, "currentUserDataAsString", SIGNAL(currentIndexChanged(int)));

  this->DefaultOverwriteModeComboBox->addItem(qSlicerSegmentationsSettingsPanel::tr("Overwrite all"), QString(/*no tr*/ "OverwriteAllSegments"));
  this->DefaultOverwriteModeComboBox->addItem(qSlicerSegmentationsSettingsPanel::tr("Overwrite visible"), QString(/*no tr*/ "OverwriteVisibleSegments"));
  this->DefaultOverwriteModeComboBox->addItem(qSlicerSegmentationsSettingsPanel::tr("Allow overlap"), QString(/*no tr*/ "OverwriteNone"));
  q->registerProperty("Segmentations/DefaultOverwriteMode", this->DefaultOverwriteModeComboBox, "currentUserDataAsString", SIGNAL(currentIndexChanged(int)));

  // Actions to propagate to the application when settings are changed
  QObject::connect(this->AutoOpacitiesCheckBox, SIGNAL(toggled(bool)), q, SLOT(setAutoOpacities(bool)));
  QObject::connect(this->SurfaceSmoothingCheckBox, SIGNAL(toggled(bool)), q, SLOT(setDefaultSurfaceSmoothing(bool)));
  QObject::connect(this->UseTerminologyCheckBox, SIGNAL(toggled(bool)), q, SLOT(setUseTerminology(bool)));
  QObject::connect(this->EditDefaultTerminologyEntryPushButton, SIGNAL(clicked()), q, SLOT(onEditDefaultTerminologyEntry()));
  QObject::connect(this->DefaultOverwriteModeComboBox, SIGNAL(currentIndexChanged(QString)), q, SLOT(setDefaultOverwriteMode(QString)));

  // Update default segmentation node from settings when startup completed.
  QObject::connect(qSlicerApplication::application(), SIGNAL(startupCompleted()), q, SLOT(updateDefaultSegmentationNodeFromWidget()));
  // Update default overwrite mode from settings when startup completed.
  QObject::connect(qSlicerApplication::application(), SIGNAL(startupCompleted()), q, SLOT(updateDefaultOverwriteModeFromWidget()));
}

// --------------------------------------------------------------------------
// qSlicerSegmentationsSettingsPanel methods

// --------------------------------------------------------------------------
qSlicerSegmentationsSettingsPanel::qSlicerSegmentationsSettingsPanel(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSegmentationsSettingsPanelPrivate(*this))
{
  Q_D(qSlicerSegmentationsSettingsPanel);
  d->init();
}

// --------------------------------------------------------------------------
qSlicerSegmentationsSettingsPanel::~qSlicerSegmentationsSettingsPanel() = default;

// --------------------------------------------------------------------------
vtkSlicerSegmentationsModuleLogic* qSlicerSegmentationsSettingsPanel::segmentationsLogic() const
{
  Q_D(const qSlicerSegmentationsSettingsPanel);
  return d->SegmentationsLogic;
}

// --------------------------------------------------------------------------
void qSlicerSegmentationsSettingsPanel::setSegmentationsLogic(vtkSlicerSegmentationsModuleLogic* logic)
{
  Q_D(qSlicerSegmentationsSettingsPanel);
  d->SegmentationsLogic = logic;
}

// --------------------------------------------------------------------------
void qSlicerSegmentationsSettingsPanel::setAutoOpacities(bool on)
{
  Q_UNUSED(on);
}

// --------------------------------------------------------------------------
void qSlicerSegmentationsSettingsPanel::setDefaultSurfaceSmoothing(bool on)
{
  Q_UNUSED(on);
  if (this->segmentationsLogic())
  {
    this->segmentationsLogic()->SetDefaultSurfaceSmoothingEnabled(on);
  }
}

// --------------------------------------------------------------------------
void qSlicerSegmentationsSettingsPanel::setUseTerminology(bool on)
{
  Q_UNUSED(on);
}

// --------------------------------------------------------------------------
void qSlicerSegmentationsSettingsPanel::setDefaultOverwriteMode(QString mode)
{
  Q_UNUSED(mode);
  if (this->segmentationsLogic())
  {
    this->segmentationsLogic()->SetDefaultOverwriteMode(vtkMRMLSegmentEditorNode::ConvertOverwriteModeFromString(mode.toStdString().c_str()));
  }
}

// --------------------------------------------------------------------------
QString qSlicerSegmentationsSettingsPanel::defaultTerminologyEntry()
{
  Q_D(qSlicerSegmentationsSettingsPanel);
  return d->DefaultTerminologyString;
}

// --------------------------------------------------------------------------
void qSlicerSegmentationsSettingsPanel::setDefaultTerminologyEntry(QString terminologyStr)
{
  Q_D(qSlicerSegmentationsSettingsPanel);
  d->DefaultTerminologyString = terminologyStr;
  QString buttonText = tr("(set)");
  if (d->TerminologiesLogic && !terminologyStr.isEmpty())
  {
    vtkNew<vtkSlicerTerminologyEntry> entry;
    std::string terminologyStdStr = d->DefaultTerminologyString.toUtf8().constData();
    if (d->TerminologiesLogic->DeserializeTerminologyEntry(terminologyStdStr, entry))
    {
      buttonText.clear();
      buttonText += (entry->GetCategoryObject() && entry->GetCategoryObject()->GetCodeMeaning() ? entry->GetCategoryObject()->GetCodeMeaning() : "?");
      buttonText += "/";
      buttonText += (entry->GetTypeObject() && entry->GetTypeObject()->GetCodeMeaning() ? entry->GetTypeObject()->GetCodeMeaning() : "?");
    }
  }
  d->EditDefaultTerminologyEntryPushButton->setText(buttonText);
}

// --------------------------------------------------------------------------
void qSlicerSegmentationsSettingsPanel::onEditDefaultTerminologyEntry()
{
  Q_D(qSlicerSegmentationsSettingsPanel);

  if (!d->TerminologiesLogic)
  {
    return;
  }
  vtkNew<vtkSlicerTerminologyEntry> entry;
  std::string terminologyStdStr = d->DefaultTerminologyString.toUtf8().constData();
  d->TerminologiesLogic->DeserializeTerminologyEntry(terminologyStdStr, entry);
  if (!qSlicerTerminologySelectorDialog::getTerminology(entry, this))
  {
    // user cancelled
    return;
  }
  this->setDefaultTerminologyEntry(vtkSlicerTerminologiesModuleLogic::SerializeTerminologyEntry(entry).c_str());
  emit defaultTerminologyEntryChanged(d->DefaultTerminologyString);
}

// --------------------------------------------------------------------------
void qSlicerSegmentationsSettingsPanel::updateDefaultSegmentationNodeFromWidget()
{
  Q_D(qSlicerSegmentationsSettingsPanel);
  this->setDefaultSurfaceSmoothing(d->SurfaceSmoothingCheckBox->isChecked());
}

// --------------------------------------------------------------------------
void qSlicerSegmentationsSettingsPanel::updateDefaultOverwriteModeFromWidget()
{
  Q_D(qSlicerSegmentationsSettingsPanel);
  this->setDefaultOverwriteMode(d->DefaultOverwriteModeComboBox->currentData().toString());
}
