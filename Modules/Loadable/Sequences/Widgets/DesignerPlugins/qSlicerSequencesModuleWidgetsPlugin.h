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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef qSlicerSequencesModuleWidgetsPlugin_h
#define qSlicerSequencesModuleWidgetsPlugin_h

// Qt includes
#include <QtUiPlugin/QDesignerCustomWidgetCollectionInterface>

// SequenceBrowser includes
#include "qMRMLSequenceBrowserPlayWidgetPlugin.h"
#include "qMRMLSequenceBrowserSeekWidgetPlugin.h"
#include "qMRMLSequenceBrowserToolBarPlugin.h"
#include "qMRMLSequenceEditWidgetPlugin.h"

// \class Group the plugins in one library
class Q_SLICER_MODULE_SEQUENCES_WIDGETS_PLUGINS_EXPORT qSlicerSequenceBrowserModuleWidgetsPlugin
  : public QObject
  , public QDesignerCustomWidgetCollectionInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDesignerCustomWidgetInterface")
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface);

public:
  QList<QDesignerCustomWidgetInterface*> customWidgets() const override
  {
    QList<QDesignerCustomWidgetInterface*> plugins;
    plugins << new qMRMLSequenceBrowserPlayWidgetPlugin;
    plugins << new qMRMLSequenceBrowserSeekWidgetPlugin;
    plugins << new qMRMLSequenceBrowserToolBarPlugin;
    plugins << new qMRMLSequenceEditWidgetPlugin;
    return plugins;
  }
};

#endif
