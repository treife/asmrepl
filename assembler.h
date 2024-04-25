#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <cstdint>
#include <vector>
#include <string>
//#include <expected>

class Assembler {
public:
	Assembler();

	bool assembleBuffer(const std::string& buf, std::vector<std::uint8_t>& out,
	                    std::string& error, bool long_mode=true);
	bool assembleFile(const std::string& filename, std::vector<std::uint8_t>& out,
	                  std::string& error);

private:
	bool assembleFileImpl(const std::string& filename, const std::string& output_filename,
                          std::string& output_stderr, std::string& error);

	std::string nasm_path;
};
#endif
