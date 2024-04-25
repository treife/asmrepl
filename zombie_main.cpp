#include "zombie.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <Windows.h>

int mainZombie() {
	if (!IsDebuggerPresent()) {
		std::cerr << "Can't run zombie as standalone application\n";
		return 1;
	}

	while (true)
		std::this_thread::sleep_for(std::chrono::seconds(1));

	return 0;
}