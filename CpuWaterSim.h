#ifndef CPUWATERSIM_H
#define CPUWATERSIM_H

#include <DesignPattern/SpecificObserver.h>
#include <Graphics/Camera/Camera.h>
#include <Graphics/Camera/CameraManFree.h>
#include <Graphics/Light/Light3D.h>
#include <Graphics/GL/GlProgram.h>
#include <Graphics/GL/GlVao.h>
#include <Graphics/GL/GLFFmpegCamcorder.h>

#include <Hud/TextHud.h>

#include <Character/AbstractCharacter.h>

#include <vector>

#include <cassert>


struct Node
{
    int index;
    bool isExchangePermitted;
    bool isSurpervisingExchange;
    float baseContribution;
    float currContribution;
    float velocity;
};

struct Vertex
{
    Node node;
    std::vector< Node > neighbors;
};


class CpuWaterSim : public scaena::AbstractCharacter,
                    public cellar::SpecificObserver<cellar::CameraMsg>
{
public:
    CpuWaterSim(scaena::AbstractStage& stage);
    virtual ~CpuWaterSim();

    virtual void enterStage();
    virtual void beginStep(const scaena::StageTime &time);
    virtual void endStep(const scaena::StageTime &time);
    virtual void draw(const scaena::StageTime &time);
    virtual void exitStage();

    virtual bool keyPressEvent(const scaena::KeyboardEvent& event);

    virtual void notify(cellar::CameraMsg &msg);

protected:
    void setupLattice();
    void setupGround();
    void setupWalls();
    void setupWater();
    void setupVerticesAndNeighbors();
    void setupLight();
    void setupTextures();
    void setupShader();

    // Vertex attribute
    int index(int i, int j);
    bool isInBounds(int i, int j);
    bool isNeighbor(int ni, int nj);
    float baseContribution(int ni, int nj);
    void position(int index, int& i, int& j);
    float height(int index);
    float floorHeight(int index);
    float overFloor(int index);
    bool isOnFloor(int index);
    float deltaHeight(int index1, int index2);
    bool isExchangePermitted(int index1, int index2);
    bool needExchangeHandling(const Node& neighbor);

    void realPosition(int i, int j, float& x, float& y);
    float groundInitHeight(int i, int j);
    float waterInitHeight(int i, int j);
    float waterInitVelocity(int i, int j);

private:
    const float _STRETCHNESS;
    const float _LOSSYNESS;

    const int _WIDTH;
    const int _HEIGHT;
    const int _ARRAY_SIZE;

    const int _NEIGHBORS_RADIUS;

    std::vector< Vertex > _vertices;

    std::vector<unsigned int> _latticeIndices;

    GLuint _groundTex;
    cellar::GlVao _groundVao;
    std::vector<cellar::Vec3f> _groundPositions;
    cellar::Material _groundMaterial;

    GLuint _wallsTex;
    cellar::GlVao _wallsVao;
    cellar::Material _wallsMaterial;

    GLuint _waterTex;
    cellar::GlVao _waterVao;
    std::vector<cellar::Vec3f> _waterPositions;
    std::vector<cellar::Vec3f> _waterNormals;
    std::vector<float> _waterVelocities;
    cellar::Material _waterMaterial;

    cellar::PointLight3D _pointLight;
    cellar::CameraManFree _cameraMan;
    cellar::GlProgram _renderShader;

    std::shared_ptr<prop2::TextHud> _fps;

    //cellar::GLFFmpegCamcorder _camcorder;
};



// IMPLEMENTATION //
inline int CpuWaterSim::index(int i, int j)
{
    assert( isInBounds(i, j) );
    return j*_WIDTH + i;
}

inline bool CpuWaterSim::isInBounds(int i, int j)
{
    return cellar::inRange(i, 0, _WIDTH-1) &&
           cellar::inRange(j, 0, _HEIGHT-1);
}

inline bool CpuWaterSim::isNeighbor(int ni, int nj)
{
    return cellar::Vec2f(ni, nj).length() <= _NEIGHBORS_RADIUS;
}

inline float CpuWaterSim::baseContribution(int ni, int nj)
{
    assert( !((ni == 0) && (nj == 0)) );
    return 1.0f / cellar::Vec2f(ni, nj).length2();
}

inline void CpuWaterSim::position(int index, int& i, int& j)
{
    assert( (0 <= index) && (index < _WIDTH*_HEIGHT) );
    i = index % _WIDTH;
    j = index / _WIDTH;
}

inline float CpuWaterSim::height(int index)
{
    return _waterPositions[index].z();
}

inline float CpuWaterSim::floorHeight(int index)
{
    return _groundPositions[index].z();
}

inline float CpuWaterSim::overFloor(int index)
{
    float over = height(index) - floorHeight(index);
    if(over < 0.0f)
    {
        _waterPositions[index].setZ(_groundPositions[index].z());
        over = 0.0f;
    }
    return over;
}

inline bool CpuWaterSim::isOnFloor(int index)
{
    return height(index) <= floorHeight(index);
}

inline float CpuWaterSim::deltaHeight(int index1, int index2)
{
    return height(index2) - height(index1);
}

inline bool CpuWaterSim::isExchangePermitted(int index1, int index2)
{
    float dz = deltaHeight(index1, index2);

    if(dz == 0.0f)
        return false;
    if(dz < 0.0f && isOnFloor(index1))
        return false;
    if(dz > 0.0f && isOnFloor(index2))
        return false;

    return true;
}

inline bool CpuWaterSim::needExchangeHandling(const Node& neighbor)
{
    return neighbor.isExchangePermitted &&
          !neighbor.isSurpervisingExchange;
}

inline void CpuWaterSim::realPosition(int i, int j, float& x, float& y)
{
    assert( isInBounds(i, j) );
    x = i / static_cast<float>(_WIDTH);
    y = j / static_cast<float>(_HEIGHT);
}

#endif // CPUWATERSIM_H
