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

// Qt includes
#include <QDebug>
#include <QDir>
#include <QDateTime>

// QtCore includes
#include "qSlicerSceneBundleReader.h"

// CTK includes
#include <ctkUtils.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLStorageNode.h>
#include <vtkMRMLMessageCollection.h>

// MRML Logic includes
#include <vtkMRMLApplicationLogic.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkNew.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// Slicer includes
#include "qSlicerCoreApplication.h"
#include "qSlicerCoreIOManager.h"

//-----------------------------------------------------------------------------
qSlicerSceneBundleReader::qSlicerSceneBundleReader(QObject* _parent)
  : Superclass(_parent)
{
}

//-----------------------------------------------------------------------------
QString qSlicerSceneBundleReader::description() const
{
  return tr("MRB Slicer Data Bundle");
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerSceneBundleReader::fileType() const
{
  return QString("SceneFile");
}

//-----------------------------------------------------------------------------
QStringList qSlicerSceneBundleReader::extensions() const
{
  return QStringList() << "*.mrb" << "*.zip" << "*.xar";
}

//-----------------------------------------------------------------------------
bool qSlicerSceneBundleReader::load(const qSlicerIO::IOProperties& properties)
{
  Q_ASSERT(properties.contains("fileName"));
  QString file = properties["fileName"].toString();

  // check for a relative path as the unzip will need an absolute one
  QFileInfo fileInfo(file);
  if (fileInfo.isRelative())
  {
    fileInfo = QFileInfo(QDir::currentPath(), file);
    file = fileInfo.absoluteFilePath();
  }
  bool clear = false;
  if (properties.contains("clear"))
  {
    clear = properties["clear"].toBool();
  }

  // Get all the nodes that have been around before loading
  QSet<vtkMRMLNode*> nodesPresentBeforeLoading;
  vtkCollection* nodes = this->mrmlScene()->GetNodes();
  for (int index = 0; index < nodes->GetNumberOfItems(); ++index)
  {
    vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(nodes->GetItemAsObject(index));
    nodesPresentBeforeLoading.insert(node);
  }

  bool success = this->mrmlScene()->ReadFromMRB(file.toUtf8(), clear, this->userMessages());

  // Get all the new nodes
  QStringList loadedNodeIds;
  nodes = this->mrmlScene()->GetNodes();
  for (int index = 0; index < nodes->GetNumberOfItems(); ++index)
  {
    vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(nodes->GetItemAsObject(index));
    if (nodesPresentBeforeLoading.contains(node) || !node || !node->GetID())
    {
      continue;
    }
    loadedNodeIds.append(node->GetID());
  }
  this->setLoadedNodes(loadedNodeIds);

  if (success)
  {
    // Set default scene file format to mrb
    qSlicerCoreIOManager* coreIOManager = qSlicerCoreApplication::application()->coreIOManager();
    coreIOManager->setDefaultSceneFileType(tr("Medical Reality Bundle") + " (.mrb)");
  }

  return success;
}
