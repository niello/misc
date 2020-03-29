#include "TypeIndex.h"
#include "Types.h"

int DoSomething2(DEM::Game::CGameWorld& World)
{
	int typeIdx = -1;
	World.ForEachComponent<DEM::Game::Type2>([&typeIdx](int idx, DEM::Game::Type2& AnimComponent)
	{
		typeIdx = idx;
	});

	return typeIdx;
}
