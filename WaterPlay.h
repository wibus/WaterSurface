#ifndef WATERPLAY_H
#define WATERPLAY_H

#include <Play/SingleActPlay.h>


class WaterPlay : public scaena::SingleActPlay
{
public:
    WaterPlay();

    virtual void loadExternalRessources();
    virtual void setUpPersistentCharacters();
};

#endif // WATERPLAY_H
