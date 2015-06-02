#include <QApplication>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QStatusBar>
#include "render-mode.hpp"
#include "scene.hpp"
#include "state.hpp"
#include "tool/move-camera.hpp"
#include "view/gl-widget.hpp"
#include "view/main-widget.hpp"
#include "view/main-window.hpp"
#include "view/tool-tip.hpp"
#include "view/util.hpp"

struct ViewMainWindow :: Impl {
  ViewMainWindow* self;
  ViewMainWidget& mainWidget;
  QStatusBar&     statusBar;
  QLabel&         messageLabel;
  QLabel&         numFacesLabel;

  Impl (ViewMainWindow* s, Config& config, Cache& cache) 
    : self          (s) 
    , mainWidget    (*new ViewMainWidget (*this->self, config, cache))
    , statusBar     (*new QStatusBar)
    , messageLabel  (*new QLabel)
    , numFacesLabel (*new QLabel)
  {
    this->self->setCentralWidget (&this->mainWidget);

    this->setupShortcuts ();
    this->setupMenuBar   ();
    this->setupStatusBar ();
  }

  void setupShortcuts () {
    auto addShortcut = [this] (const QKeySequence& keySequence, const std::function <void ()>& f) {
      QShortcut* s = new QShortcut (keySequence, this->self);

      QObject::connect (s, &QShortcut::activated, f);
    };

    addShortcut (Qt::Key_Backspace, [this] () {
      if (this->mainWidget.glWidget ().state ().hasTool ()) {
        this->mainWidget.glWidget ().state ().resetTool ();
      }
#ifndef NDEBUG
      else {
        QApplication::quit ();
      }
#endif
    });
#ifndef NDEBUG
    addShortcut (Qt::Key_I, [this] () {
      this->mainWidget.glWidget ().state ().scene ().printStatistics (false);
    });
    addShortcut (Qt::SHIFT + Qt::Key_I, [this] () {
      this->mainWidget.glWidget ().state ().scene ().printStatistics (true);
    });
#endif
    addShortcut (Qt::SHIFT + Qt::Key_C, [this] () {
      this->mainWidget.glWidget ()
                      .toolMoveCamera ()
                      .snap (this->mainWidget.glWidget ().state (), true);
    });
  }

  void setupMenuBar () {
    QMenuBar& menuBar  = *this->self->menuBar ();
    QMenu&    fileMenu = *menuBar.addMenu (QObject::tr ("&File"));
    QMenu&    editMenu = *menuBar.addMenu (QObject::tr ("&Edit"));
    QMenu&    viewMenu = *menuBar.addMenu (QObject::tr ("&View"));

    auto addAction = [] ( QMenu& menu, const QString& label, const QKeySequence& keySequence
                        , const std::function <void ()>& f ) -> QAction&
    {
      QAction* a = new QAction (label, nullptr);
      a->setShortcut (keySequence);
      menu.addAction (a);
      QObject::connect (a, &QAction::triggered, f);
      return *a;
    };

    addAction (fileMenu, QObject::tr ("&Quit"), QKeySequence::Quit, [] () {
      QMessageBox msgBox;
      msgBox.setWindowTitle     (QObject::tr ("Do you want to quit?"));
      msgBox.setText            (msgBox.windowTitle ());
      msgBox.setStandardButtons (QMessageBox::Cancel | QMessageBox::Ok);
      msgBox.setDefaultButton   (QMessageBox::Ok);
      msgBox.setEscapeButton    (QMessageBox::Cancel);

      if (msgBox.exec () == QMessageBox::Ok) {
        QApplication::quit ();
      }
    });
    addAction (editMenu, QObject::tr ("&Undo"), QKeySequence::Undo, [this] () {
      this->mainWidget.glWidget ().state ().undo ();
    });
    addAction (editMenu, QObject::tr ("&Redo"), QKeySequence::Redo, [this] () {
      this->mainWidget.glWidget ().state ().redo ();
    });
    addAction (viewMenu, QObject::tr ("&Snap camera"), Qt::Key_C, [this] () {
      this->mainWidget.glWidget ()
                      .toolMoveCamera ()
                      .snap (this->mainWidget.glWidget ().state (), false);
    });
    addAction (viewMenu, QObject::tr ("Reset &gaze point"), Qt::CTRL + Qt::Key_C, [this] () {
      this->mainWidget.glWidget ()
                      .toolMoveCamera ()
                      .resetGazePoint (this->mainWidget.glWidget ().state ());
    });
    addAction (viewMenu, QObject::tr ("Toggle &wireframe"), Qt::Key_W, [this] () {
      RenderMode mode = this->mainWidget.glWidget ().state ().scene ().commonRenderMode ();
      mode.renderWireframe (! mode.renderWireframe ());

      this->mainWidget.glWidget ().state ().scene ().commonRenderMode (mode);
      this->self->update ();
    });
    addAction (viewMenu, QObject::tr ("Toggle &shading"), Qt::SHIFT + Qt::Key_W, [this] () {
      RenderMode mode = this->mainWidget.glWidget ().state ().scene ().commonRenderMode ();

      if (mode.smoothShading ()) {
        mode.flatShading (true);
      }
      else if (mode.flatShading ()) {
        mode.smoothShading (true);
      }
      this->mainWidget.glWidget ().state ().scene ().commonRenderMode (mode);
      this->self->update ();
    });
  }

  void setupStatusBar () {
    this->self->setStatusBar (&this->statusBar);

    this->statusBar.setStyleSheet      ("QStatusBar::item { border: 0px solid black };");
    this->statusBar.setSizeGripEnabled (false);
    this->statusBar.addPermanentWidget (&this->messageLabel,1);
    this->statusBar.addPermanentWidget (&this->numFacesLabel);

    this->showDefaultToolTip ();
    this->showNumFaces       (0);
  }

  void showMessage (const QString& message) {
    this->messageLabel.setText (message);
  }

  void showToolTip (const ViewToolTip& tip) {
    this->showMessage (tip.toString ());
  }

  void showDefaultToolTip () {
    ViewToolTip tip;

    tip.add ( ViewToolTip::MouseEvent::Middle, QObject::tr ("Drag to rotate"));
    tip.add ( ViewToolTip::MouseEvent::Middle, ViewToolTip::Modifier::Shift
            , QObject::tr ("Drag to pan"));
    tip.add ( ViewToolTip::MouseEvent::Middle, ViewToolTip::Modifier::Ctrl
            , QObject::tr ("Gaze"));

    this->showToolTip (tip);
  }

  void showNumFaces (unsigned int n) {
    this->numFacesLabel.setText (QString::number (n).append (" faces"));
  }
};

DELEGATE2_BIG2_SELF (ViewMainWindow, Config&, Cache&)
GETTER    (ViewMainWidget&, ViewMainWindow, mainWidget)
DELEGATE1 (void           , ViewMainWindow, showMessage, const QString&)
DELEGATE1 (void           , ViewMainWindow, showToolTip, const ViewToolTip&)
DELEGATE  (void           , ViewMainWindow, showDefaultToolTip)
DELEGATE1 (void           , ViewMainWindow, showNumFaces, unsigned int)
