#ifndef CONFIG_H
#define CONIFG_H

#include <string>
#include <cstdint>
#include <unordered_map>

uint8_t PC_REG = 15;
uint8_t ST_REG = 14;
std::string code_file = "stack.txt";

class InstructionLookup {
    public:
        InstructionLookup() {
            lookupTable = {
            {0x00, "ADD"},
            {0x01, "STR"},
            {0x02, "SUB"},
            {0x03, "AND"},
            {0x04, "NOT"},
            {0x05, "OR"},
            {0x06, "MUL"},
            {0x07, "LSL"},
            {0x08, "LSR"},
            {0x09, "LDM"},
            {0x0A, "ADDI"},
            {0x0B, "BRA"},
            {0x0C, "BEQ"},
            {0x0D, "BNE"},
            {0x0E, "JMP"},
            {0x0F, "HLT"}
        };
        }
        std::string getMnemonic(uint8_t opcode) {
            auto it = lookupTable.find(opcode);
            if (it != lookupTable.end()) {
                return it->second;
            } else {
                return "UNKNOWN";
            }
        }

    private:
        std::unordered_map<uint8_t, std::string> lookupTable;
};

#endif
