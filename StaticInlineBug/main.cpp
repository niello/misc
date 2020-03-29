#include "TypeIndex.h"
#include <assert.h>

void RegisterTypes(Registry& registry);
void WorkWithTypes(Registry& registry);

void main()
{
	Registry registry;
	RegisterTypes(registry);
	//WorkWithTypes(registry);
	assert(registry.storages.size() == 3);
}