#include "TypeIndex.h"
#include <assert.h>

void DoSomething1(DEM::Game::CGameWorld& World);
void DoSomething2(DEM::Game::CGameWorld& World);

void main()
{
	DEM::Game::CGameWorld World;
	DoSomething1(World);
	DoSomething2(World);
	assert(World._Storages.size() == 3);
}