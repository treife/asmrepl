#include "interactive.h"
#include "zombie.h"
#include "helpers.h"
#include "assembler.h"
#include "pretty_print.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <format>
#include <conio.h>

int Interactive::run() {
	std::cout << reset;

	if (!zombie.spawn(getSelfPath())) {
		std::cerr << "Couldn't spawn zombie.\n";
		return 1;
	}
	std::cout << "Spawned process " << Color::FG_INTENSITY << zombie.getProcId() << reset << '\n';

	console = GetStdHandle(STD_OUTPUT_HANDLE);

	std::cout << PROMPT;
	GetConsoleScreenBufferInfo(console, &coninfo);
	prompt_pos_y = coninfo.dwCursorPosition.Y;
	cursor_pos = coninfo.dwCursorPosition;

	while (!quit) {
		int ch = _getch();
		if (!ch)
			continue;

		switch (input_mode) {
		case CMD_INPUT: handleCommandInput(ch); break;
		default: break;
		}
	}

	std::cout << reset;

	return 0;
}

void Interactive::handleCommandInput(int ch) {
	const short typing_row = lnBufIndexToCursorPos(ln_buf.length()).Y;
	scroll(typing_row);

	if (224 == ch) {
		got224 = true;
	}
	else if (3 == ch) { // ^C
		quit = true;
	}
	else if (1 == ch) { // ^A
		cursor_pos.X = PROMPT.length();
		cursor_pos.Y = prompt_pos_y;
		SetConsoleCursorPosition(console, cursor_pos);
	}
	else if (83 == ch) { // Delete
		int ln_idx = cursorPosToLnBufIndex();
		if (ln_idx < ln_buf.length()) {
			ln_buf.erase(ln_buf.begin() + ln_idx);
			reprintLn(0, ln_buf.length() + 1);
		}
	}
	else if (8 == ch && !ln_buf.empty()) { // Backspace
		int ln_idx = cursorPosToLnBufIndex();
		if (ln_buf.length() == ln_idx)
			--ln_idx;
		ln_buf.erase(ln_buf.begin() + ln_idx);
		reprintLn(0, ln_buf.length() + 1);

		advanceCursor(-1);
	}
	else if (75 == ch) { // Left Arrow
		advanceCursor(-1);
	}
	else if (77 == ch) { // Right Arrow
		advanceCursor();
	}
	else if (13 == ch) { // Return
		std::cout << '\n';
		if (ln_buf.starts_with('!')) { // A REPL command
			std::string_view cmd = ln_buf;
			cmd.remove_prefix(1);
			handleReplCommand(cmd);
		}
		else if (!ln_buf.empty()) {
			Assembler assembler;
			std::vector<std::uint8_t> shellcode;
			std::string error;
			if (!assembler.assembleBuffer(ln_buf, shellcode, error)) {
				std::cout << Color::FG_RED << "Error: " << reset << error << '\n';
			}
			else {
				if (!error.empty())
					std::cout << Color::FG_YELLOW << "Warning: " << reset << error << '\n';

				// The shellcode might be empty
				// If there are no real instructions in the file (e.g. only a label)
				if (!shellcode.empty()) {
					std::cout << std::hex << std::setw(2);
					for (std::uint8_t b : shellcode)
						std::cout << static_cast<int>(b) << ' ';
					std::cout << '\n' << std::dec;

					zombie.writeShellcode(shellcode);
					zombie.runShellcode();

					debugZombie();
				}
			}
		}

		ln_buf = "";

		advanceCursor();
		GetConsoleScreenBufferInfo(console, &coninfo);
		prompt_pos_y = coninfo.dwCursorPosition.Y;
		std::cout << PROMPT;
	}
	else if (std::isprint(ch)) {
		ln_buf += ch;
		std::cout << static_cast<char>(ch);
		GetConsoleScreenBufferInfo(console, &coninfo);
		cursor_pos = coninfo.dwCursorPosition;
	}

	if (224 != ch && got224)
		got224 = false;
}

void Interactive::handleReplCommand(std::string_view cmd) {
	if ("quit" == cmd || "q" == cmd) {
		quit = true;
	}
	else if ("restart" == cmd || "r" == cmd) {
		zombie.kill();
		zombie.spawn(getSelfPath());
		std::cout << "Spawned process " << Color::FG_INTENSITY << zombie.getProcId() << reset << '\n';
	}
	else {
		std::cout << Color::FG_RED << "Error: " << reset << "Unknown command\n";
	}
}

void Interactive::debugZombie() {
	DEBUG_EVENT ev;
	while (zombie.waitForDebugEvent(ev)) {
		if (EXCEPTION_DEBUG_EVENT == ev.dwDebugEventCode) {
			const EXCEPTION_RECORD& exc = ev.u.Exception.ExceptionRecord;
			if (EXCEPTION_ACCESS_VIOLATION == exc.ExceptionCode) {
				std::cout << Color::FG_RED << std::format("Access violation @ {}", exc.ExceptionAddress) << reset << "\n";
				printExceptionRecord(exc);
				printZombieInfo(zombie);

				zombie.kill();
				zombie.spawn(getSelfPath());
				std::cout << "Spawned process " << Color::FG_INTENSITY << zombie.getProcId() << reset << '\n';
				return;
			}
			else if (EXCEPTION_BREAKPOINT == exc.ExceptionCode) {
				const void* shellcode_end = (char*)zombie.getShellcodeSpace()
					+ zombie.getShellcodeSize();
				// The shellcode has finished executing
				if (exc.ExceptionAddress == shellcode_end) {
					CONTEXT ctx;
					ctx.ContextFlags = CONTEXT_INTEGER;
					GetThreadContext(zombie.getThreadHandle(), &ctx);
					printRegisters(ctx, &old_ctx);
					old_ctx = ctx;
					std::cout << '\n';
					return;
				}
				else {
					if (ev.u.Exception.dwFirstChance) {
						std::cout << std::format("First chance breakpoint reached: {}", exc.ExceptionAddress) << '\n';
						ContinueDebugEvent(zombie.getProcId(), zombie.getThreadId(), DBG_CONTINUE);
					}
					else {
						std::cout << std::format("Breakpoint reached: {}", exc.ExceptionAddress) << '\n';
					}
					printZombieInfo(zombie);
					printExceptionRecord(exc);
				}
			}
			else if (EXCEPTION_NONCONTINUABLE_EXCEPTION == exc.ExceptionCode) {
				std::cout << Color::FG_RED << "A noncontinuable exception occurred. Respawning the process.\n" << reset;
				zombie.kill();
				zombie.spawn(getSelfPath());
				std::cout << "Spawned process " << Color::FG_INTENSITY << zombie.getProcId() << reset << '\n';
			}
			else {
				std::cout << Color::FG_RED << std::format("Exception! ({:x}@{})", exc.ExceptionCode, exc.ExceptionAddress) << reset << "\n";
				return;
			}
		}
		else {
			ContinueDebugEvent(zombie.getProcId(), zombie.getThreadId(), DBG_CONTINUE);
		}
	}
}

std::size_t Interactive::cursorPosToLnBufIndex() const {
	const int rows = cursor_pos.Y - prompt_pos_y;
	const int width = coninfo.srWindow.Right;
	return (width - PROMPT.length()) + width * rows - (width - cursor_pos.X);
}

COORD Interactive::lnBufIndexToCursorPos(std::size_t idx) const {
	const short width = coninfo.srWindow.Right;
	short x = PROMPT.length();
	short y = prompt_pos_y;
	if (idx > 0) {
		const short n_until_first_wrap = width - x;
		const short remaining = std::max<short>(0, idx - n_until_first_wrap);
		if (remaining > width) {
			y = y + remaining / width;
			x = remaining % width;
		}
		else if (remaining < width && remaining > 0) {
			y = y - remaining / width;
			x = remaining % width;
		}
		else {
			x = x + idx;
		}
	}
	return {x, y};
}

void Interactive::advanceCursor(short off) {
	// Don't jump past the prompt or line
	if (off < 0 && cursor_pos.Y == prompt_pos_y && cursor_pos.X == PROMPT.length())
		return;
	if (cursorPosToLnBufIndex() + off > ln_buf.length())
		return;

	const short width = coninfo.srWindow.Right;
	short new_row = cursor_pos.Y;
	short new_col = cursor_pos.X;
	if (off > 0) {
		short n_until_first_wrap = width - cursor_pos.X;
		short remaining = std::max(0, off - n_until_first_wrap);
		if (remaining > width) {
			new_row = cursor_pos.Y + remaining / width;
			new_col = remaining % width;
		}
		else {
			new_col = cursor_pos.X + off;
		}
	}
	else if (off < 0) {
		off = std::abs(off);
		const short n_until_first_wrap = cursor_pos.X;
		const short remaining = (off > n_until_first_wrap) ? (off - n_until_first_wrap) : 0;
		if (remaining > 0) {
			new_row = cursor_pos.Y - std::max(1, remaining / width);
			new_col = width - (remaining % width) + 1;
		}
		else {
			new_col = cursor_pos.X - off;
		}
	}

	cursor_pos.X = new_col;
	cursor_pos.Y = new_row;

	SetConsoleCursorPosition(console, cursor_pos);
}

void Interactive::scroll(short row) {
	GetConsoleScreenBufferInfo(console, &coninfo);

	SMALL_RECT r;
	r = coninfo.srWindow;
	const short height = r.Bottom - r.Top;
	r.Bottom = row;
	r.Top = r.Bottom - height;

	SetConsoleWindowInfo(console, true, &r);
}

void Interactive::reprintLn(std::size_t from_idx, std::size_t to_idx) {
	COORD start_pos = lnBufIndexToCursorPos(from_idx);
	SetConsoleCursorPosition(console, start_pos);

	for (int i = from_idx; i < to_idx; i++)
		std::cout << ln_buf[i];

	int padding = to_idx - ln_buf.length() + 1;
	for (int i = 0; i <= padding; i++)
		std::cout << ' ';

	// Restore the original position of the carriage
	SetConsoleCursorPosition(console, cursor_pos);
}

int mainInteractive() {
	Interactive interactive;
	return interactive.run();
}