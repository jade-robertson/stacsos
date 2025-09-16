#include <stacsos/console.h>
#include <stacsos/list.h>
using namespace stacsos;

int main(const char *cmdline)
{
	list<int> test_list;
	for (int x =0; x<1000; x++){
        test_list.append(x);
	} 
    while(!test_list.empty()){
        console::get().writef("%d\n",test_list.pop());
    }
	return 0;
}