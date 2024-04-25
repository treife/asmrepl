#ifndef ZOMBIE_H
#define ZOMBIE_H
#include <vector>
#include <cstdint>
#include <Windows.h>

class Zombie {
public:
	Zombie() = default;
	~Zombie();
	Zombie(const Zombie& zombie) = delete;
	Zombie& operator=(const Zombie& zombie) = delete;
	Zombie(Zombie&& zombie) noexcept;
	Zombie& operator=(Zombie&& zombie) noexcept;

	bool spawn(const char* binary);
	void kill();

	bool writeShellcode(const std::vector<std::uint8_t>& shellcode);
	void runShellcode();

	bool waitForDebugEvent(DEBUG_EVENT& ev);

	DWORD getProcId() const { return proc_id; }
	DWORD getThreadId() const { return thread_id; }
	HANDLE getProcHandle() const { return proc_handle; }
	HANDLE getThreadHandle() const { return thread_handle; }
	std::size_t getShellcodeSize() const { return shellcode_size; }
	const void* getShellcodeSpace() const { return shellcode_space; }
	std::size_t getShellcodeSpaceSize() const { return shellcode_space_size; }
private:
	void* shellcode_space{nullptr};
	std::size_t shellcode_size{0};
	std::size_t shellcode_space_size{0};

	DWORD proc_id{0};
	DWORD thread_id{0};
	HANDLE proc_handle{NULL};
	HANDLE thread_handle{NULL};

	// 1 byte past the shellcode = int3 / 0xCC
	// There shellcode_size equal be at most shellcode_space_size - SHELLCODE_END_SIZE
	const char* SHELLCODE_ENDING = "\xCC";
	const std::size_t SHELLCODE_END_SIZE = 1;
};
#endif
