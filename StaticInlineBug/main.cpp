#include "TypeIndex.h"

void RegisterTypes(Registry& registry);

void main()
{
	Registry registry;

	// Registry::typeIndex access happens in translation unit TU1.cpp
	RegisterTypes(registry);

	// Registry::typeIndex access happens in translation unit main.cpp
	// Another Registry::typeIndex<Type2> instance is initialized because of this.
	registry.DoSomethingWithType<Type2>();

	assert(registry.types.size() == 3);
}