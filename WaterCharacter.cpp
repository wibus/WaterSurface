#include "WaterCharacter.h"

#include <vector>
#include <algorithm>

#include <GL3/gl3w.h>

#include <PropTeam/AbstractPropTeam.h>

#include <Stage/AbstractStage.h>
#include <Stage/Event/StageTime.h>
#include <Stage/Event/SynchronousKeyboard.h>
#include <Stage/Event/SynchronousMouse.h>

using namespace std;
using namespace cellar;
using namespace media;
using namespace scaena;

WaterCharacter::WaterCharacter(scaena::AbstractStage &stage) :
    AbstractCharacter(stage, "WaterCharacter"),
    _resolution(256, 256),
    _waterVelocityFbo(0),
    _waterHeightFbo(0),
    _waterHeightRead(0),
    _waterHeightWrite(1),
    _waterVelocityRead(2),
    _waterVelocityWrite(3),
    _groundRead(4),
    _latticeVao(0),
    _latticeBuffer(0),
    _latticeIndices(),
    _quadVao(0),
    _quadBuffer(0),
    _waterVelocityShader(),
    _waterHeightShader(),
    _renderShader(),
    _cameraMan( stage.camera() ),
    _fps()
{
    stage.camera().setTripod(
        Vec3f(0.7f, -0.2f, 0.8f),
        Vec3f(0.5f, 0.5f, 0.0f),
        Vec3f(0.0f, 0.0f, 1.0f));

    const Camera::Lens& lens = stage.camera().lens();
    stage.camera().setLens(
        lens.type(),
        lens.left()/3.0f,
        lens.right()/3.0f,
        lens.bottom()/3.0f,
        lens.top()/3.0f,
        0.1f,
        10.0f);
    stage.camera().registerObserver( *this );


    glGenFramebuffers(1, &_waterHeightFbo);
    glGenFramebuffers(1, &_waterVelocityFbo);

    glGenTextures(5, _textures);

    glGenVertexArrays(1, &_latticeVao);
    glGenBuffers(1, &_latticeBuffer);

    glGenVertexArrays(1, &_quadVao);
    glGenBuffers(1, &_quadBuffer);


    setupShaders();
    setupVaos();
}

WaterCharacter::~WaterCharacter()
{
    glDeleteTextures(5, _textures);
    glDeleteFramebuffers(1, &_waterHeightFbo);
    glDeleteFramebuffers(1, &_waterVelocityFbo);

    glDeleteVertexArrays(1, &_latticeVao);
    glDeleteBuffers(1, &_latticeBuffer);

    glDeleteVertexArrays(1, &_quadVao);
    glDeleteBuffers(1, &_quadBuffer);
}

void WaterCharacter::setupWater()
{
    vector<Vec4f> zero;
    for(int j=0; j<_resolution.y(); ++j)
        for(int i=0; i<_resolution.x(); ++i)
            zero.push_back( Vec4f(0.0f, 0.0f, 0.0f, 1.0f) );

    vector<Vec4f> waterHeight;    
    vector<Vec4f> groundHeight;
    for(int j=0; j<_resolution.y(); ++j)
    {
        for(int i=0; i<_resolution.x(); ++i)
        {
            float x = i/(float)_resolution.x();
            float y = j/(float)_resolution.y();

            float wz = 0.3 + (x < 0.25 ? cos(2*PI*x)*0.3 : 0.0);
            waterHeight.push_back(Vec4f(x, y, wz, 1.0f));

            float gz = (x > 0.7 && x < 0.8 && (y < 0.3 || y > 0.7)) ? 0.7 : 0.0;
            groundHeight.push_back(Vec4f(x, y, gz, 1.0f));
        }
    }

    // Height //
    glBindTexture(GL_TEXTURE_2D, _textures[_groundRead]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _resolution.x(), _resolution.y(), 0, GL_RGBA, GL_FLOAT, groundHeight.data());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, _textures[_waterHeightRead]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _resolution.x(), _resolution.y(), 0, GL_RGBA, GL_FLOAT, waterHeight.data());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, _textures[_waterHeightWrite]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _resolution.x(), _resolution.y(), 0, GL_RGBA, GL_FLOAT, waterHeight.data());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    // Velocity
    glBindTexture(GL_TEXTURE_2D, _textures[_waterVelocityRead]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _resolution.x(), _resolution.y(), 0, GL_RGBA, GL_FLOAT, zero.data());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, _textures[_waterVelocityWrite]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _resolution.x(), _resolution.y(), 0, GL_RGBA, GL_FLOAT, zero.data());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WaterCharacter::setupShaders()
{
    // Shaders
    GlInputsOutputs locations;
    locations.setInput(0, "position_att");


    _waterVelocityShader.setInAndOutLocations(locations);
    _waterVelocityShader.addShader(GL_VERTEX_SHADER, "resources/shaders/waterVelocity.vert");
    _waterVelocityShader.addShader(GL_FRAGMENT_SHADER, "resources/shaders/waterVelocity.frag");
    _waterVelocityShader.link();
    _waterVelocityShader.pushProgram();
    _waterVelocityShader.setInt("GroundHeightTex",  0);
    _waterVelocityShader.setInt("WaterHeightTex",   1);
    _waterVelocityShader.setInt("WaterVelocityTex", 2);
    _waterVelocityShader.popProgram();


    _waterHeightShader.setInAndOutLocations(locations);
    _waterHeightShader.addShader(GL_VERTEX_SHADER, "resources/shaders/waterHeight.vert");
    _waterHeightShader.addShader(GL_FRAGMENT_SHADER, "resources/shaders/waterHeight.frag");
    _waterHeightShader.link();
    _waterHeightShader.pushProgram();
    _waterHeightShader.setInt("GroundHeightTex",  0);
    _waterHeightShader.setInt("WaterHeightTex",   1);
    _waterHeightShader.setInt("WaterVelocityTex", 2);
    _waterHeightShader.popProgram();


    _renderShader.setInAndOutLocations(locations);
    _renderShader.addShader(GL_VERTEX_SHADER, "resources/shaders/waterRender.vert");
    _renderShader.addShader(GL_FRAGMENT_SHADER, "resources/shaders/waterRender.frag");
    _renderShader.link();
    _renderShader.pushProgram();
    _renderShader.setInt("HeightTex",        0);
    _renderShader.setInt("GroundHeightTex",  0);
    _renderShader.setInt("WaterHeightTex",   1);
    _renderShader.setInt("WaterVelocityTex", 2);
    _renderShader.setVec3f("Scale", Vec3f(1.0f, 1.0f, 1.0f));
    _renderShader.setVec4f("Color", Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
    _renderShader.popProgram();
}

void WaterCharacter::setupVaos()
{
    int position_loc =  0;

    // Lattice indices
    for(int j=0; j<_resolution.y()-1; ++j)
    {
        _latticeIndices.push_back(j * _resolution.x());
        for(int i=0; i<_resolution.x(); ++i)
        {
            _latticeIndices.push_back(j     * _resolution.x() + i);
            _latticeIndices.push_back((j+1) * _resolution.x() + i);
        }
        _latticeIndices.push_back((j+2) * _resolution.x()-1);
    }


    Vec3f scale(1.0f/_resolution.x(), 1.0/_resolution.y(), 1.0f);
    vector<Vec3f> lattice;
    for(int j=0; j<=_resolution.y(); ++j)
        for(int i=0; i<_resolution.x(); ++i)
            lattice.push_back(mult(Vec3f(i, j, 0.5f), scale));


    // lattice
    glBindVertexArray( _latticeVao );
    glBindBuffer(GL_ARRAY_BUFFER, _latticeBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lattice[0]) * lattice.size(), lattice.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 3, GL_FLOAT, 0, 0, 0);


    // Quad Vao
    Vec3f positions[4] = {
        Vec3f(0.0f, 0.0f, 0.0f),
        Vec3f(1.0f, 0.0f, 0.0f),
        Vec3f(1.0f, 1.0f, 0.0f),
        Vec3f(0.0f, 1.0f, 0.0f)
    };
    glBindVertexArray( _quadVao );
    glBindBuffer(GL_ARRAY_BUFFER, _quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 3, GL_FLOAT, 0, 0, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WaterCharacter::enterStage()
{
    stage().camera().refresh();
    _fps = stage().propTeam().createTextHud();
    _fps->setHeight(20);

    setupWater();
}

void WaterCharacter::beginStep(const StageTime &)
{
}

void WaterCharacter::endStep(const StageTime &time)
{
    float velocity  = 0.5f * time.elapsedTime();
    float turnSpeed = 0.004;

    if(stage().synchronousKeyboard().isAsciiPressed('W'))
    {
        _cameraMan.forward(velocity);
    }
    if(stage().synchronousKeyboard().isAsciiPressed('S'))
    {
        _cameraMan.forward(-velocity);
    }
    if(stage().synchronousKeyboard().isAsciiPressed('A'))
    {
        _cameraMan.sideward(-velocity);
    }
    if(stage().synchronousKeyboard().isAsciiPressed('D'))
    {
        _cameraMan.sideward(velocity);
    }

    if(stage().synchronousMouse().displacement() != Vec2i(0, 0) &&
       stage().synchronousMouse().buttonIsPressed(EMouseButton::LEFT))
    {
        _cameraMan.turnHorizontaly(stage().synchronousMouse().displacement().x() * turnSpeed);
        _cameraMan.turnVerticaly(  stage().synchronousMouse().displacement().y() * turnSpeed);
    }
}

void WaterCharacter::draw(const StageTime &time)
{
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, _resolution.x(), _resolution.y());
    glBindVertexArray(_quadVao);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _textures[_waterVelocityRead]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _textures[_waterHeightRead]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _textures[_groundRead]);


    // Velocity
    _waterVelocityShader.pushProgram();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _waterVelocityFbo);    
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _textures[_waterVelocityWrite], 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    _waterVelocityShader.popProgram();

    swap(_waterVelocityRead, _waterVelocityWrite);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _textures[_waterVelocityRead]);
    glActiveTexture(GL_TEXTURE0);


    // Height
    _waterHeightShader.pushProgram();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _waterHeightFbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _textures[_waterHeightWrite], 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    _waterHeightShader.popProgram();

    swap(_waterHeightRead, _waterHeightWrite);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _textures[_waterHeightRead]);
    glActiveTexture(GL_TEXTURE0);


    // Normal mode
    glViewport(0, 0, stage().width(), stage().height());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray( 0 );

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    _renderShader.pushProgram();    
    glBindVertexArray(_latticeVao);

    _renderShader.setVec4f("Color", Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
    _renderShader.setInt("HeightTex", 0);
    glDrawElements(GL_TRIANGLE_STRIP, _latticeIndices.size(),
                   GL_UNSIGNED_INT,   _latticeIndices.data());

    _renderShader.setVec4f("Color", Vec4f(0.2f, 0.2f, 1.0f, 0.9f));
    _renderShader.setInt("HeightTex", 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawElements(GL_TRIANGLE_STRIP, _latticeIndices.size(),
                   GL_UNSIGNED_INT,   _latticeIndices.data());
    glDisable(GL_BLEND);

    _renderShader.popProgram();


    _fps->setText(string("FPS : ") + toString(1.0f / time.elapsedTime()));
    _fps->setHandlePosition(Vec2f(10, 10));
}

void WaterCharacter::exitStage()
{
}

void WaterCharacter::notify(CameraMsg &msg)
{
    _renderShader.pushProgram();

    if(msg.change == CameraMsg::EChange::PROJECTION)
        _renderShader.setMat4f("Projection", msg.camera.projectionMatrix());
    else
        _renderShader.setMat4f("View", msg.camera.viewMatrix());

    _renderShader.popProgram();
}
