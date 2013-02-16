#include "MainWindow.h"
#include <QVBoxLayout>
#include <QFileDialog>

#include <Play/AbstractPlay.h>
#include <Act/AbstractAct.h>


MainWindow::MainWindow(scaena::QGLStage *stage) :
    _stage(stage),
    _menuBar(new QMenuBar())
{
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);

    // Build menu bar
    QMenu* menuFile  = _menuBar->addMenu("File");

    QAction* actionRestart = menuFile->addAction("Restart");
    QObject::connect(actionRestart, SIGNAL(triggered()),
                     _stage,        SLOT(restartPlay()));

    QAction* actionExit = menuFile->addAction("Exit");
    QObject::connect(actionExit, SIGNAL(triggered()),
                     this,        SLOT(close()));


    // Tweak layout
    _menuBar->setMinimumHeight(_menuBar->sizeHint().height());
    _menuBar->setMaximumHeight(_menuBar->sizeHint().height());
    layout->addWidget(_menuBar);
    layout->addWidget(_stage);
    setLayout(layout);

    resize(800, 600 + _menuBar->height());
}
