#include "TypeIndex.h"
#include "Types.h"

void DoSomething1(DEM::Game::CGameWorld& World)
{
	World.RegisterComponent<DEM::Game::Type1>();
	World.RegisterComponent<DEM::Game::Type2>();
	World.RegisterComponent<DEM::Game::Type3>();

	//World.ForEachEntityWith<DEM::Game::Type2, DEM::Game::Type1>(
	//	[](DEM::Game::Type2& t2, DEM::Game::Type1* t1)
	//{
	//	// do something
	//});
}
