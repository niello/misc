#pragma once
#include <vector>

#undef NDEBUG // For assertion failures in release
#include <assert.h>

struct Type1 {};
struct Type2 {};
struct Type3 {};

class Registry final
{
public:

	// The key part of the problem. Each typeIndex<T> must be unique across all the application.
	inline static uint32_t nextTypeIndex = 0;
	template<typename T> inline static const uint32_t typeIndex = nextTypeIndex++;

	std::vector<bool> types;

	template<class T> void RegisterType()
	{
		const auto idx = typeIndex<T>;
		if (types.size() <= idx) types.resize(idx + 1);
		types[idx] = true;
	}

	template<typename T>
	void DoSomethingWithType()
	{
		const auto idx = typeIndex<T>;
		assert(idx < types.size() && types[idx]); // Always passes, used to prevent optimization
	}
};
