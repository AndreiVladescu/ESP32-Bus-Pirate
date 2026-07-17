#pragma once

#include <string>
#include <vector>
#include <cctype>
#include <sstream>
#include <algorithm>
#include "Models/ByteCode.h"
#include "Models/Instruction.h"

class InstructionTransformer {
public:
    std::vector<Instruction> transform(const std::string& raw);
    std::vector<ByteCode> transformByteCode(const Instruction& instruction);
    std::vector<ByteCode> transformByteCodes(const std::vector<Instruction>& instructions);
    bool isInstructionCommand(const std::string& raw) const;

private:
    bool isHex(const std::string& token) const;
    bool isDecimal(const std::string& token) const;
    bool isCharLiteral(const std::string& token) const;
    bool isSymbol(const std::string& token) const;
    bool isRepeatedSymbol(const std::string& token) const;
    bool isSymbolWithRepeat(const std::string& token) const;
    bool isStringLiteral(const std::string& token) const;
    uint8_t parseHex(const std::string& token) const;
    uint8_t parseDecimal(const std::string& token) const;
    uint8_t parseCharLiteral(const std::string& token) const;
    ByteCode parseSymbol(char c) const;
    std::pair<ByteCodeEnum, uint8_t> parseSymbolWithRepeat(const std::string& token) const;
};
