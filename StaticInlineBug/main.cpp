#include "Types.h"
#include "TypeIndex.h"
#include "Storage.h"
#include <assert.h>

int DoSomething1(DEM::Game::CGameWorld& World);
int DoSomething2(DEM::Game::CGameWorld& World);

void main()
{
	DEM::Game::CGameWorld World;
	assert(DoSomething1(World) == DoSomething2(World));
	assert(World._Storages.size() == 3);
}