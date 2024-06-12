#include "main.h"

int main() {
	IOCPServ ioServ;
	ioServ.Init();
	ioServ.StartServer();

	while (true) {

	}

	ioServ.DestroyThread();

	return 0;
}