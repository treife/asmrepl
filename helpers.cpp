#include "helpers.h"

#include <format>

const char* getSelfPath() {
	static char self[MAX_PATH] = {0};
	if (!self[0])
		GetModuleFileName(NULL, self, sizeof self);
	return self;
}

std::string getTempFilePath() {
	char temp_dir[MAX_PATH];
	GetTempPathA(sizeof temp_dir, temp_dir);
	char path[MAX_PATH];
	GetTempFileNameA(temp_dir, "ASM", 0, path);
	return path;
}

std::string getErrorMessage(DWORD id) {
	if (0 == id)
		id = GetLastError();
	if (0 == id)
		return {};

	char* buf;
	constexpr DWORD FLAGS = FORMAT_MESSAGE_ALLOCATE_BUFFER |
	                        FORMAT_MESSAGE_FROM_SYSTEM |
	                        FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD length = FormatMessageA(FLAGS, nullptr, id, 0, (char*)&buf, 0, nullptr);
	if (0 == length)
		return {};

	LocalFree(buf);
	return std::format("({}){}", id, buf);
}