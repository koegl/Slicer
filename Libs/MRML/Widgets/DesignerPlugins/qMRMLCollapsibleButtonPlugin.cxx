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

// qMRML includes
#include "qMRMLCollapsibleButtonPlugin.h"
#include "qMRMLCollapsibleButton.h"

//-----------------------------------------------------------------------------
qMRMLCollapsibleButtonPlugin::qMRMLCollapsibleButtonPlugin(QObject* _parent)
  : QObject(_parent)
{
}

//-----------------------------------------------------------------------------
QWidget* qMRMLCollapsibleButtonPlugin::createWidget(QWidget* _parent)
{
  qMRMLCollapsibleButton* _widget = new qMRMLCollapsibleButton(_parent);
  return _widget;
}

//-----------------------------------------------------------------------------
QString qMRMLCollapsibleButtonPlugin::domXml() const
{
  return "<widget class=\"qMRMLCollapsibleButton\" \
          name=\"MRMLCollapsibleButton\">\n"
         "</widget>\n";
}

//-----------------------------------------------------------------------------
QString qMRMLCollapsibleButtonPlugin::includeFile() const
{
  return "qMRMLCollapsibleButton.h";
}

//-----------------------------------------------------------------------------
bool qMRMLCollapsibleButtonPlugin::isContainer() const
{
  return true;
}

//-----------------------------------------------------------------------------
QString qMRMLCollapsibleButtonPlugin::name() const
{
  return "qMRMLCollapsibleButton";
}
