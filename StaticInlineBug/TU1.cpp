#include "TypeIndex.h"
#include "Types.h"

int DoSomething1(DEM::Game::CGameWorld& World)
{
	World.RegisterComponent<DEM::Game::Type1>();
	World.RegisterComponent<DEM::Game::Type2>();
	World.RegisterComponent<DEM::Game::Type3>();

	int typeIdx = -1;
	World.ForEachEntityWith<DEM::Game::Type2, DEM::Game::Type1>(
		[&typeIdx](int idx, DEM::Game::Type2& t2, DEM::Game::Type1* t1)
	{
		typeIdx = idx;
	});

	return typeIdx;
}
