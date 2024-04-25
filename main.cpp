#include <cstring>

#include "zombie_main.h"
#include "interactive.h"

int main(int argc, char* argv[]) {
	if (argc == 2 && !strcmp(argv[1], "--zombie"))
		return mainZombie();
	else
		return mainInteractive();
}