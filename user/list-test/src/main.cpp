#include <stacsos/atomic.h>
#include <stacsos/console.h>
#include <stacsos/objects.h>
#include <stacsos/threads.h>
#include <stacsos/list.h>

using namespace stacsos;


struct test_str{
	int a;
};
int main(const char *cmdline)
{
	list<int> i_list;
	for (int x =0; x<1000; x++){
		// test_str *b = new test_str();
		console::get().writef("%d\n",x);
		// delete b;
		i_list.push(x);
		i_list.clear();
	} 
}