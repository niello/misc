#include <assert.h>

int DoSomething1();
int DoSomething2();

void main()
{
	assert(DoSomething1() == DoSomething2());
}