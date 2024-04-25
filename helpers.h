#ifndef HELPERS_H
#define HELPERS_H
#include <string>
#include <Windows.h>

const char* getSelfPath();
std::string getTempFilePath();
std::string getErrorMessage(DWORD id=0);
#endif
