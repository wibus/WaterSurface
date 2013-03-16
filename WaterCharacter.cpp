#include "WaterCharacter.h"
using namespace std;
using namespace cellar;
using namespace scaena;

#include <vector>
#include <algorithm>

#include <GL/glew.h>

#include <Graphics/Image.h>

#include <Stage/AbstractStage.h>
#include <Stage/Event/StageTime.h>
#include <Stage/Event/SynchronousKeyboard.h>
#include <Stage/Event/SynchronousMouse.h>


WaterCharacter::WaterCharacter(scaena::AbstractStage &stage) :
    AbstractCharacter(stage, "WaterCharacter"),
    _size(256, 256),
    _latticeIndices(),
    _waterVelocityFbo(0),
    _waterVelocityTex(0),
    _waterHeightFbo(0),
    _waterHeightTex(0),    
    _waterLatticeVao(0),
    _waterLatticeBuffer(0),
    _groundLatticeVao(0),
    _groundLatticeBuffer(0),
    _quadVao(0),
    _quadBuffer(0),
    _waterVelocityShader(),
    _waterHeightShader(),
    _renderShader(),
    _cameraMan( stage.camera() ),
    _fps()
{
    stage.camera().setTripod(Vec3f(0.7f, -0.2f, 0.8f),
                             Vec3f(0.5f, 0.5f, 0.0f),
                             Vec3f(0.0f, 0.0f, 1.0f));
    const Camera::Lens& lens = stage.camera().lens();
    stage.camera().setLens(lens.type(), lens.left(), lens.right(), lens.bottom(), lens.top(), 0.1f, 10.0f);
    stage.camera().registerObserver( *this );

    setupWater();
    setupShaders();
    setupVaos();
}

WaterCharacter::~WaterCharacter()
{
    glDeleteFramebuffers(1, &_waterVelocityFbo);
    glDeleteTextures(1, &_waterVelocityTex);
    glDeleteFramebuffers(1, &_waterHeightFbo);
    glDeleteTextures(1, &_waterHeightTex);

    glDeleteVertexArrays(1, &_waterLatticeVao);
    glDeleteBuffers(1, &_waterLatticeBuffer);

    glDeleteVertexArrays(1, &_groundLatticeVao);
    glDeleteBuffers(1, &_groundLatticeBuffer);

    glDeleteVertexArrays(1, &_quadVao);
    glDeleteBuffers(1, &_quadBuffer);
}

void WaterCharacter::setupWater()
{
    vector<Vec3ub> zero;
    for(int j=0; j<_size.y(); ++j)
        for(int i=0; i<_size.x(); ++i)
            zero.push_back( Vec3ub(0, 0 ,0) );

    vector<Vec3ub> waterInit;
    for(int j=0; j<_size.y(); ++j)
    {
        for(int i=0; i<_size.x(); ++i)
        {
            unsigned char x = (i*255)/_size.x();
            unsigned char y = (j*255)/_size.y();
            unsigned char z = 128;
            if(x == 128 && y == 128) z = 220;
            waterInit.push_back(Vec3ub(x, y, z));
        }
    }

    // Height //
    glGenTextures(1, &_waterHeightTex);
    glBindTexture(GL_TEXTURE_2D, _waterHeightTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _size.x(), _size.y(), 0, GL_RGB, GL_UNSIGNED_BYTE, waterInit.data());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // FrameBuffer
    glGenFramebuffers(1, &_waterHeightFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _waterHeightFbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _waterHeightTex, 0);


    // Velocity
    glGenTextures(1, &_waterVelocityTex);
    glBindTexture(GL_TEXTURE_2D, _waterVelocityTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _size.x(), _size.y(), 0, GL_RGB, GL_UNSIGNED_BYTE, zero.data());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // FrameBuffer
    glGenFramebuffers(1, &_waterVelocityFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _waterVelocityFbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _waterVelocityTex, 0);


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WaterCharacter::setupShaders()
{
    // Shaders
    cellar::GlInputsOutputs locations;
    locations.setInput(0, "position_att");


    _waterVelocityShader.setInAndOutLocations(locations);
    _waterVelocityShader.addShader(GL_VERTEX_SHADER, "resources/shaders/waterVelocity.vert");
    _waterVelocityShader.addShader(GL_FRAGMENT_SHADER, "resources/shaders/waterVelocity.frag");
    _waterVelocityShader.pushProgram();
    _waterVelocityShader.setInt("HeightTex",       0);
    _waterVelocityShader.setInt("VelocityTex",     1);
    _waterVelocityShader.setVec2f("Size", _size);
    _waterVelocityShader.popProgram();


    _waterHeightShader.setInAndOutLocations(locations);
    _waterHeightShader.addShader(GL_VERTEX_SHADER, "resources/shaders/waterHeight.vert");
    _waterHeightShader.addShader(GL_FRAGMENT_SHADER, "resources/shaders/waterHeight.frag");
    _waterHeightShader.pushProgram();
    _waterHeightShader.setInt("HeightTex",       0);
    _waterHeightShader.setInt("VelocityTex",     1);
    _waterHeightShader.setVec2f("Size", _size);
    _waterHeightShader.popProgram();


    _renderShader.setInAndOutLocations(locations);
    _renderShader.addShader(GL_VERTEX_SHADER, "resources/shaders/render.vert");
    _renderShader.addShader(GL_FRAGMENT_SHADER, "resources/shaders/render.frag");
    _renderShader.pushProgram();
    _renderShader.setVec2f("Size", _size);
    _renderShader.setFloat("ZDepth", 255.0f);
    _renderShader.setVec4f("Color", Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
    _renderShader.setInt("DiffuseTex", 1);
    _renderShader.popProgram();
}

void WaterCharacter::setupVaos()
{
    int position_loc =  0;

    // Lattice indices
    for(int j=0; j<_size.y()-1; ++j)
    {
        _latticeIndices.push_back(j * _size.x());
        for(int i=0; i<_size.x(); ++i)
        {
            _latticeIndices.push_back(j     * _size.x() + i);
            _latticeIndices.push_back((j+1) * _size.x() + i);
        }
        _latticeIndices.push_back((j+2) * _size.x()-1);
    }

    vector<Vec3ub> lattice;
    for(int j=0; j<=_size.y(); ++j)
        for(int i=0; i<_size.x(); ++i)
            lattice.push_back(Vec3ub(i, j, 0));


    // Water lattice
    glGenVertexArrays(1, &_waterLatticeVao);
    glBindVertexArray( _waterLatticeVao );
    glGenBuffers(1, &_waterLatticeBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _waterLatticeBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lattice[0]) * lattice.size(), lattice.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 3, GL_UNSIGNED_BYTE, 0, 0, 0);


    // Ground lattice
    for(unsigned int i=0; i<lattice.size(); ++i)
        lattice[i].setZ( 0 );

    glGenVertexArrays(1, &_groundLatticeVao);
    glBindVertexArray( _groundLatticeVao );
    glGenBuffers(1, &_groundLatticeBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _groundLatticeBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lattice[0]) * lattice.size(), lattice.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 3, GL_UNSIGNED_BYTE, 0, 0, 0);



    // Quad Vao
    Vec4f positions[4] = {
        Vec4f(-1, -1, 0, 1),
        Vec4f( 1, -1, 0, 1),
        Vec4f( 1,  1, 0, 1),
        Vec4f(-1,  1, 0, 1)
    };
    glGenVertexArrays(1, &_quadVao);
    glBindVertexArray( _quadVao );
    glGenBuffers(1, &_quadBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 4, GL_FLOAT, 0, 0, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WaterCharacter::enterStage()
{
    stage().camera().refresh();
}

void WaterCharacter::beginStep(const StageTime &time)
{
    // Copy FBO -> PBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _waterHeightFbo);
    glViewport(0, 0, _size.x(), _size.y());

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _waterLatticeBuffer);
    glReadPixels(0, 0, _size.x(), _size.y(), GL_RGB, GL_UNSIGNED_BYTE, 0x0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Update Water Height
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, _size.x(), _size.y());

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _waterVelocityTex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _waterHeightTex);
    glBindVertexArray(_quadVao);

    _waterVelocityShader.pushProgram();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _waterVelocityFbo);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    _waterVelocityShader.popProgram();

    _waterHeightShader.pushProgram();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _waterHeightFbo);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    _waterHeightShader.popProgram();


    // Normal mode
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray( 0 );
    glViewport(0, 0, stage().width(), stage().height());
}

void WaterCharacter::endStep(const StageTime &time)
{
    float velocity  = 0.5f * time.elapsedTime();
    float turnSpeed = 0.004;

    if(stage().synchronousKeyboard().isAsciiPressed('w'))
    {
        _cameraMan.forward(velocity);
    }
    if(stage().synchronousKeyboard().isAsciiPressed('s'))
    {
        _cameraMan.forward(-velocity);
    }
    if(stage().synchronousKeyboard().isAsciiPressed('a'))
    {
        _cameraMan.sideward(-velocity);
    }
    if(stage().synchronousKeyboard().isAsciiPressed('d'))
    {
        _cameraMan.sideward(velocity);
    }

    if(stage().synchronousMouse().displacement() != Vec2i(0, 0) &&
       stage().synchronousMouse().buttonIsPressed(Mouse::LEFT))
    {
        _cameraMan.turnHorizontaly(stage().synchronousMouse().displacement().x() * turnSpeed);
        _cameraMan.turnVerticaly(  stage().synchronousMouse().displacement().y() * turnSpeed);
    }
}

void WaterCharacter::draw(const StageTime &time)
{
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    _renderShader.pushProgram();

   _renderShader.setVec4f("Color", Vec4f(0.25, 0.18, 0.04, 1.0f));
    glDisable(GL_BLEND);
    glBindVertexArray(_groundLatticeVao);
    glDrawElements(GL_TRIANGLE_STRIP, _latticeIndices.size(),
                   GL_UNSIGNED_INT,   _latticeIndices.data());

    _renderShader.setVec4f("Color", Vec4f(0.15f, 0.2f, 0.8f, 0.5f));
    glEnable(GL_BLEND);
    glBindVertexArray(_waterLatticeVao);    
    glDrawElements(GL_TRIANGLE_STRIP, _latticeIndices.size(),
                   GL_UNSIGNED_INT,   _latticeIndices.data());

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

    if(msg.change == CameraMsg::PROJECTION)
        _renderShader.setMat4f("Projection", msg.camera.projectionMatrix());
    else
        _renderShader.setMat4f("View", msg.camera.viewMatrix());

    _renderShader.popProgram();
}
