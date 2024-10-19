#include <stacsos/console.h>
using namespace stacsos;

int main(const char *cmdline)
{
	char* a;
	for (int x =0; x<1000; x++){
		console::get().writef("%d\n",x);
		a = new char;
		delete a;
	} 
	return 0;
}