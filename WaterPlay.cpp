#include <memory>
using namespace std;

#include "WaterPlay.h"
#include "WaterCharacter.h"
#include "CpuWaterSim.h"
using namespace scaena;


WaterPlay::WaterPlay() :
    SingleActPlay("WaterPlay")
{
}

void WaterPlay::loadExternalRessources()
{
}

void WaterPlay::setUpPersistentCharacters()
{
    addPersistentCharacter(
        shared_ptr<AbstractCharacter>(new CpuWaterSim( stage() ))
    );
}
