#include "TypeIndex.h"
#include "Types.h"

void RegisterTypes(Registry& registry)
{
	registry.RegisterType<Type1>();
	registry.RegisterType<Type2>();
	registry.RegisterType<Type3>();
}
