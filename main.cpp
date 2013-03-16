#include <exception>
#include <iostream>
using namespace std;

#include <ScaenaApplication/Application.h>
#include <ScaenaApplication/GlMainWindow.h>
#include <Stage/QGLStage.h>
using namespace scaena;

#include "WaterPlay.h"


int main(int argc, char** argv) try
{
    getApplication().init(argc, argv);
    getApplication().setPlay(std::shared_ptr<AbstractPlay>(new WaterPlay()));

    QGLStage* stage = new QGLStage();
    getApplication().addCustomStage(stage);
    getApplication().chooseStage(stage->id());

    GlMainWindow window(stage);
    window.setGlWindowSpace(800, 600);
    window.centerOnScreen();
    window.show();

    stage->setDrawSynch( true );
    stage->setUpdateInterval( 16 );

    return getApplication().execute();
}
catch(exception& e)
{
    cerr << "Exception caught : " << e.what() << endl;
}
catch(...)
{
    cerr << "Exception passed threw.." << endl;
    throw;
}
