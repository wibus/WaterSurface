#ifndef WATERCHARACTER_H
#define WATERCHARACTER_H

#include <vector>

#include <DataStructure/Vector.h>
#include <Camera/Camera.h>
#include <Camera/CameraManFree.h>
#include <GL/GlProgram.h>
#include <Hud/TextHud.h>
#include <Character/AbstractCharacter.h>


class WaterCharacter : public scaena::AbstractCharacter,
                       public cellar::SpecificObserver<media::CameraMsg>
{
public:
    WaterCharacter(scaena::AbstractStage& stage);
    virtual ~WaterCharacter();

    // Character interface
    virtual void enterStage();
    virtual void beginStep(const scaena::StageTime &time);
    virtual void endStep(const scaena::StageTime &time);
    virtual void draw(const scaena::StageTime &time);
    virtual void exitStage();

    // Specific observer interface
    virtual void notify(media::CameraMsg &msg);

private:
    void setupWater();
    void setupShaders();
    void setupVaos();


    cellar::Vec2i _size;
    std::vector<unsigned int> _latticeIndices;

    unsigned int _waterVelocityFbo;
    unsigned int _waterVelocityTex;
    unsigned int _waterHeightFbo;
    unsigned int _waterHeightTex;    
    unsigned int _waterLatticeVao;
    unsigned int _waterLatticeBuffer;

    unsigned int _groundLatticeVao;
    unsigned int _groundLatticeBuffer;

    unsigned int _quadVao;
    unsigned int _quadBuffer;

    media::GlProgram _waterVelocityShader;
    media::GlProgram _waterHeightShader;
    media::GlProgram _renderShader;

    media::CameraManFree _cameraMan;
    std::shared_ptr<prop2::TextHud> _fps;
};

#endif // WATERCHARACTER_H
