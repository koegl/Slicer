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

#include "qSlicerMainWindow_p.h"

// Qt includes
#include <QAction>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QQueue>
#include <QSettings>
#include <QShowEvent>
#include <QSignalMapper>
#include <QStyle>
#include <QTableView>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>

// CTK includes
#include <ctkErrorLogWidget.h>
#include <ctkMessageBox.h>
#ifdef Slicer_USE_PYTHONQT
# include <ctkPythonConsole.h>
#endif
#ifdef Slicer_USE_QtTesting
# include <ctkQtTestingUtility.h>
#endif
#include <ctkVTKSliceView.h>
#include <ctkVTKWidgetsUtils.h>

// SlicerApp includes
#include "qSlicerAboutDialog.h"
#include "qSlicerActionsDialog.h"
#include "qSlicerApplication.h"
#include "qSlicerAbstractModule.h"
#if defined Slicer_USE_QtTesting && defined Slicer_BUILD_CLI_SUPPORT
# include "qSlicerCLIModuleWidgetEventPlayer.h"
#endif
#include "qSlicerCommandOptions.h"
#include "qSlicerCoreCommandOptions.h"
#include "qSlicerErrorReportDialog.h"
#ifdef Slicer_BUILD_EXTENSIONMANAGER_SUPPORT
# include "qSlicerExtensionsManagerModel.h"
#endif
#include "qSlicerLayoutManager.h"
#include "qSlicerModuleManager.h"
#include "qSlicerModulesMenu.h"
#include "qSlicerModuleSelectorToolBar.h"
#include "qSlicerIOManager.h"

// qMRML includes
#include <qMRMLSliceWidget.h>

// MRMLLogic includes
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLSliceLayerLogic.h>

// MRML includes
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLMessageCollection.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceCompositeNode.h>

// VTK includes
#include <vtkCollection.h>

namespace
{

//-----------------------------------------------------------------------------
void setThemeIcon(QAction* action, const QString& name)
{
  action->setIcon(QIcon::fromTheme(name, action->icon()));
}

} // end of anonymous namespace

//-----------------------------------------------------------------------------
// qSlicerMainWindowPrivate methods

qSlicerMainWindowPrivate::qSlicerMainWindowPrivate(qSlicerMainWindow& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerMainWindowPrivate::~qSlicerMainWindowPrivate() {}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::init()
{
  Q_Q(qSlicerMainWindow);
  this->setupUi(q);

  this->setupStatusBar();
  q->setupMenuActions();
  this->StartupState = q->saveState();
  q->restoreGUIState();
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::setupUi(QMainWindow* mainWindow)
{
  Q_Q(qSlicerMainWindow);

  this->Ui_qSlicerMainWindow::setupUi(mainWindow);

  qSlicerApplication* app = qSlicerApplication::application();

  //----------------------------------------------------------------------------
  // Recently loaded files
  //----------------------------------------------------------------------------
  QObject::connect(app->coreIOManager(), SIGNAL(newFileLoaded(qSlicerIO::IOProperties)), q, SLOT(onNewFileLoaded(qSlicerIO::IOProperties)));
  QObject::connect(app->coreIOManager(), SIGNAL(fileSaved(qSlicerIO::IOProperties)), q, SLOT(onFileSaved(qSlicerIO::IOProperties)));

  //----------------------------------------------------------------------------
  // Load DICOM
  //----------------------------------------------------------------------------
#ifndef Slicer_BUILD_DICOM_SUPPORT
  this->LoadDICOMAction->setVisible(false);
#endif

  //----------------------------------------------------------------------------
  // ModulePanel
  //----------------------------------------------------------------------------
  this->PanelDockWidget->toggleViewAction()->setText(qSlicerMainWindow::tr("Show &Module Panel"));
  this->PanelDockWidget->toggleViewAction()->setToolTip(qSlicerMainWindow::tr("Collapse/Expand the GUI panel and allows Slicer's viewers to occupy "
                                                                              "the entire application window"));
  this->PanelDockWidget->toggleViewAction()->setShortcut(QKeySequence("Ctrl+5"));
  this->AppearanceMenu->insertAction(this->ShowStatusBarAction, this->PanelDockWidget->toggleViewAction());

  //----------------------------------------------------------------------------
  // ModuleManager
  //----------------------------------------------------------------------------
  // Update the list of modules when they are loaded
  qSlicerModuleManager* moduleManager = qSlicerApplication::application()->moduleManager();
  if (!moduleManager)
  {
    qWarning() << "No module manager is created.";
  }

  QObject::connect(moduleManager, SIGNAL(moduleLoaded(QString)), q, SLOT(onModuleLoaded(QString)));

  QObject::connect(moduleManager, SIGNAL(moduleAboutToBeUnloaded(QString)), q, SLOT(onModuleAboutToBeUnloaded(QString)));

  //----------------------------------------------------------------------------
  // ModuleSelector ToolBar
  //----------------------------------------------------------------------------
  // Create a Module selector
  this->ModuleSelectorToolBar = new qSlicerModuleSelectorToolBar(qSlicerMainWindow::tr("Module Selection"), q);
  this->ModuleSelectorToolBar->setObjectName(QString::fromUtf8("ModuleSelectorToolBar"));
  this->ModuleSelectorToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
  this->ModuleSelectorToolBar->setModuleManager(moduleManager);
  q->insertToolBar(this->ModuleToolBar, this->ModuleSelectorToolBar);
  this->ModuleSelectorToolBar->stackUnder(this->ModuleToolBar);

  // Connect the selector with the module panel
  this->ModulePanel->setModuleManager(moduleManager);
  QObject::connect(this->ModuleSelectorToolBar, SIGNAL(moduleSelected(QString)), this->ModulePanel, SLOT(setModule(QString)));

  // Ensure the panel dock widget is visible
  QObject::connect(this->ModuleSelectorToolBar, SIGNAL(moduleSelected(QString)), this->PanelDockWidget, SLOT(show()));

  //----------------------------------------------------------------------------
  // MouseMode ToolBar
  //----------------------------------------------------------------------------
  // MouseMode toolBar should listen the MRML scene
  this->MouseModeToolBar->setApplicationLogic(qSlicerApplication::application()->applicationLogic());
  this->MouseModeToolBar->setMRMLScene(qSlicerApplication::application()->mrmlScene());
  QObject::connect(qSlicerApplication::application(), SIGNAL(mrmlSceneChanged(vtkMRMLScene*)), this->MouseModeToolBar, SLOT(setMRMLScene(vtkMRMLScene*)));

  QList<QAction*> toolBarActions;
  toolBarActions << this->MainToolBar->toggleViewAction();
  // toolBarActions << this->UndoRedoToolBar->toggleViewAction();
  toolBarActions << this->ModuleSelectorToolBar->toggleViewAction();
  toolBarActions << this->ModuleToolBar->toggleViewAction();
  toolBarActions << this->ViewToolBar->toggleViewAction();
  // toolBarActions << this->LayoutToolBar->toggleViewAction();
  toolBarActions << this->MouseModeToolBar->toggleViewAction();
  toolBarActions << this->ViewersToolBar->toggleViewAction();
  toolBarActions << this->DialogToolBar->toggleViewAction();
  this->WindowToolBarsMenu->addActions(toolBarActions);

  //----------------------------------------------------------------------------
  // Hide toolbars by default
  //----------------------------------------------------------------------------
  // Hide the Layout toolbar by default
  // The setVisibility slot of the toolbar is connected to the
  // QAction::triggered signal.
  // It's done for a long list of reasons. If you change this behavior, make sure
  // minimizing the application and restore it doesn't hide the module panel. check
  // also the geometry and the state of the menu qactions are correctly restored when
  // loading slicer.
  this->UndoRedoToolBar->toggleViewAction()->trigger();
  this->LayoutToolBar->toggleViewAction()->trigger();
  // q->removeToolBar(this->UndoRedoToolBar);
  // q->removeToolBar(this->LayoutToolBar);
  delete this->UndoRedoToolBar;
  this->UndoRedoToolBar = nullptr;
  delete this->LayoutToolBar;
  this->LayoutToolBar = nullptr;

  // Color of the spacing between views:
  QFrame* layoutFrame = new QFrame(this->CentralWidget);
  layoutFrame->setObjectName("CentralWidgetLayoutFrame");
  QHBoxLayout* centralLayout = new QHBoxLayout(this->CentralWidget);
  centralLayout->setContentsMargins(0, 0, 0, 0);
  centralLayout->addWidget(layoutFrame);

  //----------------------------------------------------------------------------
  // Layout Manager
  //----------------------------------------------------------------------------
  // Instantiate and assign the layout manager to the slicer application
  this->LayoutManager = new qSlicerLayoutManager(layoutFrame);
  // Prevent updates until the main window is shown to avoid detached viewports appear too early.
  this->LayoutManager->setEnabled(false);
  this->LayoutManager->setScriptedDisplayableManagerDirectory(qSlicerApplication::application()->slicerHome() + "/bin/Python/mrmlDisplayableManager");
  qSlicerApplication::application()->setLayoutManager(this->LayoutManager);
#ifdef Slicer_USE_QtTesting
  // we store this layout manager to the Object state property for QtTesting
  qSlicerApplication::application()->testingUtility()->addObjectStateProperty(qSlicerApplication::application()->layoutManager(), QString(/*no tr*/ "layout"));
  qSlicerApplication::application()->testingUtility()->addObjectStateProperty(this->ModuleSelectorToolBar->modulesMenu(), QString("currentModule"));
#endif
  // Layout manager should also listen the MRML scene
  // Note: This creates the OpenGL context for each view, so things like
  // multisampling should probably be configured before this line is executed.
  this->LayoutManager->setMRMLScene(qSlicerApplication::application()->mrmlScene());
  QObject::connect(qSlicerApplication::application(), SIGNAL(mrmlSceneChanged(vtkMRMLScene*)), this->LayoutManager, SLOT(setMRMLScene(vtkMRMLScene*)));
  QObject::connect(this->LayoutManager, SIGNAL(layoutChanged(int)), q, SLOT(onLayoutChanged(int)));

  // TODO: When module will be managed by the layoutManager, this should be
  //       revisited.
  QObject::connect(this->LayoutManager, SIGNAL(selectModule(QString)), this->ModuleSelectorToolBar, SLOT(selectModule(QString)));

  // Add menus for configuring compare view
  QMenu* compareMenu = new QMenu(qSlicerMainWindow::tr("Select number of viewers..."), mainWindow);
  compareMenu->setObjectName("CompareMenuView");
  compareMenu->addAction(this->ViewLayoutCompare_2_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_3_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_4_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_5_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_6_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_7_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_8_viewersAction);
  this->ViewLayoutCompareAction->setMenu(compareMenu);
  QObject::connect(compareMenu, SIGNAL(triggered(QAction*)), q, SLOT(onLayoutCompareActionTriggered(QAction*)));

  // ... and for widescreen version of compare view as well
  compareMenu = new QMenu(qSlicerMainWindow::tr("Select number of viewers..."), mainWindow);
  compareMenu->setObjectName("CompareMenuWideScreen");
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_2_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_3_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_4_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_5_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_6_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_7_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_8_viewersAction);
  this->ViewLayoutCompareWidescreenAction->setMenu(compareMenu);
  QObject::connect(compareMenu, SIGNAL(triggered(QAction*)), q, SLOT(onLayoutCompareWidescreenActionTriggered(QAction*)));

  // ... and for the grid version of the compare views
  compareMenu = new QMenu(qSlicerMainWindow::tr("Select number of viewers..."), mainWindow);
  compareMenu->setObjectName("CompareMenuGrid");
  compareMenu->addAction(this->ViewLayoutCompareGrid_2x2_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareGrid_3x3_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareGrid_4x4_viewersAction);
  this->ViewLayoutCompareGridAction->setMenu(compareMenu);
  QObject::connect(compareMenu, SIGNAL(triggered(QAction*)), q, SLOT(onLayoutCompareGridActionTriggered(QAction*)));

  // Authorize Drops action from outside
  q->setAcceptDrops(true);

  //----------------------------------------------------------------------------
  // View Toolbar
  //----------------------------------------------------------------------------
  // Populate the View ToolBar with all the layouts of the layout manager
  this->LayoutButton = new QToolButton(q);
  this->LayoutButton->setText(qSlicerMainWindow::tr("Layout"));
  this->LayoutButton->setMenu(this->LayoutMenu);
  this->LayoutButton->setPopupMode(QToolButton::InstantPopup);

  this->LayoutButton->setDefaultAction(this->ViewLayoutConventionalAction);

  QObject::connect(this->LayoutMenu, SIGNAL(triggered(QAction*)), q, SLOT(onLayoutActionTriggered(QAction*)));

  this->ViewToolBar->addWidget(this->LayoutButton);
  QObject::connect(this->ViewToolBar, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)), this->LayoutButton, SLOT(setToolButtonStyle(Qt::ToolButtonStyle)));

  //----------------------------------------------------------------------------
  // Viewers Toolbar
  //----------------------------------------------------------------------------
  // Viewers toolBar should listen the MRML scene
  this->ViewersToolBar->setApplicationLogic(qSlicerApplication::application()->applicationLogic());
  this->ViewersToolBar->setMRMLScene(qSlicerApplication::application()->mrmlScene());
  QObject::connect(qSlicerApplication::application(), SIGNAL(mrmlSceneChanged(vtkMRMLScene*)), this->ViewersToolBar, SLOT(setMRMLScene(vtkMRMLScene*)));

  //----------------------------------------------------------------------------
  // Undo/Redo Toolbar
  //----------------------------------------------------------------------------
  // Listen to the scene to enable/disable the undo/redo toolbuttons
  // q->qvtkConnect(qSlicerApplication::application()->mrmlScene(), vtkCommand::ModifiedEvent,
  //               q, SLOT(onMRMLSceneModified(vtkObject*)));
  // q->onMRMLSceneModified(qSlicerApplication::application()->mrmlScene());

  //----------------------------------------------------------------------------
  // Icons in the menu
  //----------------------------------------------------------------------------
  // Customize QAction icons with standard pixmaps

  this->CutAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  this->CopyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  this->PasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);

  setThemeIcon(this->FileExitAction, "application-exit");
  setThemeIcon(this->EditUndoAction, "edit-undo");
  setThemeIcon(this->EditRedoAction, "edit-redo");
  setThemeIcon(this->CutAction, "edit-cut");
  setThemeIcon(this->CopyAction, "edit-copy");
  setThemeIcon(this->PasteAction, "edit-paste");
  setThemeIcon(this->EditApplicationSettingsAction, "preferences-system");
  setThemeIcon(this->ModuleHomeAction, "go-home");

  //----------------------------------------------------------------------------
  // Error log widget
  //----------------------------------------------------------------------------

  this->ErrorLogWidget = new ctkErrorLogWidget;
  this->ErrorLogWidget->setErrorLogModel(qSlicerApplication::application()->errorLogModel());

  this->ErrorLogDockWidget = new QDockWidget(qSlicerMainWindow::tr("Error Log"));
  this->ErrorLogDockWidget->setObjectName("ErrorLogDockWidget");
  this->ErrorLogDockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
  this->ErrorLogDockWidget->setWidget(this->ErrorLogWidget);
  // Set default state
  mainWindow->addDockWidget(Qt::RightDockWidgetArea, this->ErrorLogDockWidget);
  this->ErrorLogDockWidget->hide();

  this->ErrorLogToggleViewAction = this->ErrorLogDockWidget->toggleViewAction();
  this->ErrorLogToggleViewAction->setText(qSlicerMainWindow::tr("&Error Log"));
  this->ErrorLogToggleViewAction->setToolTip(qSlicerMainWindow::tr("Show/hide Error Log window"));
  this->ErrorLogToggleViewAction->setShortcuts({ qSlicerMainWindow::tr("Ctrl+0") });

  QObject::connect(this->ErrorLogToggleViewAction, SIGNAL(toggled(bool)), q, SLOT(onErrorLogToggled(bool)));

  this->ViewMenu->insertAction(this->ModuleHomeAction, this->ErrorLogToggleViewAction);

  // Change orientation depending on where the widget is docked
  QObject::connect(this->ErrorLogDockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), q, SLOT(onErrorLogDockWidgetAreaChanged(Qt::DockWidgetArea)));

  // Dismiss the error message notification if the user interacted with the error log.
  QObject::connect(this->ErrorLogWidget, SIGNAL(userViewed()), q, SLOT(onUserViewedErrorLog()));

  //----------------------------------------------------------------------------
  // Python console
  //----------------------------------------------------------------------------
#ifdef Slicer_USE_PYTHONQT
  if (q->pythonConsole())
  {
    if (QSettings().value("Python/DockableWindow").toBool())
    {
      this->PythonConsoleDockWidget = new QDockWidget(qSlicerMainWindow::tr("Python Console"));
      this->PythonConsoleDockWidget->setObjectName("PythonConsoleDockWidget");
      this->PythonConsoleDockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
      this->PythonConsoleDockWidget->setWidget(q->pythonConsole());
      this->PythonConsoleToggleViewAction = this->PythonConsoleDockWidget->toggleViewAction();
      // Set default state
      q->addDockWidget(Qt::BottomDockWidgetArea, this->PythonConsoleDockWidget);
      this->PythonConsoleDockWidget->hide();
    }
    else
    {
      ctkPythonConsole* pythonConsole = q->pythonConsole();
      pythonConsole->setWindowTitle(qSlicerMainWindow::tr("Slicer Python Console"));
      pythonConsole->resize(600, 280);
      pythonConsole->hide();
      this->PythonConsoleToggleViewAction = new QAction("", this->ViewMenu);
      this->PythonConsoleToggleViewAction->setCheckable(true);
    }
    q->pythonConsole()->setScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->updatePythonConsolePalette();
    QObject::connect(q->pythonConsole(), SIGNAL(aboutToExecute(const QString&)), q, SLOT(onPythonConsoleUserInput(const QString&)));
    // Set up show/hide action
    this->PythonConsoleToggleViewAction->setText(qSlicerMainWindow::tr("&Python Console"));
    this->PythonConsoleToggleViewAction->setToolTip(qSlicerMainWindow::tr("Show Python Console window for controlling the application's data, user interface, and internals"));
    this->PythonConsoleToggleViewAction->setShortcuts({ qSlicerMainWindow::tr("Ctrl+3"), qSlicerMainWindow::tr("Ctrl+`") });
    QObject::connect(this->PythonConsoleToggleViewAction, SIGNAL(toggled(bool)), q, SLOT(onPythonConsoleToggled(bool)));
    this->ViewMenu->insertAction(this->ModuleHomeAction, this->PythonConsoleToggleViewAction);
    this->PythonConsoleToggleViewAction->setIcon(QIcon(":/python-icon.png"));
    this->DialogToolBar->addAction(this->PythonConsoleToggleViewAction);
  }
  else
  {
    qWarning("qSlicerMainWindowPrivate::setupUi: Failed to create Python console");
  }
#endif

  //----------------------------------------------------------------------------
  // Dockable Widget Area Definitions
  //----------------------------------------------------------------------------
  // Setting the left and right dock widget area to occupy the bottom corners
  // means the module panel is able to have more vertical space since it is the
  // usual left/right dockable widget. Since the module panel is typically not a
  // majority of the width dimension, this means the python console in the
  // bottom widget area still has a wide aspect ratio.
  // If application window is narrow then the Python console can be docked to the top
  // to use the full width of the application window.
  q->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  q->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::updatePythonConsolePalette()
{
  Q_Q(qSlicerMainWindow);
#ifdef Slicer_USE_PYTHONQT
  ctkPythonConsole* pythonConsole = q->pythonConsole();
  if (!pythonConsole)
  {
    return;
  }
  QPalette palette = qSlicerApplication::application()->palette();

  // pythonConsole->setBackgroundColor is not called, because by default
  // the background color of the current palette is used, which is good.

  // Color of the >> prompt. Blue in both bright and dark styles (brighter in dark style).
  pythonConsole->setPromptColor(palette.color(QPalette::Highlight));

  // Color of text that user types in the console. Blue in both bright and dark styles (brighter in dark style).
  pythonConsole->setCommandTextColor(palette.color(QPalette::Link));

  // Color of output of commands. Black in bright style, white in dark style.
  pythonConsole->setOutputTextColor(palette.color(QPalette::WindowText));

  // Color of error messages. Red in both bright and dark styles, but slightly brighter in dark style.
  QColor textColor = q->palette().color(QPalette::Normal, QPalette::Text);
  if (textColor.lightnessF() < 0.5)
  {
    // Light theme
    pythonConsole->setErrorTextColor(QColor::fromRgb(240, 0, 0)); // darker than Qt::red
  }
  else
  {
    // Dark theme
    pythonConsole->setErrorTextColor(QColor::fromRgb(255, 68, 68)); // lighter than Qt::red
  }

  // Color of welcome message (printed when the terminal is reset)
  // and "user input" (this does not seem to be used in Slicer).
  // Gray in both bright and dark styles.
  pythonConsole->setStdinTextColor(palette.color(QPalette::Disabled, QPalette::WindowText));   // gray
  pythonConsole->setWelcomeTextColor(palette.color(QPalette::Disabled, QPalette::WindowText)); // gray
#endif
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::setupRecentlyLoadedMenu(const QList<qSlicerIO::IOProperties>& fileProperties)
{
  Q_Q(qSlicerMainWindow);

  this->RecentlyLoadedMenu->setEnabled(fileProperties.size() > 0);
  this->RecentlyLoadedMenu->clear();

  QListIterator<qSlicerIO::IOProperties> iterator(fileProperties);
  iterator.toBack();
  while (iterator.hasPrevious())
  {
    qSlicerIO::IOProperties filePropertie = iterator.previous();
    QString fileName = filePropertie.value("fileName").toString();
    if (fileName.isEmpty())
    {
      continue;
    }
    QAction* action = this->RecentlyLoadedMenu->addAction(fileName, q, SLOT(onFileRecentLoadedActionTriggered()));
    action->setProperty("fileParameters", filePropertie);
    action->setEnabled(QFile::exists(fileName));
  }

  // Add separator and clear action
  this->RecentlyLoadedMenu->addSeparator();
  QAction* clearAction = this->RecentlyLoadedMenu->addAction(qSlicerMainWindow::tr("Clear History"), q, SLOT(onFileRecentLoadedActionTriggered()));
  clearAction->setProperty("clearMenu", QVariant(true));
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::filterRecentlyLoadedFileProperties()
{
  int numberOfFilesToKeep = QSettings().value("RecentlyLoadedFiles/NumberToKeep").toInt();

  // Remove extra element
  while (this->RecentlyLoadedFileProperties.size() > numberOfFilesToKeep)
  {
    this->RecentlyLoadedFileProperties.dequeue();
  }
}

//-----------------------------------------------------------------------------
QList<qSlicerIO::IOProperties> qSlicerMainWindowPrivate::readRecentlyLoadedFiles()
{
  QList<qSlicerIO::IOProperties> fileProperties;

  QSettings settings;
  int size = settings.beginReadArray("RecentlyLoadedFiles/RecentFiles");
  for (int i = 0; i < size; ++i)
  {
    settings.setArrayIndex(i);
    QVariant file = settings.value("file");
    qSlicerIO::IOProperties properties = file.toMap();
    properties["fileName"] = qSlicerApplication::application()->toSlicerHomeAbsolutePath(properties["fileName"].toString());
    fileProperties << properties;
  }
  settings.endArray();

  return fileProperties;
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::writeRecentlyLoadedFiles(const QList<qSlicerIO::IOProperties>& fileProperties)
{
  QSettings settings;
  settings.beginWriteArray("RecentlyLoadedFiles/RecentFiles", fileProperties.size());
  for (int i = 0; i < fileProperties.size(); ++i)
  {
    settings.setArrayIndex(i);
    qSlicerIO::IOProperties properties = fileProperties.at(i);
    properties["fileName"] = qSlicerApplication::application()->toSlicerHomeRelativePath(properties["fileName"].toString());
    settings.setValue("file", properties);
  }
  settings.endArray();
}

//-----------------------------------------------------------------------------
bool qSlicerMainWindowPrivate::confirmCloseApplication()
{
  Q_Q(qSlicerMainWindow);

  QString details;
  bool sceneModified = isSceneContentModifiedSinceRead(details);

  bool close = false;
  if (sceneModified)
  {
    QMessageBox* messageBox = new QMessageBox(QMessageBox::Warning,
                                              qSlicerMainWindow::tr("Save before exit?"),
                                              qSlicerMainWindow::tr("The scene has been modified. Do you want to save it before exit?"),
                                              QMessageBox::NoButton,
                                              q);
    QAbstractButton* saveButton = messageBox->addButton(qSlicerMainWindow::tr("Save"), QMessageBox::ActionRole);
    QAbstractButton* exitButton = messageBox->addButton(qSlicerMainWindow::tr("Exit (discard modifications)"), QMessageBox::ActionRole);
    messageBox->addButton(qSlicerMainWindow::tr("Cancel exit"), QMessageBox::RejectRole);

    if (!details.isEmpty())
    {
      messageBox->setDetailedText(details);
    }
    messageBox->exec();
    if (messageBox->clickedButton() == saveButton)
    {
      // \todo Check if the save data dialog was "applied" and close the
      // app in that case
      this->FileSaveSceneAction->trigger();
    }
    else if (messageBox->clickedButton() == exitButton)
    {
      close = true;
    }
    messageBox->deleteLater();
  }
  else
  {
    close = ctkMessageBox::confirmExit("MainWindow/DontConfirmExit", q);
  }
  return close;
}

//-----------------------------------------------------------------------------
bool qSlicerMainWindowPrivate::isSceneContentModifiedSinceRead(QString& details)
{
  Q_Q(qSlicerMainWindow);
  bool modifiedStorable = false;
  bool modifiedScene = false;
  details.clear();
  vtkMRMLScene* scene = qSlicerApplication::application()->mrmlScene();
  vtkNew<vtkCollection> modifiedStorableNodes;
  vtkNew<vtkCollection> modifiedNodesInScene;
  if (scene->GetStorableNodesModifiedSinceRead(modifiedStorableNodes))
  {
    for (int i = 0; i < modifiedStorableNodes->GetNumberOfItems(); i++)
    {
      vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(modifiedStorableNodes->GetItemAsObject(i));
      if (node)
      {
        if (!modifiedStorable)
        {
          details += qSlicerMainWindow::tr("Modifications in data files:") + "\n";
        }
        modifiedStorable = true;
        details += QString("- %1 (%2)\n").arg(node->GetName() ? node->GetName() : "unnamed").arg(node->GetID() ? node->GetID() : "unknown");
      }
    }
  }
  if (scene->GetModifiedSinceRead(modifiedNodesInScene))
  {
    for (int i = 0; i < modifiedNodesInScene->GetNumberOfItems(); i++)
    {
      vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(modifiedNodesInScene->GetItemAsObject(i));
      if (node)
      {
        if (modifiedStorableNodes->IsItemPresent(node))
        {
          // modified storable nodes are already listed
          continue;
        }
        if (!modifiedScene)
        {
          details += qSlicerMainWindow::tr("Modifications in the scene file:") + "\n";
        }
        modifiedScene = true;
        details += QString("- %1 (%2)\n").arg(node->GetName() ? node->GetName() : "unnamed").arg(node->GetID() ? node->GetID() : "unknown");
      }
    }
  }
  return modifiedStorable || modifiedScene;
}

//-----------------------------------------------------------------------------
bool qSlicerMainWindowPrivate::confirmCloseScene()
{
  Q_Q(qSlicerMainWindow);
  QString details;
  if (!isSceneContentModifiedSinceRead(details))
  {
    // no unsaved changes, no need to ask confirmation
    return true;
  }

  ctkMessageBox* confirmCloseMsgBox = new ctkMessageBox(q);
  confirmCloseMsgBox->setAttribute(Qt::WA_DeleteOnClose);
  confirmCloseMsgBox->setWindowTitle(qSlicerMainWindow::tr("Save before closing scene?"));
  confirmCloseMsgBox->setText(qSlicerMainWindow::tr("The scene has been modified. Do you want to save it before exit?"));

  // Use AcceptRole&RejectRole instead of Save&Discard because we would
  // like discard changes to be the default behavior.
  confirmCloseMsgBox->addButton(qSlicerMainWindow::tr("Close scene (discard modifications)"), QMessageBox::AcceptRole);
  confirmCloseMsgBox->addButton(qSlicerMainWindow::tr("Save scene"), QMessageBox::RejectRole);
  confirmCloseMsgBox->addButton(QMessageBox::Cancel);

  if (!details.isEmpty())
  {
    confirmCloseMsgBox->setDetailedText(details);
  }

  confirmCloseMsgBox->setDontShowAgainVisible(true);
  confirmCloseMsgBox->setDontShowAgainSettingsKey("MainWindow/DontConfirmSceneClose");
  confirmCloseMsgBox->setIcon(QMessageBox::Question);
  int resultCode = confirmCloseMsgBox->exec();
  if (resultCode == QMessageBox::Cancel)
  {
    return false;
  }
  if (resultCode != QMessageBox::AcceptRole)
  {
    if (!qSlicerApplication::application()->ioManager()->openSaveDataDialog())
    {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::setupStatusBar()
{
  Q_Q(qSlicerMainWindow);
  this->ErrorLogToolButton = new QToolButton();
  this->ErrorLogToolButton->setDefaultAction(this->ErrorLogToggleViewAction);
  q->statusBar()->addPermanentWidget(this->ErrorLogToolButton);

  QObject::connect(
    qSlicerApplication::application()->errorLogModel(), SIGNAL(entryAdded(ctkErrorLogLevel::LogLevel)), q, SLOT(onWarningsOrErrorsOccurred(ctkErrorLogLevel::LogLevel)));
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::setErrorLogIconHighlighted(bool highlighted)
{
  Q_Q(qSlicerMainWindow);
  QIcon defaultIcon = q->style()->standardIcon(QStyle::SP_MessageBoxCritical);
  QIcon icon = defaultIcon;
  if (!highlighted)
  {
    QIcon disabledIcon;
    disabledIcon.addPixmap(defaultIcon.pixmap(QSize(32, 32), QIcon::Disabled, QIcon::On), QIcon::Active, QIcon::On);
    icon = disabledIcon;
  }
  this->ErrorLogToggleViewAction->setIcon(icon);
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::addFavoriteModule(const QString& moduleName)
{
  Q_Q(qSlicerMainWindow);
  int index = this->FavoriteModules.indexOf(moduleName);
  if (index < 0)
  {
    return;
  }

  qSlicerAbstractCoreModule* coreModule = qSlicerApplication::application()->moduleManager()->module(moduleName);
  qSlicerAbstractModule* module = qobject_cast<qSlicerAbstractModule*>(coreModule);
  if (!module)
  {
    return;
  }

  QAction* action = module->action();
  if (!action || action->icon().isNull())
  {
    return;
  }

  Q_ASSERT(action->data().toString() == module->name());
  Q_ASSERT(action->text() == module->title());

  // find the location of where to add the action.
  // Note: FavoriteModules is sorted
  QAction* beforeAction = nullptr; // 0 means insert at end
  for (QAction* const toolBarAction : this->ModuleToolBar->actions())
  {
    bool isActionAFavoriteModule = (this->FavoriteModules.indexOf(toolBarAction->data().toString()) != -1);
    if (isActionAFavoriteModule && //
        this->FavoriteModules.indexOf(toolBarAction->data().toString()) > index)
    {
      beforeAction = toolBarAction;
      break;
    }
  }
  this->ModuleToolBar->insertAction(beforeAction, action);
  action->setParent(this->ModuleToolBar);
}

//-----------------------------------------------------------------------------
// qSlicerMainWindow methods

//-----------------------------------------------------------------------------
qSlicerMainWindow::qSlicerMainWindow(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerMainWindowPrivate(*this))
{
  Q_D(qSlicerMainWindow);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerMainWindow::qSlicerMainWindow(qSlicerMainWindowPrivate* pimpl, QWidget* windowParent)
  : Superclass(windowParent)
  , d_ptr(pimpl)
{
  // init() is called by derived class.
}

//-----------------------------------------------------------------------------
qSlicerMainWindow::~qSlicerMainWindow()
{
  Q_D(qSlicerMainWindow);
  // When quitting the application, the modules are unloaded (~qSlicerCoreApplication)
  // in particular the Colors module which deletes vtkMRMLColorLogic and removes
  // all the color nodes from the scene. If a volume was loaded in the views,
  // it would then try to render it with no color node and generate warnings.
  // There is no need to render anything so remove the volumes from the views.
  // It is maybe not the best place to do that but I couldn't think of anywhere
  // else.
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  vtkCollection* sliceLogics = layoutManager ? layoutManager->mrmlSliceLogics() : nullptr;
  if (sliceLogics)
  {
    for (int i = 0; i < sliceLogics->GetNumberOfItems(); ++i)
    {
      vtkMRMLSliceLogic* sliceLogic = vtkMRMLSliceLogic::SafeDownCast(sliceLogics->GetItemAsObject(i));
      if (!sliceLogic)
      {
        continue;
      }
      sliceLogic->GetSliceCompositeNode()->SetReferenceBackgroundVolumeID(nullptr);
      sliceLogic->GetSliceCompositeNode()->SetReferenceForegroundVolumeID(nullptr);
      sliceLogic->GetSliceCompositeNode()->SetReferenceLabelVolumeID(nullptr);
    }
  }
}

//-----------------------------------------------------------------------------
qSlicerModuleSelectorToolBar* qSlicerMainWindow::moduleSelector() const
{
  Q_D(const qSlicerMainWindow);
  return d->ModuleSelectorToolBar;
}

#ifdef Slicer_USE_PYTHONQT
//---------------------------------------------------------------------------
ctkPythonConsole* qSlicerMainWindow::pythonConsole() const
{
  return qSlicerCoreApplication::application()->pythonConsole();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onPythonConsoleUserInput(const QString& cmd)
{
  if (!cmd.isEmpty())
  {
    qDebug("Python console user input: %s", qPrintable(cmd));
  }
}
#endif

//---------------------------------------------------------------------------
void qSlicerMainWindow::onErrorLogDockWidgetAreaChanged(Qt::DockWidgetArea dockArea)
{
  Q_D(const qSlicerMainWindow);

  if (dockArea == Qt::DockWidgetArea::BottomDockWidgetArea || dockArea == Qt::DockWidgetArea::TopDockWidgetArea)
  {
    d->ErrorLogWidget->setLayoutOrientation(Qt::Horizontal);
  }
  else if (dockArea == Qt::DockWidgetArea::LeftDockWidgetArea || dockArea == Qt::DockWidgetArea::RightDockWidgetArea)
  {
    d->ErrorLogWidget->setLayoutOrientation(Qt::Vertical);
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onUserViewedErrorLog()
{
  Q_D(qSlicerMainWindow);
  d->setErrorLogIconHighlighted(false);
}

//---------------------------------------------------------------------------
ctkErrorLogWidget* qSlicerMainWindow::errorLogWidget() const
{
  Q_D(const qSlicerMainWindow);
  return d->ErrorLogWidget;
}

//---------------------------------------------------------------------------
QDockWidget* qSlicerMainWindow::errorLogDockWidget() const
{
  Q_D(const qSlicerMainWindow);
  return d->ErrorLogDockWidget;
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_ShowStatusBarAction_triggered(bool toggled)
{
  this->statusBar()->setVisible(toggled);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileFavoriteModulesAction_triggered()
{
  qSlicerApplication::application()->openSettingsDialog(qSlicerApplication::tr("Modules"));
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileAddDataAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openAddDataDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileLoadDataAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openAddDataDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileImportSceneAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openAddSceneDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileLoadSceneAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openLoadSceneDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileAddVolumeAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openAddVolumesDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileAddTransformAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openAddTransformDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileSaveSceneAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openSaveDataDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileExitAction_triggered()
{
  this->close();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_SDBSaveToDirectoryAction_triggered()
{
  // Q_D(qSlicerMainWindow);
  // open a file dialog to let the user choose where to save
  QString tempDir = qSlicerCoreApplication::application()->temporaryPath();
  QString saveDirName = QFileDialog::getExistingDirectory(this, tr("Slicer Data Bundle Directory (Select Empty Directory)"), tempDir, QFileDialog::ShowDirsOnly);
  if (saveDirName.isEmpty())
  {
    std::cout << "No directory name chosen!" << std::endl;
    return;
  }

  qSlicerIO::IOProperties properties;

  // pass in a screen shot
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (layoutManager)
  {
    QWidget* widget = layoutManager->viewport();
    QImage screenShot = ctk::grabVTKWidget(widget);
    properties["screenShot"] = screenShot;
  }

  properties["fileName"] = saveDirName;
  qSlicerCoreApplication::application()->coreIOManager()->saveNodes(QString("SceneFile"), properties);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_SDBSaveToMRBAction_triggered()
{
  //
  // open a file dialog to let the user choose where to save
  // make sure it was selected and add a .mrb to it if needed
  //
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save Data Bundle File"), "", tr("Medical Reality Bundle (*.mrb)"));

  if (fileName.isEmpty())
  {
    std::cout << "No directory name chosen!" << std::endl;
    return;
  }

  if (!fileName.endsWith(".mrb"))
  {
    fileName += QString(".mrb");
  }
  qSlicerIO::IOProperties properties;
  properties["fileName"] = fileName;
  qSlicerCoreApplication::application()->coreIOManager()->saveNodes(QString("SceneFile"), properties);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileCloseSceneAction_triggered()
{
  Q_D(qSlicerMainWindow);
  if (d->confirmCloseScene())
  {
    qDebug("Close main MRML scene");
    qSlicerCoreApplication::application()->mrmlScene()->Clear(false);
    // Make sure we don't remember the last scene's filename to prevent
    // accidentally overwriting the scene.
    qSlicerCoreApplication::application()->mrmlScene()->SetURL("");
    // Set default scene file format to .mrml
    qSlicerCoreIOManager* coreIOManager = qSlicerCoreApplication::application()->coreIOManager();
    coreIOManager->setDefaultSceneFileType(tr("MRML Scene") + " (.mrml)");
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_EditRecordMacroAction_triggered()
{
#ifdef Slicer_USE_QtTesting
  qSlicerApplication::application()->testingUtility()->recordTestsBySuffix(QString("xml"));
#endif
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_EditPlayMacroAction_triggered()
{
#ifdef Slicer_USE_QtTesting
  qSlicerApplication::application()->testingUtility()->openPlayerDialog();
#endif
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_EditUndoAction_triggered()
{
  qSlicerApplication::application()->mrmlScene()->Undo();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_EditRedoAction_triggered()
{
  qSlicerApplication::application()->mrmlScene()->Redo();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_ModuleHomeAction_triggered()
{
  this->setHomeModuleCurrent();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::setLayout(int layout)
{
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (!layoutManager)
  {
    return;
  }
  layoutManager->setLayout(layout);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::removeAllMaximizedViewNodes()
{
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (!layoutManager)
  {
    return;
  }
  layoutManager->removeAllMaximizedViewNodes();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::setLayoutNumberOfCompareViewRows(int num)
{
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (!layoutManager)
  {
    return;
  }
  layoutManager->setLayoutNumberOfCompareViewRows(num);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::setLayoutNumberOfCompareViewColumns(int num)
{
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (!layoutManager)
  {
    return;
  }
  layoutManager->setLayoutNumberOfCompareViewColumns(num);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onErrorLogToggled(bool toggled)
{
  Q_D(qSlicerMainWindow);
  if (toggled)
  {
    // Show & raise to make the error log appear if it was stacked below other docking widgets.
    d->ErrorLogDockWidget->show();
    d->ErrorLogDockWidget->activateWindow();
    d->ErrorLogDockWidget->raise();

    // Set focus to the message list to allow immediate interaction
    // (e.g., make the End key to jump to last message)
    QTableView* tableView = d->ErrorLogDockWidget->findChild<QTableView*>();
    if (tableView)
    {
      tableView->setFocus();
    }

    d->setErrorLogIconHighlighted(false);
  }
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::onPythonConsoleToggled(bool toggled)
{
  Q_D(qSlicerMainWindow);
#ifdef Slicer_USE_PYTHONQT
  ctkPythonConsole* pythonConsole = this->pythonConsole();
  if (!pythonConsole)
  {
    qCritical() << Q_FUNC_INFO << " failed: python console is not available";
    return;
  }
  if (d->PythonConsoleDockWidget)
  {
    // Dockable Python console
    if (toggled)
    {
      // Show & raise to make the console appear if it was stacked below other docking widgets.
      d->PythonConsoleDockWidget->show();
      d->PythonConsoleDockWidget->activateWindow();
      d->PythonConsoleDockWidget->raise();
      QTextEdit* textEditWidget = pythonConsole->findChild<QTextEdit*>();
      if (textEditWidget)
      {
        textEditWidget->setFocus();
      }
    }
  }
  else
  {
    // Independent Python console
    if (toggled)
    {
      pythonConsole->show();
      pythonConsole->activateWindow();
      pythonConsole->raise();
    }
    else
    {
      pythonConsole->hide();
    }
  }
#else
  Q_UNUSED(toggled);
#endif
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_WindowToolbarsResetToDefaultAction_triggered()
{
  Q_D(qSlicerMainWindow);
  this->restoreState(d->StartupState);
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::onFileRecentLoadedActionTriggered()
{
  Q_D(qSlicerMainWindow);

  QAction* loadRecentFileAction = qobject_cast<QAction*>(this->sender());
  Q_ASSERT(loadRecentFileAction);

  // Clear menu if it applies
  if (loadRecentFileAction->property("clearMenu").isValid())
  {
    d->RecentlyLoadedFileProperties.clear();
    d->setupRecentlyLoadedMenu(d->RecentlyLoadedFileProperties);
    return;
  }

  QVariant fileParameters = loadRecentFileAction->property("fileParameters");
  Q_ASSERT(fileParameters.isValid());

  qSlicerIO::IOProperties fileProperties = fileParameters.toMap();
  qSlicerIO::IOFileType fileType = static_cast<qSlicerIO::IOFileType>(fileProperties.find("fileType").value().toString());

  qSlicerApplication* app = qSlicerApplication::application();

  vtkNew<vtkMRMLMessageCollection> userMessages;
  bool success = app->coreIOManager()->loadNodes(fileType, fileProperties, nullptr, userMessages);
  qSlicerIOManager::showLoadNodesResultDialog(success, userMessages);
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::closeEvent(QCloseEvent* event)
{
  Q_D(qSlicerMainWindow);

  // This is necessary because of a Qt bug on MacOS.
  // (https://bugreports.qt.io/browse/QTBUG-43344).
  // This flag prevents a second close event to be handled.
  if (d->IsClosing)
  {
    return;
  }
  d->IsClosing = true;

  if (d->confirmCloseApplication())
  {
    // Proceed with closing the application

    // Exit current module to leave it a chance to change the UI (e.g. layout)
    // before writing settings.
    d->ModuleSelectorToolBar->selectModule("");

    this->saveGUIState();
    event->accept();

    QTimer::singleShot(0, qApp, SLOT(closeAllWindows()));
  }
  else
  {
    // Request is cancelled, application will not be closed
    event->ignore();
    d->IsClosing = false;
  }
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::showEvent(QShowEvent* event)
{
  Q_D(qSlicerMainWindow);
  this->Superclass::showEvent(event);
  if (!d->WindowInitialShowCompleted)
  {
    d->WindowInitialShowCompleted = true;

    // Show layout (updates were disabled until now to prevent detached viewports
    // appear before the main window appears).
    qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
    if (layoutManager)
    {
      layoutManager->setEnabled(true);
      layoutManager->refresh();
    }

    this->disclaimer();
    this->pythonConsoleInitialDisplay();

#ifdef Slicer_BUILD_EXTENSIONMANAGER_SUPPORT
    qSlicerApplication* app = qSlicerApplication::application();
    if (app && app->extensionsManagerModel())
    {
      connect(app->extensionsManagerModel(), SIGNAL(extensionUpdatesAvailable(bool)), this, SLOT(setExtensionUpdatesAvailable(bool)));
      this->setExtensionUpdatesAvailable(!app->extensionsManagerModel()->availableUpdateExtensions().empty());
    }
#endif

    emit initialWindowShown();
  }
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::pythonConsoleInitialDisplay()
{
  Q_D(qSlicerMainWindow);
#ifdef Slicer_USE_PYTHONQT
  qSlicerApplication* app = qSlicerApplication::application();
  if (qSlicerCoreApplication::testAttribute(qSlicerCoreApplication::AA_DisablePython))
  {
    return;
  }
  if (app->commandOptions()->showPythonConsole() && d->PythonConsoleDockWidget)
  {
    d->PythonConsoleDockWidget->show();
  }
#endif
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::disclaimer()
{
  qSlicerCoreApplication* app = qSlicerCoreApplication::application();
  if (app->testAttribute(qSlicerCoreApplication::AA_EnableTesting) || //
      !app->coreCommandOptions()->pythonCode().isEmpty() ||           //
      !app->coreCommandOptions()->pythonScript().isEmpty())
  {
    return;
  }

  QString message = QString(Slicer_DISCLAIMER_AT_STARTUP);
  if (message.isEmpty())
  {
    // No disclaimer message to show, skip the popup
    return;
  }

  // Replace "%1" in the text by the name and version of the application
  message = message.arg(app->applicationName() + " " + app->applicationVersion());

  ctkMessageBox* disclaimerMessage = new ctkMessageBox(this);
  disclaimerMessage->setAttribute(Qt::WA_DeleteOnClose, true);
  disclaimerMessage->setText(message);
  disclaimerMessage->setIcon(QMessageBox::Information);
  disclaimerMessage->setDontShowAgainSettingsKey("MainWindow/DontShowDisclaimerMessage");
  QTimer::singleShot(0, disclaimerMessage, SLOT(exec()));
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::setupMenuActions()
{
  Q_D(qSlicerMainWindow);

  d->ViewLayoutConventionalAction->setData(vtkMRMLLayoutNode::SlicerLayoutConventionalView);
  d->ViewLayoutConventionalWidescreenAction->setData(vtkMRMLLayoutNode::SlicerLayoutConventionalWidescreenView);
  d->ViewLayoutConventionalPlotAction->setData(vtkMRMLLayoutNode::SlicerLayoutConventionalPlotView);
  d->ViewLayoutFourUpAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourUpView);
  d->ViewLayoutFourUpPlotAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourUpPlotView);
  d->ViewLayoutFourUpPlotTableAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourUpPlotTableView);
  d->ViewLayoutFourUpTableAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourUpTableView);
  d->ViewLayoutDual3DAction->setData(vtkMRMLLayoutNode::SlicerLayoutDual3DView);
  d->ViewLayoutTriple3DAction->setData(vtkMRMLLayoutNode::SlicerLayoutTriple3DEndoscopyView);
  d->ViewLayoutOneUp3DAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUp3DView);
  d->ViewLayout3DTableAction->setData(vtkMRMLLayoutNode::SlicerLayout3DTableView);
  d->ViewLayoutOneUpPlotAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUpPlotView);
  d->ViewLayoutOneUpRedSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUpRedSliceView);
  d->ViewLayoutOneUpYellowSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUpYellowSliceView);
  d->ViewLayoutOneUpGreenSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUpGreenSliceView);
  d->ViewLayoutTabbed3DAction->setData(vtkMRMLLayoutNode::SlicerLayoutTabbed3DView);
  d->ViewLayoutTabbedSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutTabbedSliceView);
  d->ViewLayoutCompareAction->setData(vtkMRMLLayoutNode::SlicerLayoutCompareView);
  d->ViewLayoutCompareWidescreenAction->setData(vtkMRMLLayoutNode::SlicerLayoutCompareWidescreenView);
  d->ViewLayoutCompareGridAction->setData(vtkMRMLLayoutNode::SlicerLayoutCompareGridView);
  d->ViewLayoutThreeOverThreeAction->setData(vtkMRMLLayoutNode::SlicerLayoutThreeOverThreeView);
  d->ViewLayoutThreeOverThreePlotAction->setData(vtkMRMLLayoutNode::SlicerLayoutThreeOverThreePlotView);
  d->ViewLayoutFourOverFourAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourOverFourView);
  d->ViewLayoutTwoOverTwoAction->setData(vtkMRMLLayoutNode::SlicerLayoutTwoOverTwoView);
  d->ViewLayoutSideBySideAction->setData(vtkMRMLLayoutNode::SlicerLayoutSideBySideView);
  d->ViewLayoutFourByThreeSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourByThreeSliceView);
  d->ViewLayoutFourByTwoSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourByTwoSliceView);
  d->ViewLayoutFiveByTwoSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutFiveByTwoSliceView);
  d->ViewLayoutThreeByThreeSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutThreeByThreeSliceView);
  d->ViewLayoutDualMonitorFourUpViewAction->setData(vtkMRMLLayoutNode::SlicerLayoutDualMonitorFourUpView);

  d->ViewLayoutCompare_2_viewersAction->setData(2);
  d->ViewLayoutCompare_3_viewersAction->setData(3);
  d->ViewLayoutCompare_4_viewersAction->setData(4);
  d->ViewLayoutCompare_5_viewersAction->setData(5);
  d->ViewLayoutCompare_6_viewersAction->setData(6);
  d->ViewLayoutCompare_7_viewersAction->setData(7);
  d->ViewLayoutCompare_8_viewersAction->setData(8);

  d->ViewLayoutCompareWidescreen_2_viewersAction->setData(2);
  d->ViewLayoutCompareWidescreen_3_viewersAction->setData(3);
  d->ViewLayoutCompareWidescreen_4_viewersAction->setData(4);
  d->ViewLayoutCompareWidescreen_5_viewersAction->setData(5);
  d->ViewLayoutCompareWidescreen_6_viewersAction->setData(6);
  d->ViewLayoutCompareWidescreen_7_viewersAction->setData(7);
  d->ViewLayoutCompareWidescreen_8_viewersAction->setData(8);

  d->ViewLayoutCompareGrid_2x2_viewersAction->setData(2);
  d->ViewLayoutCompareGrid_3x3_viewersAction->setData(3);
  d->ViewLayoutCompareGrid_4x4_viewersAction->setData(4);

  d->setErrorLogIconHighlighted(false);

#ifdef Slicer_USE_PYTHONQT
  if (this->pythonConsole())
  {
    this->pythonConsole()->installEventFilter(this);
  }
#endif

  qSlicerApplication* app = qSlicerApplication::application();

#ifdef Slicer_BUILD_EXTENSIONMANAGER_SUPPORT
  d->ViewExtensionsManagerAction->setVisible(app->revisionUserSettings()->value("Extensions/ManagerEnabled").toBool());
#else
  d->ViewExtensionsManagerAction->setVisible(false);
  d->WindowToolBarsMenu->removeAction(d->ViewExtensionsManagerAction);
#endif

#if defined Slicer_USE_QtTesting && defined Slicer_BUILD_CLI_SUPPORT
  if (app->commandOptions()->enableQtTesting() || //
      app->userSettings()->value("QtTesting/Enabled").toBool())
  {
    d->EditPlayMacroAction->setVisible(true);
    d->EditRecordMacroAction->setVisible(true);
    app->testingUtility()->addPlayer(new qSlicerCLIModuleWidgetEventPlayer());
  }
#endif
  Q_UNUSED(app);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_LoadDICOMAction_triggered()
{
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (!layoutManager)
  {
    return;
  }
  layoutManager->setCurrentModule(/*no tr*/ "DICOM");
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onWarningsOrErrorsOccurred(ctkErrorLogLevel::LogLevel logLevel)
{
  Q_D(qSlicerMainWindow);
  if (logLevel > ctkErrorLogLevel::Info)
  {
    d->setErrorLogIconHighlighted(true);
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_EditApplicationSettingsAction_triggered()
{
  qSlicerApplication::application()->openSettingsDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::addFileToRecentFiles(const qSlicerIO::IOProperties& fileProperties)
{
  Q_D(qSlicerMainWindow);

  // Remove previous instance of the same file name. Since the file name can be slightly different
  // (different directory separator, etc.) we don't do a binary compare but compare QFileInfo.
  QString fileName = fileProperties.value("fileName").toString();
  if (fileName.isEmpty())
  {
    return;
  }
  QFileInfo newFileInfo(fileName);
  for (auto propertiesIt = d->RecentlyLoadedFileProperties.begin(); propertiesIt != d->RecentlyLoadedFileProperties.end();)
  {
    QFileInfo existingFileInfo(propertiesIt->value("fileName").toString());
    if (newFileInfo == existingFileInfo)
    {
      // remove previous instance
      propertiesIt = d->RecentlyLoadedFileProperties.erase(propertiesIt);
    }
    else
    {
      propertiesIt++;
    }
  }

  d->RecentlyLoadedFileProperties.enqueue(fileProperties);
  d->filterRecentlyLoadedFileProperties();
  d->setupRecentlyLoadedMenu(d->RecentlyLoadedFileProperties);
  // Keep the settings up-to-date
  qSlicerMainWindowPrivate::writeRecentlyLoadedFiles(d->RecentlyLoadedFileProperties);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onNewFileLoaded(const qSlicerIO::IOProperties& fileProperties)
{
  this->addFileToRecentFiles(fileProperties);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onFileSaved(const qSlicerIO::IOProperties& fileProperties)
{
  Q_D(qSlicerMainWindow);
  QString fileName = fileProperties["fileName"].toString();
  if (fileName.isEmpty())
  {
    return;
  }
  // Adding every saved file to the recent files list could quickly overwrite the entire list,
  // therefore we only add the scene file.
  if (fileName.endsWith(".mrml", Qt::CaseInsensitive) //
      || fileName.endsWith(".mrb", Qt::CaseInsensitive))
  {
    // Scene file properties do not contain fileType and it contains screenshot,
    // which can cause complication when attempted to be stored,
    // therefore we create a new clean property set.
    qSlicerIO::IOProperties properties;
    properties["fileName"] = fileName;
    properties["fileType"] = QString("SceneFile");
    this->addFileToRecentFiles(properties);
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_CopyAction_triggered()
{
  QWidget* focused = QApplication::focusWidget();
  if (focused != nullptr)
  {
    QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier));
    QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier));
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_PasteAction_triggered()
{
  QWidget* focused = QApplication::focusWidget();
  if (focused != nullptr)
  {
    QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier));
    QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_V, Qt::ControlModifier));
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_CutAction_triggered()
{
  QWidget* focused = QApplication::focusWidget();
  if (focused != nullptr)
  {
    QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier));
    QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_X, Qt::ControlModifier));
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_ViewExtensionsManagerAction_triggered()
{
#ifdef Slicer_BUILD_EXTENSIONMANAGER_SUPPORT
  qSlicerApplication* app = qSlicerApplication::application();
  app->openExtensionsManagerDialog();
#endif
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onModuleLoaded(const QString& moduleName)
{
  Q_D(qSlicerMainWindow);
  // Add action to favorite module toolbar (if it is a favorite module)
  if (d->FavoriteModules.contains(moduleName))
  {
    d->addFavoriteModule(moduleName);
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onModuleAboutToBeUnloaded(const QString& moduleName)
{
  Q_D(qSlicerMainWindow);

  if (d->ModuleSelectorToolBar->selectedModule() == moduleName)
  {
    d->ModuleSelectorToolBar->selectModule("");
  }

  for (QAction* const action : d->ModuleToolBar->actions())
  {
    if (action->data().toString() == moduleName)
    {
      d->ModuleToolBar->removeAction(action);
      return;
    }
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onMRMLSceneModified(vtkObject* sender)
{
  Q_UNUSED(sender);
  // Q_D(qSlicerMainWindow);
  //
  // vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sender);
  // if (scene && scene->IsBatchProcessing())
  //   {
  //   return;
  //   }
  // d->EditUndoAction->setEnabled(scene && scene->GetNumberOfUndoLevels());
  // d->EditRedoAction->setEnabled(scene && scene->GetNumberOfRedoLevels());
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onLayoutActionTriggered(QAction* action)
{
  Q_D(qSlicerMainWindow);
  bool found = false;
  // std::cerr << "onLayoutActionTriggered: " << action->text().toStdString() << std::endl;
  for (QAction* const maction : d->LayoutMenu->actions())
  {
    if (action->text() == maction->text())
    {
      found = true;
      break;
    }
  }

  if (found && !action->data().isNull())
  {
    this->setLayout(action->data().toInt());
    this->removeAllMaximizedViewNodes();
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onLayoutCompareActionTriggered(QAction* action)
{
  Q_D(qSlicerMainWindow);

  // std::cerr << "onLayoutCompareActionTriggered: " << action->text().toStdString() << std::endl;

  // we need to communicate both the layout change and the number of viewers.
  this->setLayout(d->ViewLayoutCompareAction->data().toInt());
  this->removeAllMaximizedViewNodes();
  this->setLayoutNumberOfCompareViewRows(action->data().toInt());
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onLayoutCompareWidescreenActionTriggered(QAction* action)
{
  Q_D(qSlicerMainWindow);

  // std::cerr << "onLayoutCompareWidescreenActionTriggered: " << action->text().toStdString() << std::endl;

  // we need to communicate both the layout change and the number of viewers.
  this->setLayout(d->ViewLayoutCompareWidescreenAction->data().toInt());
  this->removeAllMaximizedViewNodes();
  this->setLayoutNumberOfCompareViewColumns(action->data().toInt());
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onLayoutCompareGridActionTriggered(QAction* action)
{
  Q_D(qSlicerMainWindow);

  // std::cerr << "onLayoutCompareGridActionTriggered: " << action->text().toStdString() << std::endl;

  // we need to communicate both the layout change and the number of viewers.
  this->setLayout(d->ViewLayoutCompareGridAction->data().toInt());
  this->removeAllMaximizedViewNodes();
  this->setLayoutNumberOfCompareViewRows(action->data().toInt());
  this->setLayoutNumberOfCompareViewColumns(action->data().toInt());
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onLayoutChanged(int layout)
{
  Q_D(qSlicerMainWindow);

  // Update the layout button icon with the current view layout.

  // Actions with a menu are ignored, because they are just submenus without
  // data assigned, so they should never be triggered (they could be triggered
  // at startup, when layout is set to SlicerLayoutInitialView = 0).

  for (QAction* const action : d->LayoutMenu->actions())
  {
    if (!action->menu() && action->data().toInt() == layout)
    {
      d->LayoutButton->setDefaultAction(action);
    }
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::dragEnterEvent(QDragEnterEvent* event)
{
  qSlicerApplication::application()->ioManager()->dragEnterEvent(event);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::dropEvent(QDropEvent* event)
{
  qSlicerApplication::application()->ioManager()->dropEvent(event);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::restoreGUIState(bool force /*=false*/)
{
  Q_D(qSlicerMainWindow);
  QSettings settings;
  settings.beginGroup("MainWindow");
  this->setToolButtonStyle(settings.value("ShowToolButtonText").toBool() ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
  bool restore = settings.value("RestoreGeometry", false).toBool();
  if (restore || force)
  {
    this->restoreGeometry(settings.value("geometry").toByteArray());
    this->restoreState(settings.value("windowState").toByteArray());
    qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
    if (layoutManager)
    {
      layoutManager->setLayout(settings.value("layout").toInt());
    }
  }
  settings.endGroup();
  d->FavoriteModules << settings.value("Modules/FavoriteModules").toStringList();

  for (const qSlicerIO::IOProperties& fileProperty : qSlicerMainWindowPrivate::readRecentlyLoadedFiles())
  {
    d->RecentlyLoadedFileProperties.enqueue(fileProperty);
  }
  d->filterRecentlyLoadedFileProperties();
  d->setupRecentlyLoadedMenu(d->RecentlyLoadedFileProperties);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::saveGUIState(bool force /*=false*/)
{
  Q_D(qSlicerMainWindow);
  QSettings settings;
  settings.beginGroup("MainWindow");
  bool restore = settings.value("RestoreGeometry", false).toBool();
  if (restore || force)
  {
    settings.setValue("geometry", this->saveGeometry());
    settings.setValue("windowState", this->saveState());
    qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
    if (layoutManager)
    {
      settings.setValue("layout", layoutManager->layout());
    }
  }
  settings.endGroup();
  qSlicerMainWindowPrivate::writeRecentlyLoadedFiles(d->RecentlyLoadedFileProperties);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::setHomeModuleCurrent()
{
  Q_D(qSlicerMainWindow);
  QSettings settings;
  QString homeModule = settings.value("Modules/HomeModule").toString();
  d->ModuleSelectorToolBar->selectModule(homeModule);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::restoreToolbars()
{
  Q_D(qSlicerMainWindow);
  this->restoreState(d->StartupState);
}

//---------------------------------------------------------------------------
bool qSlicerMainWindow::eventFilter(QObject* object, QEvent* event)
{
  Q_D(qSlicerMainWindow);
#ifdef Slicer_USE_PYTHONQT
  if (object == this->pythonConsole())
  {
    if (event->type() == QEvent::Hide)
    {
      bool wasBlocked = d->PythonConsoleToggleViewAction->blockSignals(true);
      d->PythonConsoleToggleViewAction->setChecked(false);
      d->PythonConsoleToggleViewAction->blockSignals(wasBlocked);
    }
  }
#endif
  return this->Superclass::eventFilter(object, event);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::changeEvent(QEvent* event)
{
  Q_D(qSlicerMainWindow);
  switch (event->type())
  {
    case QEvent::PaletteChange:
    {
      d->updatePythonConsolePalette();
      break;
    }
    default: break;
  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::setExtensionUpdatesAvailable(bool updateAvailable)
{
#ifdef Slicer_BUILD_EXTENSIONMANAGER_SUPPORT
  Q_D(qSlicerMainWindow);
  qSlicerApplication* app = qSlicerApplication::application();
  if (!app || !app->revisionUserSettings()->value("Extensions/ManagerEnabled").toBool())
  {
    return;
  }

  // Check if there was a change
  const char extensionUpdateAvailablePropertyName[] = "extensionUpdateAvailable";
  if (d->ViewExtensionsManagerAction->property(extensionUpdateAvailablePropertyName).toBool() == updateAvailable)
  {
    // no change
    return;
  }
  d->ViewExtensionsManagerAction->setProperty(extensionUpdateAvailablePropertyName, updateAvailable);

  if (updateAvailable)
  {
    d->ViewExtensionsManagerAction->setIcon(QIcon(":/Icons/ExtensionNotificationIcon.png"));
  }
  else
  {
    d->ViewExtensionsManagerAction->setIcon(QIcon(":/Icons/ExtensionDefaultIcon.png"));
  }
#endif
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FavoriteModulesChanged()
{
  Q_D(qSlicerMainWindow);

  // Update favorite module name list
  d->FavoriteModules = QSettings().value("Modules/FavoriteModules").toStringList();

  // Update favorite module toolbar
  d->ModuleToolBar->clear();
  for (const QString& moduleName : d->FavoriteModules)
  {
    if (d->FavoriteModules.contains(moduleName))
    {
      d->addFavoriteModule(moduleName);
    }
  }
}
