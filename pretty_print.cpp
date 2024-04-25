#include "pretty_print.h"
#include "zombie.h"

#include <string>
#include <iomanip>
#include <iostream>
#include <format>

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

std::ostream& reset(std::ostream& os) {
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	static WORD initial = 0;
	if (!initial) {
		CONSOLE_SCREEN_BUFFER_INFO coninfo;
		GetConsoleScreenBufferInfo(console, &coninfo);
		initial = coninfo.wAttributes;
	}
	else {
		SetConsoleTextAttribute(console, initial);
	}
	return os;
}

std::ostream& operator <<(std::ostream& os, Color color) {
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD w_color = to_underlying(color);

	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	GetConsoleScreenBufferInfo(console, &coninfo);

	WORD new_attribs = coninfo.wAttributes;
	if ((w_color & 0xf) != 0)
		new_attribs = (new_attribs & 0xfff0) | (w_color & 0xf);
	if ((w_color & 0xf0) != 0)
		new_attribs = (new_attribs & 0xff0f) | (w_color & 0xf0);

	SetConsoleTextAttribute(console, w_color);
	return os;
}

void printZombieInfo(const Zombie& zombie) {
	std::cout << std::format("Process {:x} ({} [{}])\n",
	                         zombie.getProcId(), zombie.getShellcodeSpace(),
	                         zombie.getShellcodeSpaceSize());
}

void printExceptionRecord(const EXCEPTION_RECORD& e) {
	const char* code;
	switch (e.ExceptionCode) {
	case EXCEPTION_ACCESS_VIOLATION: code = "EXCEPTION_ACCESS_VIOLATION"; break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: code = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"; break;
	case EXCEPTION_BREAKPOINT: code = "EXCEPTION_BREAKPOINT"; break;
	case EXCEPTION_DATATYPE_MISALIGNMENT: code = "EXCEPTION_DATATYPE_MISALIGNMENT"; break;
	case EXCEPTION_FLT_DENORMAL_OPERAND: code = "EXCEPTION_FLT_DENORMAL_OPERAND"; break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO: code = "EXCEPTION_FLT_DIVIDE_BY_ZERO"; break;
	case EXCEPTION_FLT_INEXACT_RESULT: code = "EXCEPTION_FLT_INEXACT_RESULT"; break;
	case EXCEPTION_FLT_INVALID_OPERATION: code = "EXCEPTION_FLT_INVALID_OPERATION"; break;
	case EXCEPTION_FLT_OVERFLOW: code = "EXCEPTION_FLT_OVERFLOW"; break;
	case EXCEPTION_FLT_STACK_CHECK: code = "EXCEPTION_FLT_STACK_CHECK"; break;
	case EXCEPTION_FLT_UNDERFLOW: code = "EXCEPTION_FLT_UNDERFLOW"; break;
	case EXCEPTION_ILLEGAL_INSTRUCTION: code = "EXCEPTION_ILLEGAL_INSTRUCTION"; break;
	case EXCEPTION_IN_PAGE_ERROR: code = "EXCEPTION_IN_PAGE_ERROR"; break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO: code = "EXCEPTION_INT_DIVIDE_BY_ZERO"; break;
	case EXCEPTION_INT_OVERFLOW: code = "EXCEPTION_INT_OVERFLOW"; break;
	case EXCEPTION_INVALID_DISPOSITION: code = "EXCEPTION_INVALID_DISPOSITION"; break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: code = "EXCEPTION_NONCONTINUABLE_EXCEPTION"; break;
	case EXCEPTION_PRIV_INSTRUCTION: code = "EXCEPTION_PRIV_INSTRUCTION"; break;
	case EXCEPTION_SINGLE_STEP: code = "EXCEPTION_SINGLE_STEP"; break;
	case EXCEPTION_STACK_OVERFLOW: code = "EXCEPTION_STACK_OVERFLOW"; break;
	default: code = "unknown"; break;
	}
	std::cout << "Code: " << code << '\n';

	std::string flags;
	if (e.ExceptionFlags & EXCEPTION_NONCONTINUABLE)
		std::cout << Color::FG_RED << "Flags: Noncontinuable\n" << reset;
}

#define IS_DIFFERENT(reg) (!last_c || (last_c && last_c->reg != c.reg))
void printRegisters(const CONTEXT& c, const CONTEXT* last_c) {
	std::cout << std::hex;
	if (IS_DIFFERENT(Rax)) std::cout << Color::FG_GREEN << "Rax" << reset << " = 0x" << c.Rax << '\n';
	if (IS_DIFFERENT(Rcx)) std::cout << Color::FG_GREEN << "Rcx" << reset << " = 0x" << c.Rcx << '\n';
	if (IS_DIFFERENT(Rdx)) std::cout << Color::FG_GREEN << "Rdx" << reset << " = 0x" << c.Rdx << '\n';
	if (IS_DIFFERENT(Rbx)) std::cout << Color::FG_GREEN << "Rbx" << reset << " = 0x" << c.Rbx << '\n';
	if (IS_DIFFERENT(Rsp)) std::cout << Color::FG_GREEN << "Rsp" << reset << " = 0x" << c.Rsp << '\n';
	if (IS_DIFFERENT(Rbp)) std::cout << Color::FG_GREEN << "Rbp" << reset << " = 0x" << c.Rbp << '\n';
	if (IS_DIFFERENT(Rsi)) std::cout << Color::FG_GREEN << "Rsi" << reset << " = 0x" << c.Rsi << '\n';
	if (IS_DIFFERENT(Rdi)) std::cout << Color::FG_GREEN << "Rdi" << reset << " = 0x" << c.Rdi << '\n';
	if (IS_DIFFERENT(R8))  std::cout << Color::FG_GREEN << "R8 " << reset << " = 0x" << c.R8  << '\n';
	if (IS_DIFFERENT(R9))  std::cout << Color::FG_GREEN << "R9 " << reset << " = 0x" << c.R9  << '\n';
	if (IS_DIFFERENT(R10)) std::cout << Color::FG_GREEN << "R10" << reset << " = 0x" << c.R10 << '\n';
	if (IS_DIFFERENT(R11)) std::cout << Color::FG_GREEN << "R11" << reset << " = 0x" << c.R11 << '\n';
	if (IS_DIFFERENT(R12)) std::cout << Color::FG_GREEN << "R12" << reset << " = 0x" << c.R12 << '\n';
	if (IS_DIFFERENT(R13)) std::cout << Color::FG_GREEN << "R13" << reset << " = 0x" << c.R13 << '\n';
	if (IS_DIFFERENT(R14)) std::cout << Color::FG_GREEN << "R14" << reset << " = 0x" << c.R14 << '\n';
	if (IS_DIFFERENT(R15)) std::cout << Color::FG_GREEN << "R15" << reset << " = 0x" << c.R15 << '\n';
	std::cout << std::dec;

	if (IS_DIFFERENT(EFlags)) {	
		std::cout << Color::FG_GREEN << "ZF" << reset << " = " << (c.EFlags & 0x1) << '\t' <<
		             Color::FG_GREEN << "PF" << reset << " = " << (c.EFlags & 0x2) << '\t' <<
		             Color::FG_GREEN << "AF" << reset << " = " << (c.EFlags & 0x4) << '\t' <<
		             Color::FG_GREEN << "ZF" << reset << " = " << (c.EFlags & 0x6) << '\n' <<
		             Color::FG_GREEN << "SF" << reset << " = " << (c.EFlags & 0x7) << '\t' <<
		             Color::FG_GREEN << "TF" << reset << " = " << (c.EFlags & 0x8) << '\t' <<
		             Color::FG_GREEN << "IF" << reset << " = " << (c.EFlags & 0x9) << '\t' <<
		             Color::FG_GREEN << "DF" << reset << " = " << (c.EFlags & 0xa) << '\n' <<
		             Color::FG_GREEN << "OF" << reset << " = " << (c.EFlags & 0xb) << '\n';
	}
}