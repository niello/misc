struct TypeIndex
{
	inline static int next = 0;
	template<typename T> inline static const int value = next++;
};
