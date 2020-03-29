#pragma once
#include "Storage.h"
#include <vector>
#include <memory>

class Registry final
{
public:

	// The key part of the problem. Each typeIndex<T> must be unique across all the application.
	inline static uint32_t nextTypeIndex = 0;
	template<typename T> inline static const uint32_t typeIndex = nextTypeIndex++;

	std::vector<std::unique_ptr<IStorage>> storages;

	template<class T> void RegisterType()
	{
		const auto idx = typeIndex<T>;
		if (storages.size() <= idx) storages.resize(idx + 1);
		storages[idx] = std::make_unique<CStorageImpl<T>>();
	}

	template<typename T, typename TCallback>
	void ForEachComponent(TCallback Callback)
	{
		const auto TypeIndex = typeIndex<T>;
		if (storages.size() <= TypeIndex) return;

		if (auto pStorage = static_cast<CStorageImpl<T>*>(storages[TypeIndex].get()))
			for (auto&& Component : *pStorage)
				Callback(std::ref(Component));
	}
};
