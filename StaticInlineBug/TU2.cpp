#include "TypeIndex.h"
#include "Types.h"

void WorkWithTypes(Registry& registry)
{
	registry.ForEachComponent<Type2>([](Type2& AnimComponent)
	{
		// do something
	});
}
