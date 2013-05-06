#include "CpuWaterSim.h"

#include <GL3/gl3w.h>

#include <Algorithm/Noise.h>
#include <DataStructure/Vector.h>
#include <GL/GlToolkit.h>
#include <Image/ImageBank.h>

#include <PropTeam/AbstractPropTeam.h>

#include <Stage/AbstractStage.h>
#include <Stage/Event/StageTime.h>
#include <Stage/Event/SynchronousKeyboard.h>
#include <Stage/Event/SynchronousMouse.h>
#include <Stage/Event/KeyboardEvent.h>


using namespace std;
using namespace cellar;
using namespace media;
using namespace scaena;


CpuWaterSim::CpuWaterSim(scaena::AbstractStage &stage) :
    AbstractCharacter(stage, "CpuWaterSim"),
    _STRETCHNESS(0.35f),
    _LOSSYNESS(_STRETCHNESS/1000.0f),
    _WIDTH(128),
    _HEIGHT(128),
    _ARRAY_SIZE((_WIDTH)*(_HEIGHT)),
    _NEIGHBORS_RADIUS(2),
    _vertices(),
    _latticeIndices(),
    _groundTex(0),
    _groundVao(),
    _groundMaterial(),
    _wallsTex(0),
    _wallsVao(),
    _wallsMaterial(),
    _waterTex(0),
    _waterVao(),
    _waterMaterial(),
    _pointLight(),
    _cameraMan(stage.camera()),
    _renderShader(),
    _fps()
{
    Camera::Lens lens = stage.camera().lens();
    stage.camera().setLens(lens.type(), lens.left() / 10.0f,      lens.right() / 10.0f,
                                        lens.bottom() / 10.0f,    lens.top() / 10.0f,
                                        lens.nearPlane() / 10.0f, lens.farPlane() / 40.0f);
    stage.camera().setTripod(Vec3f(2.3f, 1.6f, 1.5f),
                             Vec3f(0.5f, 0.5f, 0.4f),
                             Vec3f(0.0f, 0.0f, 1.0f));
    stage.camera().registerObserver( *this );

    _fps = stage.propTeam().createTextHud();
    _fps->setHandlePosition(Vec2f(10, 10));

    setupLight();
    setupTextures();
    setupShader();
    setupLattice();
    setupGround();
    setupWalls();
    setupWater();
    setupVerticesAndNeighbors();

    /*
    _camcorder.setFileName("VideoTest.avi");
    _camcorder.setFrameRate(25);
    _camcorder.setBitRate(2500000);
    _camcorder.setFrameWidth(stage.width());
    _camcorder.setFrameHeight(stage.height());
    _camcorder.setup();
    */
}

CpuWaterSim::~CpuWaterSim()
{
    glDeleteTextures(1, &_groundTex);
    glDeleteTextures(1, &_wallsTex);
    glDeleteTextures(1, &_waterTex);

    //_camcorder.finalise();
}

void CpuWaterSim::enterStage()
{
    cout << "New simulation" << endl;

    stage().camera().refresh();

    for(int j=0; j<_HEIGHT; ++j)
    {
        for(int i=0; i<_WIDTH; ++i)
        {
            float x, y;
            realPosition(i, j, x, y);
            int currIndex = index(i, j);

            _waterVelocities[currIndex] = waterInitVelocity(x, y);
            _waterPositions[currIndex].setZ(maxVal(waterInitHeight(i, j),
                                                   groundInitHeight(i, j)));
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, _waterVao.bufferId("position"));
    glBufferData(GL_ARRAY_BUFFER,  sizeof(_waterPositions[0]) * _waterPositions.size(),
                 _waterPositions.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, _waterVao.bufferId("normal"));
    glBufferData(GL_ARRAY_BUFFER,  sizeof(_waterNormals[0]) * _waterNormals.size(),
                 _waterNormals.data(), GL_STREAM_DRAW);
}

void CpuWaterSim::setupVerticesAndNeighbors()
{
    for(int j=0; j<_HEIGHT; ++j)
    {
        for(int i=0; i<_WIDTH; ++i)
         {
             Vertex vertex;
             vertex.node.index = index(i, j);
             vertex.node.isExchangePermitted = true;        // Overall exchange
             vertex.node.isSurpervisingExchange = true;     // Must be ;)
             vertex.node.currContribution = 0.0f;
             vertex.node.baseContribution = 1.0f;           // Velocity ratio

             float totalContribution = 0.0f;

             for(int nj = -_NEIGHBORS_RADIUS; nj <= _NEIGHBORS_RADIUS; ++nj)
             {
                 for(int ni = -_NEIGHBORS_RADIUS; ni <= _NEIGHBORS_RADIUS; ++ni)
                 {
                     // Off bounds neighbors
                     if( !isInBounds(i+ni, j+nj) )
                         continue;

                     // Current node
                     if(ni == 0 && nj == 0)
                         continue;

                     // Is near enough
                     if( !isNeighbor(ni, nj) )
                         continue;


                     Node neighbor;
                     neighbor.index = index(i+ni, j+nj);
                     neighbor.isExchangePermitted = true;
                     neighbor.baseContribution = baseContribution(ni, nj);
                     neighbor.currContribution = 0.0f;

                     // Below nodes
                     if(nj < 0 || (nj == 0 && ni < 0))
                     {
                         neighbor.isSurpervisingExchange = false;
                     }
                     // Above nodes
                     else
                     {
                         neighbor.isSurpervisingExchange = false; //true
                     }

                     vertex.neighbors.push_back( neighbor );

                     totalContribution += neighbor.baseContribution;
                 }
             }

             for(size_t n=0; n<vertex.neighbors.size(); ++n)
                 vertex.neighbors[n].baseContribution /= totalContribution;

             _vertices.push_back( vertex );
         }
    }
}

void CpuWaterSim::beginStep(const StageTime &time)
{
    // Update velocities
    for(size_t v=0; v<_vertices.size(); ++v)
    {
        Vertex& vertex = _vertices[v];
        Node& node = vertex.node;

        float dzMean = 0.0f;

        for(size_t n=0; n<vertex.neighbors.size(); ++n)
        {
            Node& neighbor = vertex.neighbors[n];

            float dz = deltaHeight(node.index, neighbor.index);
            dz = max(dz, -overFloor(node.index));
            dz = min(dz, overFloor(neighbor.index));
            neighbor.currContribution = dz * neighbor.baseContribution;
            dzMean += neighbor.currContribution;
        }

        float velocity = _waterVelocities[node.index];
        float acc = (dzMean * _STRETCHNESS) - (velocity * _LOSSYNESS);
        node.velocity = velocity + acc;
        // Real velocity will be computed in next stage
        _waterVelocities[node.index] = 0.0f;

        float totContrib = 0.0f;
        for(size_t n=0; n<vertex.neighbors.size(); ++n)
        {
            Node& neighbor = vertex.neighbors[n];
            if(neighbor.isExchangePermitted =
               isExchangePermitted(node.index, neighbor.index))
                totContrib += neighbor.baseContribution;
        }

        if(totContrib == 0.0f)
        {
            node.isExchangePermitted = false;
        }
        else
        {
            node.isExchangePermitted = true;
            for(size_t n=0; n<vertex.neighbors.size(); ++n)
            {
                Node& neighbor = vertex.neighbors[n];
                neighbor.currContribution = neighbor.baseContribution / totContrib;
            }
        }
    }

    // Update positions
    for(size_t v=0; v<_vertices.size(); ++v)
    {
        Vertex& vertex = _vertices[v];
        Node& node = vertex.node;

        if(!node.isExchangePermitted) continue;

        float expectedWaterMoved = node.velocity;
        float maxWaterMoved = max(expectedWaterMoved, -overFloor(node.index));

        for(size_t n=0; n<vertex.neighbors.size(); ++n)
        {
            Node& neighbor = vertex.neighbors[n];

            if(!neighbor.isExchangePermitted) continue;

            float waterMoved = min(
                maxWaterMoved * neighbor.currContribution,
                overFloor(neighbor.index)
            );

            _waterPositions[node.index][2] += waterMoved;
            _waterVelocities[node.index] += waterMoved;
        }
    }

    // Update normals
    for(int j=0; j<_HEIGHT; ++j)
    {
        for(int i=0; i<_WIDTH; ++i)
        {
            float dx =
                _waterPositions[index(clamp(i+1, 0, _WIDTH-1), clamp(j, 0, _HEIGHT-1))].z() -
                _waterPositions[index(clamp(i-1, 0, _WIDTH-1), clamp(j, 0, _HEIGHT-1))].z();
            float dy =
                _waterPositions[index(clamp(i, 0, _WIDTH-1), clamp(j+1, 0, _HEIGHT-1))].z() -
                _waterPositions[index(clamp(i, 0, _WIDTH-1), clamp(j-1, 0, _HEIGHT-1))].z();

            int currIndex = index(i, j);
            _waterNormals[currIndex].setX( -dx );
            _waterNormals[currIndex].setY( -dy );
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, _waterVao.bufferId("position"));
    glBufferData(GL_ARRAY_BUFFER,  sizeof(_waterPositions[0]) * _waterPositions.size(),
                 _waterPositions.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, _waterVao.bufferId("normal"));
    glBufferData(GL_ARRAY_BUFFER,  sizeof(_waterNormals[0]) * _waterNormals.size(),
                 _waterNormals.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CpuWaterSim::endStep(const StageTime &time)
{
    Vec3f from = stage().camera().tripod().from();
    Vec3f to = stage().camera().tripod().to();
    Vec2f radius = rotate(Vec2f(from - to), PI/200.0f);
    Vec3f newPos(to.x() + radius.x(), to.y() + radius.y(), from.z());
    stage().camera().setTripod(newPos, to, Vec3f(0.0, 0.0, 1.0));


    float velocity  = 1.0f * time.elapsedTime();
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

void CpuWaterSim::draw(const StageTime &time)
{
    _renderShader.pushProgram();

    _renderShader.setVec4f("material.diffuse",   _groundMaterial.diffuse);
    _renderShader.setVec4f("material.specular",  _groundMaterial.specular);
    _renderShader.setFloat("material.shininess", _groundMaterial.shininess);
    _renderShader.setFloat("material.fresnel",   _groundMaterial.fresnel);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, _groundTex);
    _groundVao.bind();
    glDrawElements(GL_TRIANGLE_STRIP, _latticeIndices.size(),
                   GL_UNSIGNED_INT,   _latticeIndices.data());

    _renderShader.setVec4f("material.diffuse",   _wallsMaterial.diffuse);
    _renderShader.setVec4f("material.specular",  _wallsMaterial.specular);
    _renderShader.setFloat("material.shininess", _wallsMaterial.shininess);
    _renderShader.setFloat("material.fresnel",   _wallsMaterial.fresnel);
    glBindTexture(GL_TEXTURE_2D, _wallsTex);
    _wallsVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 24);

    _renderShader.setVec4f("material.diffuse",   _waterMaterial.diffuse);
    _renderShader.setVec4f("material.specular",  _waterMaterial.specular);
    _renderShader.setFloat("material.shininess", _waterMaterial.shininess);
    _renderShader.setFloat("material.fresnel",   _waterMaterial.fresnel);
    //glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, _waterTex);
    _waterVao.bind();
    glDrawElements(GL_TRIANGLE_STRIP, _latticeIndices.size(),
                   GL_UNSIGNED_INT,   _latticeIndices.data());

    _renderShader.popProgram();

    _fps->setText("FPS: " + toString(1.0 / time.elapsedTime()));

    //_camcorder.recordFrame();
}

void CpuWaterSim::exitStage()
{
}

bool CpuWaterSim::keyPressEvent(const scaena::KeyboardEvent& event)
{
    if(event.getAscii() == 'P')
    {
        for(unsigned int i=0; i<_waterPositions.size(); ++i)
        {
            if(i%5 == 0)
                cout << endl;
            cout << _waterPositions[i] << '\t';
        }

        return true;
    }

    return false;
}

void CpuWaterSim::notify(CameraMsg &msg)
{
    _renderShader.pushProgram();

    if(msg.change == CameraMsg::PROJECTION)
        _renderShader.setMat4f("Projection", msg.camera.projectionMatrix());
    else
    {
        _renderShader.setMat4f("View",   msg.camera.viewMatrix());
        _renderShader.setMat3f("Normal", submat(msg.camera.viewMatrix(), 3, 3));
        _renderShader.setVec4f("light.position", msg.camera.viewMatrix() * _pointLight.position);
    }

    _renderShader.popProgram();
}

void CpuWaterSim::setupLattice()
{
    for(int j=0; j<_HEIGHT-1; ++j)
    {
        _latticeIndices.push_back(j * _WIDTH);
        for(int i=0; i<_WIDTH; ++i)
        {
            _latticeIndices.push_back(j     * _WIDTH + i);
            _latticeIndices.push_back((j+1) * _WIDTH + i);
        }
        _latticeIndices.push_back((j+2) * _WIDTH-1);
    }
}

void CpuWaterSim::setupGround()
{
    GlVbo3Df positionBuff;
    positionBuff.attribLocation = _renderShader.getAttributeLocation("position");
    positionBuff.dataArray.resize(_ARRAY_SIZE);

    GlVbo3Df normalBuff;
    normalBuff.attribLocation = _renderShader.getAttributeLocation("normal");
    normalBuff.dataArray.resize(_ARRAY_SIZE);

    GlVbo2Df texCoordBuff;
    texCoordBuff.attribLocation = _renderShader.getAttributeLocation("texCoord");
    texCoordBuff.dataArray.resize(_ARRAY_SIZE);

    for(int j=0; j<_HEIGHT; ++j)
    {
        for(int i=0; i<_WIDTH; ++i)
        {
            float x, y;
            realPosition(i, j, x, y);
            int currIndex = index(i, j);

            positionBuff.dataArray[currIndex](x, y, groundInitHeight(i, j));
            normalBuff  .dataArray[currIndex](0.0f, 0.0f, 2.0f / _WIDTH);
            texCoordBuff.dataArray[currIndex](x, y);
        }
    }

    _groundVao.createBuffer("position", positionBuff);
    _groundVao.createBuffer("normal",   normalBuff);
    _groundVao.createBuffer("texCoord", texCoordBuff);
    _groundPositions = positionBuff.dataArray;

    _groundMaterial.diffuse(1.0f, 1.0f, 1.0f, 1.0f);
    _groundMaterial.specular(0.0f, 0.0f, 0.0f, 0.0f);
    _groundMaterial.shininess = 0.0f;
    _groundMaterial.fresnel = 0.05f;
}

void CpuWaterSim::setupWalls()
{
    const int NB_FACES = 6;
    const int NB_VERT_FACE = 6;
    const int NB_VERTICIES = NB_FACES * NB_VERT_FACE;

    GlVbo3Df positionBuff;
    positionBuff.attribLocation = _renderShader.getAttributeLocation("position");
    positionBuff.dataArray.resize(NB_VERTICIES);

    GlVbo3Df normalBuff;
    normalBuff.attribLocation = _renderShader.getAttributeLocation("normal");
    normalBuff.dataArray.resize(NB_VERTICIES);

    GlVbo2Df texCoordBuff;
    texCoordBuff.attribLocation = _renderShader.getAttributeLocation("texCoord");
    texCoordBuff.dataArray.resize(NB_VERTICIES);

    Vec3f faceU[NB_FACES] = {
        Vec3f(0.5f, 0.0f, 0.0f), // Down
        Vec3f(-.5f, 0.0f, 0.0f), // South
        Vec3f(0.0f, -.5f, 0.0f), // East
        Vec3f(0.5f, 0.0f, 0.0f), // North
        Vec3f(0.0f, 0.5f, 0.0f), // West
        Vec3f(0.5f, 0.0f, 0.0f)  // Up
    };
    Vec3f faceV[NB_FACES] = {
        Vec3f(0.0f, 0.5f, 0.0f),
        Vec3f(0.0f, 0.0f, 0.5f),
        Vec3f(0.0f, 0.0f, 0.5f),
        Vec3f(0.0f, 0.0f, 0.5f),
        Vec3f(0.0f, 0.0f, 0.5f),
        Vec3f(0.0f, -.5f, 0.0f)
    };
    Vec3f faceCenter[NB_FACES] = {
        Vec3f(0.5f, 0.5f, 0.0f),
        Vec3f(0.5f, 0.0f, 0.5f),
        Vec3f(1.0f, 0.5f, 0.5f),
        Vec3f(0.5f, 1.0f, 0.5f),
        Vec3f(0.0f, 0.5f, 0.5f),
        Vec3f(0.5f, 0.5f, 1.0f)
    };
    Vec3f faceNormal[NB_FACES] = {
        Vec3f(0.0f, 0.0f, 1.0f),
        Vec3f(0.0f, 1.0f, 0.0f),
        Vec3f(-1.f, 0.0f, 0.0f),
        Vec3f(0.0f, -1.f, 0.0f),
        Vec3f(1.0f, 0.0f, 0.0f),
        Vec3f(0.0f, 0.0f, -1.f)
    };

    for(int f=0; f<NB_FACES; ++f)
    {
        positionBuff.dataArray[f*NB_VERT_FACE + 0] = faceCenter[f] - faceU[f] - faceV[f];
        positionBuff.dataArray[f*NB_VERT_FACE + 1] = faceCenter[f] + faceU[f] - faceV[f];
        positionBuff.dataArray[f*NB_VERT_FACE + 2] = faceCenter[f] + faceU[f] + faceV[f];
        positionBuff.dataArray[f*NB_VERT_FACE + 3] = faceCenter[f] + faceU[f] + faceV[f];
        positionBuff.dataArray[f*NB_VERT_FACE + 4] = faceCenter[f] - faceU[f] + faceV[f];
        positionBuff.dataArray[f*NB_VERT_FACE + 5] = faceCenter[f] - faceU[f] - faceV[f];

        normalBuff.dataArray[f*NB_VERT_FACE + 0] = faceNormal[f];
        normalBuff.dataArray[f*NB_VERT_FACE + 1] = faceNormal[f];
        normalBuff.dataArray[f*NB_VERT_FACE + 2] = faceNormal[f];
        normalBuff.dataArray[f*NB_VERT_FACE + 3] = faceNormal[f];
        normalBuff.dataArray[f*NB_VERT_FACE + 4] = faceNormal[f];
        normalBuff.dataArray[f*NB_VERT_FACE + 5] = faceNormal[f];

        texCoordBuff.dataArray[f*NB_VERT_FACE + 0] = Vec2f(0.0f, 0.0f);
        texCoordBuff.dataArray[f*NB_VERT_FACE + 1] = Vec2f(4.0f, 0.0f);
        texCoordBuff.dataArray[f*NB_VERT_FACE + 2] = Vec2f(4.0f, 1.0f);
        texCoordBuff.dataArray[f*NB_VERT_FACE + 3] = Vec2f(4.0f, 1.0f);
        texCoordBuff.dataArray[f*NB_VERT_FACE + 4] = Vec2f(0.0f, 1.0f);
        texCoordBuff.dataArray[f*NB_VERT_FACE + 5] = Vec2f(0.0f, 0.0f);
    }


    _wallsVao.createBuffer("position", positionBuff);
    _wallsVao.createBuffer("normal",   normalBuff);
    _wallsVao.createBuffer("texCoord", texCoordBuff);

    _wallsMaterial.diffuse(1.0f, 1.0f, 1.0f, 1.0f);
    _wallsMaterial.specular(0.4f, 0.4f, 0.4f, 1.0f);
    _wallsMaterial.shininess = 30.0f;
    _wallsMaterial.fresnel = 0.05f;
}

void CpuWaterSim::setupWater()
{
    _waterVelocities.clear();
    _waterVelocities.resize(_ARRAY_SIZE);

    GlVbo3Df positionBuff;
    positionBuff.attribLocation = _renderShader.getAttributeLocation("position");
    positionBuff.dataArray.resize(_ARRAY_SIZE);

    GlVbo3Df normalBuff;
    normalBuff.attribLocation = _renderShader.getAttributeLocation("normal");
    normalBuff.dataArray.resize(_ARRAY_SIZE);

    GlVbo2Df texCoordBuff;
    texCoordBuff.attribLocation = _renderShader.getAttributeLocation("texCoord");
    texCoordBuff.dataArray.resize(_ARRAY_SIZE);

    for(int j=0; j<_HEIGHT; ++j)
    {
        for(int i=0; i<_WIDTH; ++i)
        {
            float x, y;
            realPosition(i, j, x, y);
            int currIndex = index(i, j);

            _waterVelocities[currIndex] =  waterInitVelocity(x, y);
            normalBuff  .dataArray[currIndex](0.0f, 0.0f, 2.0f / _WIDTH);
            texCoordBuff.dataArray[currIndex](x, y);
            positionBuff.dataArray[currIndex](x, y, 0.0f);
        }
    }

    _waterPositions = positionBuff.dataArray;
    _waterNormals = normalBuff.dataArray;

    _waterVao.createBuffer("position", positionBuff);
    _waterVao.createBuffer("normal",   normalBuff);
    _waterVao.createBuffer("texCoord", texCoordBuff);

    _waterMaterial.diffuse(1.0f, 1.0f, 1.0f, 1.0f);
    _waterMaterial.specular(0.8f, 0.8f, 0.8f, 1.0f);
    _waterMaterial.shininess = 100.0f;
    _waterMaterial.fresnel = 0.05f;
}

void CpuWaterSim::setupLight()
{
    _pointLight.ambient  = Vec4f(0.02f, 0.02f, 0.02f, 1.0f);
    _pointLight.diffuse  = Vec4f(0.9f, 0.8f, 0.6f, 1.0f);
    _pointLight.specular = Vec4f(0.6f, 0.5f, 0.2f, 1.0f);
    _pointLight.position = Vec4f(0.9f, 0.9f, 0.9f, 1.0f);
    _pointLight.attenuationCoefs = Vec4f(0.5f, 0.3f, 0.8f, 0.0f);
}

void CpuWaterSim::setupTextures()
{
    _groundTex = GlToolkit::genTextureId(getImageBank().
        getImage("resources/textures/dirt.bmp"));

    _wallsTex = GlToolkit::genTextureId(getImageBank().
        getImage("resources/textures/woodwall.bmp"));

    _waterTex = GlToolkit::genTextureId(getImageBank().
        getImage("resources/textures/water.bmp"));
}

void CpuWaterSim::setupShader()
{
    GlInputsOutputs locations;
    locations.setInput(0, "position");
    locations.setInput(1, "normal");
    locations.setInput(2, "texCoord");
    _renderShader.setInAndOutLocations(locations);
    _renderShader.addShader(GL_VERTEX_SHADER, "resources/shaders/renderCpu.vert");
    _renderShader.addShader(GL_FRAGMENT_SHADER, "resources/shaders/renderCpu.frag");
    _renderShader.link();

    _renderShader.pushProgram();
    _renderShader.setInt("DiffuseTex", 0);
    _renderShader.setVec4f("light.ambient", _pointLight.ambient);
    _renderShader.setVec4f("light.diffuse", _pointLight.diffuse);
    _renderShader.setVec4f("light.specular", _pointLight.specular);
    _renderShader.setVec4f("light.attenuationCoefs", _pointLight.attenuationCoefs);
    _renderShader.popProgram();
}

float CpuWaterSim::groundInitHeight(int i, int j)
{

    float x, y;
    realPosition(i, j, x, y);

    //return 0.1f;

/*
    // Beam
    if(Vec2f(x, y).distanceTo(0.5f, 0.75f) < 0.1f)
        return 0.65;
    return 0.1f;
//*/


//*
    if(x < 0.45f || x > 0.55f)
        return 0.1;
    if(y < 0.38f || y > 0.62f)
        return 0.55f;
    if(y > 0.43f && y < 0.57f)
        return 0.55f;
    return 0.1f;

//*/

/*
    //Parking
    if(x>0.3f && x<0.6f &&
       y>0.2f)
        return 0.8f;

    if(x <= 0.3f)
        return 0.4f;
    if(x >= 0.6f)
        return 0.1f;
    return 0.7f-x;
//*/
}

float CpuWaterSim::waterInitHeight(int i, int j)
{
    float x, y;
    realPosition(i, j, x, y);

/*
    // Wave slot
    float length = 0.1f;
    if(x < length && y>=0.4f && y<=0.6f)
        return 0.55f + cos(x/length*PI)*0.15f;
    return 0.4f; //+ SimplexNoise::noise2d(x*20, y*20) * 0.003;

//*/
//*
    //Line wave
    const float start = 0.0f;
    const float length = 0.1f;
    const float middle = 0.35f;
    const float amplitude = 0.16f;

    float distance = x;

    if(distance < start)
        return middle + amplitude;
    if(distance < start + length)
        return middle + cos(PI*(distance)/length)*amplitude;
    else
        return middle - amplitude;
//*/
/*
    //Middle drop
    const float radius = 0.1f;
    const float amplitude = 0.15f;
    const float middle = 0.5f;
    float distance = Vec2f(x, y).distanceTo(0.5f, 0.5f);
    if(distance < radius)
        return middle + cos(distance*PI/radius)*amplitude;
    return middle-amplitude;
//*/

/*
    //Parking
    if(x<=0.3f && y>=0.7f)
        return y-0.3f;
    return 0.0f;
//*/
}

float CpuWaterSim::waterInitVelocity(int i, int j)
{
    float x, y;
    realPosition(i, j, x, y);

    return 0.0f;
}
