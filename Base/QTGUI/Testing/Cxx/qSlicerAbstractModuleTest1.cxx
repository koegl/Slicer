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

#include "qSlicerAbstractModule.h"

// STD includes

class AModule : public qSlicerAbstractModule
{
public:
  QString title() const override { return "A Title"; }
  qSlicerAbstractModuleRepresentation* createWidgetRepresentation() override { return nullptr; }

  vtkMRMLAbstractLogic* createLogic() override { return nullptr; }

protected:
  void setup() override {}
};

int qSlicerAbstractModuleTest1(int, char*[])
{
  AModule module;

  return EXIT_SUCCESS;
}
