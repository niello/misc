#pragma once
#include "Storage.h"
#include <vector>
#include <memory>

/////////////////////////////////////////////////////
// Utilities
/////////////////////////////////////////////////////

template<class T>
struct just_type
{
	using type = std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>;
};

template<class T> using just_type_t = typename just_type<T>::type;

template<class T>
struct ensure_pointer
{
	using type = std::remove_reference_t<std::remove_pointer_t<T>>*;
};

template<class T> using ensure_pointer_t = typename ensure_pointer<T>::type;

template <typename T, std::size_t ... Indices>
auto tuple_pop_front_impl(const T& tuple, std::index_sequence<Indices...>)
{
	return std::make_tuple(std::get<1 + Indices>(tuple)...);
}

template <typename T>
auto tuple_pop_front(const T& tuple)
{
	return tuple_pop_front_impl(tuple, std::make_index_sequence<std::tuple_size<T>::value - 1>());
}

/////////////////////////////////////////////////////
// Main class
/////////////////////////////////////////////////////

namespace DEM::Game
{

class CGameWorld final
{
public:

	// Zero-based type index for fast component storage access
	inline static uint32_t ComponentTypeCount = 0;
	template<typename T> inline static const uint32_t ComponentTypeIndex = ComponentTypeCount++;

	std::vector<std::unique_ptr<IStorage>> _Storages;

	template<typename TComponent, typename... Components>
	bool GetNextStorages(std::tuple<CStorageImpl<TComponent>*, CStorageImpl<Components>*...>& Out);
	template<typename TComponent, typename... Components>
	bool GetNextComponents(int EntityID, std::tuple<ensure_pointer_t<TComponent>, ensure_pointer_t<Components>...>& Out, const std::tuple<CStorageImpl<TComponent>*, CStorageImpl<Components>*...>& Storages);

	template<class T> void RegisterComponent();
	template<class T> typename CStorageImpl<T>* FindComponentStorage();

	template<typename TComponent, typename... Components, typename TCallback>
	void ForEachEntityWith(TCallback Callback);
	template<typename TComponent, typename TCallback>
	void ForEachComponent(TCallback Callback);
};

template<class T>
void CGameWorld::RegisterComponent()
{
	// Static type is enough for distinguishing between different components, no dynamic RTTI needed
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Storages.size() <= TypeIndex)
		_Storages.resize(TypeIndex + 1);
	auto storage = std::make_unique<CStorageImpl<T>>();
	storage->resize(1);
	_Storages[TypeIndex] = std::move(storage);
}
//---------------------------------------------------------------------

template<class T>
typename CStorageImpl<T>* CGameWorld::FindComponentStorage()
{
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Storages.size() <= TypeIndex) return nullptr;

	return static_cast<CStorageImpl<T>*>(_Storages[TypeIndex].get());
}
//---------------------------------------------------------------------

template<typename TComponent, typename... Components>
inline bool CGameWorld::GetNextStorages(std::tuple<CStorageImpl<TComponent>*, CStorageImpl<Components>*...>& Out)
{
	CStorageImpl<TComponent>* pStorage = nullptr;
	std::tuple<CStorageImpl<Components>*...> NextStorages;

	bool NextOk = true;
	if constexpr(sizeof...(Components) > 0)
		NextOk = GetNextStorages<Components...>(NextStorages);

	if (NextOk)
		pStorage = FindComponentStorage<just_type_t<TComponent>>();

	Out = std::tuple_cat(std::make_tuple(pStorage), NextStorages);

	return true;
}
//---------------------------------------------------------------------

template<typename TComponent, typename... Components>
bool CGameWorld::GetNextComponents(int EntityID, std::tuple<ensure_pointer_t<TComponent>, ensure_pointer_t<Components>...>& Out, const std::tuple<CStorageImpl<TComponent>*, CStorageImpl<Components>*...>& Storages)
{
	ensure_pointer_t<TComponent> pComponent = nullptr;
	std::tuple<ensure_pointer_t<Components>...> NextComponents;

	bool NextOk = true;
	if constexpr(sizeof...(Components) > 0)
		NextOk = GetNextComponents<Components...>(EntityID, NextComponents, tuple_pop_front(Storages));

	if (NextOk)
		if (auto pStorage = std::get<0>(Storages))
			pComponent = nullptr;

	Out = std::tuple_cat(std::make_tuple(pComponent), NextComponents);

	return true;
}
//---------------------------------------------------------------------

template<typename TComponent, typename... Components, typename TCallback>
inline void CGameWorld::ForEachEntityWith(TCallback Callback)
{
	static_assert(!std::is_pointer_v<TComponent>, "First component in ForEachEntityWith must be mandatory!");

	if (auto pStorage = FindComponentStorage<just_type_t<TComponent>>())
	{
		std::tuple<CStorageImpl<Components>*...> NextStorages;
		if constexpr(sizeof...(Components) > 0)
			if (!GetNextStorages<Components...>(NextStorages)) return;

		for (auto&& Component : *pStorage)
		{
			int EntityID = 0;

			std::tuple<ensure_pointer_t<Components>...> NextComponents;
			if constexpr(sizeof...(Components) > 0)
				if (!GetNextComponents<Components...>(EntityID, NextComponents, NextStorages)) continue;

			if constexpr (std::is_const_v<TComponent>)
				std::apply(std::forward<TCallback>(Callback), std::tuple_cat(std::make_tuple(std::cref(Component)), NextComponents));
			else
				std::apply(std::forward<TCallback>(Callback), std::tuple_cat(std::make_tuple(std::ref(Component)), NextComponents));
		}
	}
}
//---------------------------------------------------------------------

template<typename TComponent, typename TCallback>
inline void CGameWorld::ForEachComponent(TCallback Callback)
{
	if (auto pStorage = FindComponentStorage<just_type_t<TComponent>>())
	{
		for (auto&& Component : *pStorage)
		{
			if constexpr (std::is_const_v<TComponent>)
				Callback(std::cref(Component));
			else
				Callback(std::ref(Component));
		}
	}
}
//---------------------------------------------------------------------

}
