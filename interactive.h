#ifndef INTERACTIVE_H
#define INTERACTIVE_H
#include <string>
#include <string_view>
#include <cstdint>
#define NOMINMAX
#include <Windows.h>

#include "zombie.h"

class Interactive {
public:
	int run();
private:
	void handleCommandInput(int ch);
	void handleReplCommand(std::string_view cmd);

	void debugZombie();

	std::size_t cursorPosToLnBufIndex() const;
	COORD lnBufIndexToCursorPos(std::size_t idx) const;
	void advanceCursor(short off=1);
	void scroll(short row);
	void reprintLn(std::size_t from_idx, std::size_t to_idx);
	bool got224{false};
	HANDLE console;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	COORD cursor_pos;
	short prompt_pos_y;
	std::string ln_buf;
	const std::string PROMPT{"> "};

	Zombie zombie;
	CONTEXT old_ctx{};
	enum {CMD_INPUT} input_mode{CMD_INPUT};
	bool quit{false};
};

int mainInteractive();
#endif
