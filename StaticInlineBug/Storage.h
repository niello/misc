#pragma once
#include <vector>

struct IStorage
{
	virtual ~IStorage() = default;
};

template<typename T>
struct CStorageImpl : public IStorage, public std::vector<T>
{
	virtual ~CStorageImpl() override = default;
};
