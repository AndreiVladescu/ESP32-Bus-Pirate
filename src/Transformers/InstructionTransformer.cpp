#include "InstructionTransformer.h"

std::vector<Instruction> InstructionTransformer::transform(const std::string& raw) {
    std::vector<Instruction> instructions;
    if (raw.empty()) return instructions;

    bool inChar = false;
    bool inBlock = false;
    bool inString = false;
    std::string token;
    std::string currentRaw;
    Instruction* currentInstruction = nullptr;

    for (char c : raw) {
        if (inChar) {
            token += c;
            currentRaw += c;
            if (c == '\'') {
                if (currentInstruction) {
                    currentInstruction->addToken(token);
                }
                token.clear();
                inChar = false;
            }
            continue;
        }

        if (inString) {
            token += c;
            currentRaw += c;
            if (c == '"') {
                if (currentInstruction) {
                    currentInstruction->addToken(token);
                }
                token.clear();
                inString = false;
                continue;
            }
            continue;
        }


        if (c == '\'') {
            token = "'";
            currentRaw += c;
            inChar = true;
            continue;
        }

        if (c == '"') {
            token = "\"";
            currentRaw += c;
            inString = true;
            continue;
        }

        if (c == '[' || c == '{' || c == '>') {
            inBlock = true;
            currentRaw = std::string(1, c);
            currentInstruction = new Instruction(currentRaw);
            currentInstruction->prefix = c;
            continue;
        }

        if (c == ']') {
            if (!token.empty() && currentInstruction) {
                if (token.back() == ',') token.pop_back();
                currentInstruction->addToken(token);
                token.clear();
            }
            if (currentInstruction) {
                currentRaw += c;
                currentInstruction->raw = currentRaw;
                instructions.push_back(*currentInstruction);
                delete currentInstruction;
                currentInstruction = nullptr;
            }
            inBlock = false;
            continue;
        }

        if (inBlock) {
            currentRaw += c;
            if (std::isspace(c)) {
                if (!token.empty() && currentInstruction) {
                    if (token.back() == ',') token.pop_back();
                    currentInstruction->addToken(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        }
    }

    return instructions;
}

std::vector<ByteCode> InstructionTransformer::transformByteCode(const Instruction& instruction) {
    std::vector<ByteCode> bytecodes;

    if (instruction.prefix == '[') {
        bytecodes.emplace_back(ByteCodeEnum::Start);
    }

    for (const auto& tok : instruction.tokens) {
        if (isCharLiteral(tok)) {
            bytecodes.emplace_back(ByteCodeEnum::Write, parseCharLiteral(tok));
        } 
        
        else if (isStringLiteral(tok)) {
            for (char c : tok.substr(1, tok.size() - 2)) {
                bytecodes.emplace_back(ByteCodeEnum::Write, static_cast<uint8_t>(c));
            }
        } 
        
        else if (isHex(tok)) {
            bytecodes.emplace_back(ByteCodeEnum::Write, parseHex(tok));
        } 
        
        else if (isDecimal(tok)) {
            bytecodes.emplace_back(ByteCodeEnum::Write, parseDecimal(tok));
        } 
        
        else if (isSymbolWithRepeat(tok)) {
            std::pair<ByteCodeEnum, uint8_t> result = parseSymbolWithRepeat(tok);
            ByteCodeEnum sym = result.first;
            uint8_t repeat = result.second;
            bytecodes.emplace_back(sym, 0, 8, repeat);
        }

        else if (isRepeatedSymbol(tok)) {
            char symbol = tok[0];
            uint8_t repeat = tok.size();
            bytecodes.emplace_back(parseSymbol(symbol).getCommand(), 0, 8, repeat);
        }
        
        else if (isSymbol(tok)) {
            bytecodes.push_back(parseSymbol(tok[0]));
        }
    }

    if (instruction.prefix == '[') {
        bytecodes.emplace_back(ByteCodeEnum::Stop);
    }

    return bytecodes;
}

std::vector<ByteCode> InstructionTransformer::transformByteCodes(const std::vector<Instruction>& instructions) {
    std::vector<ByteCode> allByteCodes;

    for (auto& inst : instructions) {
        std::vector<ByteCode> bytecodes = transformByteCode(inst);
        allByteCodes.insert(allByteCodes.end(), bytecodes.begin(), bytecodes.end());
    }

    return allByteCodes;
}

bool InstructionTransformer::isInstructionCommand(const std::string& raw) const {
    if (raw.empty()) return false;

    // Trim
    size_t start = raw.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return false;

    char first = raw[start];
    return (first == '[' || first == '>' || first == '{');
}

bool InstructionTransformer::isHex(const std::string& token) const {
    if (token.size() <= 2 || token[0] != '0' || std::tolower(token[1]) != 'x') {
        return false;
    }

    if (!std::all_of(token.begin() + 2, token.end(),
                     [](unsigned char c) { return std::isxdigit(c); })) {
        return false;
    }

    try {
        return std::stoul(token, nullptr, 16) <= 0xFF;
    } catch (...) {
        return false;
    }
}

bool InstructionTransformer::isDecimal(const std::string& token) const {
    if (token.empty() || !std::all_of(token.begin(), token.end(),
                                     [](unsigned char c) { return std::isdigit(c); })) {
        return false;
    }

    try {
        return std::stoul(token) <= 0xFF;
    } catch (...) {
        return false;
    }
}

bool InstructionTransformer::isCharLiteral(const std::string& token) const {
    return token.size() == 3 && token.front() == '\'' && token.back() == '\'';
}

bool InstructionTransformer::isStringLiteral(const std::string& token) const {
    return token.size() >= 2 &&
           ((token.front() == '"' && token.back() == '"') ||
            (token.front() == '\'' && token.back() == '\''));
}

bool InstructionTransformer::isSymbol(const std::string& token) const {
    if (token.size() != 1) return false;
    char c = token[0];
    switch (c) {
        case 'r':
        case 'd':
        case 'D':
        case 's':
        case 'S':
        case 'h':
        case 'l':
            return true;
        default:
            return std::ispunct(c);
    }
}

bool InstructionTransformer::isSymbolWithRepeat(const std::string& token) const {
    auto pos = token.find(':');
    if (pos == std::string::npos) return false;
    if (pos == 0 || pos == token.size() - 1) return false;

    char symbol = token[0];
    switch (symbol) {
        case 'r':
        case 'd':
        case 'D':
        case 's':
        case 'S':
        case 'h':
        case 'l':
            break;
        default:
            return false;
    }

    std::string repeatStr = token.substr(pos + 1);
    return std::all_of(repeatStr.begin(), repeatStr.end(), ::isdigit);
}


uint8_t InstructionTransformer::parseHex(const std::string& token) const {
    return static_cast<uint8_t>(std::stoul(token, nullptr, 16));
}

uint8_t InstructionTransformer::parseDecimal(const std::string& token) const {
    return static_cast<uint8_t>(std::stoul(token));
}

uint8_t InstructionTransformer::parseCharLiteral(const std::string& token) const {
    return static_cast<uint8_t>(token[1]);
}

ByteCode InstructionTransformer::parseSymbol(char c) const {
    switch (c) {
        case 'r': return ByteCode(ByteCodeEnum::Read, 0, 8, 1);
        case 'd': return ByteCode(ByteCodeEnum::DelayUs, 0, 8, 1);
        case 'D': return ByteCode(ByteCodeEnum::DelayMs, 0, 8, 1);
        case 's': return ByteCode(ByteCodeEnum::Start);
        case 'S': return ByteCode(ByteCodeEnum::Stop);
        case 'h': return ByteCode(ByteCodeEnum::AuxHigh);
        case 'l': return ByteCode(ByteCodeEnum::AuxLow);
        default: return ByteCode(ByteCodeEnum::None);
    }
}

std::pair<ByteCodeEnum, uint8_t> InstructionTransformer::parseSymbolWithRepeat(const std::string& token) const {
    char symbol = token[0];
    size_t colonPos = token.find(':');
    unsigned long parsed = std::stoul(token.substr(colonPos + 1));
    uint8_t repeat = std::min(parsed, 255ul);

    return { parseSymbol(symbol).getCommand(), repeat };
}

bool InstructionTransformer::isRepeatedSymbol(const std::string& token) const {
    if (token.empty()) return false;
    char first = token[0];

    // repeat available on 
    switch (first) {
        case 'r':
        case 'd':
        case 'D':
        case 'h':
        case 'l':
        case 's':
        case 'S':
            break;
        default:
            return false;
    }

    // verify each char
    return std::all_of(token.begin(), token.end(), [first](char c) {
        return c == first;
    });
}
