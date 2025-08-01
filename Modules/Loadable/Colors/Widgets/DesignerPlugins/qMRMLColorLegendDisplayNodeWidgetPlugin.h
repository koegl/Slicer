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

#ifndef __qMRMLColorLegendDisplayNodeWidgetPlugin_h
#define __qMRMLColorLegendDisplayNodeWidgetPlugin_h

#include <QtGlobal>
#include "qSlicerColorsModuleWidgetsAbstractPlugin.h"
#include "qSlicerColorsModuleWidgetsPluginsExport.h"

class Q_SLICER_QTMODULES_COLORS_WIDGETS_PLUGINS_EXPORT qMRMLColorLegendDisplayNodeWidgetPlugin
  : public QObject
  , public qSlicerColorsModuleWidgetsAbstractPlugin
{
  Q_OBJECT

public:
  qMRMLColorLegendDisplayNodeWidgetPlugin(QObject* newParent = nullptr);

  // Don't reimplement this method.
  QString group() const override;

  // You can reimplement these methods
  QString toolTip() const override;
  QString whatsThis() const override;

  QWidget* createWidget(QWidget* newParent) override;
  QString domXml() const override;
  QIcon icon() const override;
  QString includeFile() const override;
  bool isContainer() const override;
  QString name() const override;
};

#endif
