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

// Qt includes
#include <QDebug>

// Slicer includes
#include "qSlicerTemplateKeyModuleWidget.h"
#include "ui_qSlicerTemplateKeyModuleWidget.h"

//-----------------------------------------------------------------------------
class qSlicerTemplateKeyModuleWidgetPrivate : public Ui_qSlicerTemplateKeyModuleWidget
{
public:
  qSlicerTemplateKeyModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerTemplateKeyModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerTemplateKeyModuleWidgetPrivate::qSlicerTemplateKeyModuleWidgetPrivate() {}

//-----------------------------------------------------------------------------
// qSlicerTemplateKeyModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerTemplateKeyModuleWidget::qSlicerTemplateKeyModuleWidget(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerTemplateKeyModuleWidgetPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerTemplateKeyModuleWidget::~qSlicerTemplateKeyModuleWidget() {}

//-----------------------------------------------------------------------------
void qSlicerTemplateKeyModuleWidget::setup()
{
  Q_D(qSlicerTemplateKeyModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}
