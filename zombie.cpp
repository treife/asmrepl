#include "zombie.h"
#include "helpers.h"

#include <iostream>
#include <utility>
#include <string>
#include <chrono>

Zombie::~Zombie() {
	kill();
}

Zombie::Zombie(Zombie&& zombie) noexcept {
	*this = std::move(zombie);
}

Zombie& Zombie::operator=(Zombie&& zombie) noexcept {
	if (this == &zombie)
		return *this;

	kill();

	shellcode_space = zombie.shellcode_space;
	shellcode_size = zombie.shellcode_size;
	shellcode_space_size = zombie.shellcode_space_size;
	proc_id	 = zombie.proc_id;
	thread_id = zombie.thread_id;
	proc_handle = zombie.proc_handle;
	thread_handle = zombie.thread_handle;

	zombie.shellcode_space = nullptr;
	zombie.shellcode_size = 0;
	zombie.shellcode_space_size = 0;
	zombie.proc_id = 0;
	zombie.thread_id = 0;
	zombie.proc_handle = NULL;
	zombie.thread_handle = NULL;

	return *this;
}

bool Zombie::spawn(const char* binary) {
	STARTUPINFO si = {0};
	si.cb = sizeof si;
	PROCESS_INFORMATION proc_info;
	std::string cmdline = "\"" + std::string(binary) + "\" --zombie";
	if (!CreateProcessA(binary, cmdline.data(), nullptr, nullptr, false,
	                    DEBUG_ONLY_THIS_PROCESS, nullptr, nullptr,
	                    &si, &proc_info)) {
		std::cerr << "CreateProcess error: " << getErrorMessage() << '\n';
		return false;
	}
	CloseHandle(proc_info.hProcess);
	CloseHandle(proc_info.hThread);

	const auto tm_timeout = std::chrono::system_clock::now() + std::chrono::seconds(5);
	while (std::chrono::system_clock::now() < tm_timeout) {
		DEBUG_EVENT ev;
		while (WaitForDebugEvent(&ev, 500)) {
			if (CREATE_PROCESS_DEBUG_EVENT == ev.dwDebugEventCode) {
				CloseHandle(ev.u.CreateProcessInfo.hFile);
				proc_id = proc_info.dwProcessId;
				thread_id = proc_info.dwThreadId;
				proc_handle = ev.u.CreateProcessInfo.hProcess;
				thread_handle = ev.u.CreateProcessInfo.hThread;
	
				if (!writeShellcode({0xCC})) {
					std::cerr << "Zombie has timed out\n";
					kill();
					return false;
				}

				runShellcode();
			}
			else if (EXCEPTION_DEBUG_EVENT == ev.dwDebugEventCode) {
				const EXCEPTION_RECORD& exc = ev.u.Exception.ExceptionRecord;
				if (EXCEPTION_BREAKPOINT == exc.ExceptionCode &&
				    exc.ExceptionAddress == shellcode_space) {
					// No more system exceptions will be thrown
					return true;
				}

				ContinueDebugEvent(proc_id, thread_id, DBG_CONTINUE);
			}
			else {
				ContinueDebugEvent(proc_id, thread_id, DBG_CONTINUE);
			}
		}
	}

	return true;
}

void Zombie::kill() {
	if (shellcode_space) {
		VirtualFreeEx(proc_handle, shellcode_space, 0, MEM_RELEASE);
		shellcode_space = nullptr;
	}

	if (proc_handle) {
		DebugActiveProcessStop(proc_id);
		proc_handle = NULL;
	}

	if (thread_handle) {
		CloseHandle(thread_handle);
		thread_handle = NULL;
	}

	shellcode_size = 0;
	shellcode_space_size = 0;
	proc_id = 0;
	thread_id = 0;
}

bool Zombie::writeShellcode(const std::vector<std::uint8_t>& shellcode) {
	shellcode_size = shellcode.size();

	// +1 byte (SHELLCODE_END_SIZE) for the int3 past the shellcode
	if (shellcode_size >= shellcode_space_size) {
		if (shellcode_space)
			VirtualFreeEx(proc_handle, shellcode_space, 0, MEM_RELEASE);
		shellcode_space_size = shellcode.size() + SHELLCODE_END_SIZE;
		shellcode_space = VirtualAllocEx(proc_handle, nullptr,
		                                 shellcode_space_size,
		                                 MEM_COMMIT | MEM_RESERVE,
		                                 PAGE_EXECUTE_READWRITE);
		if (!shellcode_space) {
			std::cerr << "Couldn't allocate shellcode space: " << getErrorMessage() << '\n';
			return false;
		}
	}

	WriteProcessMemory(proc_handle, shellcode_space,
	                   shellcode.data(), shellcode_size, nullptr);
	WriteProcessMemory(proc_handle, (char*)shellcode_space + shellcode_size,
	                   SHELLCODE_ENDING, SHELLCODE_END_SIZE, nullptr);

	return true;
}

void Zombie::runShellcode() {
	CONTEXT ctx = {0};
	ctx.ContextFlags = CONTEXT_CONTROL;
	GetThreadContext(thread_handle, &ctx);
	ctx.Rip = reinterpret_cast<DWORD64>(shellcode_space);
	SetThreadContext(thread_handle, &ctx);

	ContinueDebugEvent(proc_id, thread_id, DBG_CONTINUE);
}

bool Zombie::waitForDebugEvent(DEBUG_EVENT& ev) {
	// Return false only if the wait timed out
	return !(!WaitForDebugEvent(&ev, 500) && GetLastError() == ERROR_SEM_TIMEOUT);
}