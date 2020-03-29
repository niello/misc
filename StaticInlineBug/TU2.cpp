#include "TypeIndex.h"
#include "Types.h"

void DoSomething2(DEM::Game::CGameWorld& World)
{
	World.ForEachComponent<DEM::Game::Type2>([](DEM::Game::Type2& AnimComponent)
	{
		// do something
	});
}
