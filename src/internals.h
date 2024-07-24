#ifndef INTERNAL_H_
#define INTERNAL_H_

#include <cstdint>
#include <iostream>
#include <bitset>


struct OPCODE {
    uint8_t mnemonic;
    uint8_t dest;
    uint8_t reg1;
    uint8_t reg2;

    OPCODE(uint8_t mne, uint8_t d, uint8_t r1, uint8_t r2)
        : mnemonic(mne), dest(d), reg1(r1), reg2(r2) {}
};

class REGISTERS {
public:
    uint16_t regs[16];
    void write_reg(uint8_t idx, uint16_t val) {
        if (idx != 0) {
            regs[idx] = val;
        }
    }
    uint16_t read_reg(uint8_t idx){
        return regs[idx];
    }
    REGISTERS(){
        for (int i = 0; i < 16; ++i) {
            regs[i] = (uint16_t) 0;
        }
    }
};

class ALU {
public:
    uint16_t add(uint16_t reg1, uint16_t reg2) {
        return (uint16_t)((int16_t) reg1 + (int16_t) reg2);
    }

    uint16_t sub(uint16_t reg1, uint16_t reg2) {
        return (uint16_t)((int16_t) reg1 - (int16_t) reg2);
    }
    uint16_t mul(uint16_t reg1, uint16_t reg2) {
        return (uint16_t)((int16_t) reg1 * (int16_t) reg2);
    }
    uint16_t andbit(uint16_t reg1, uint16_t reg2) {
        return reg1 & reg2;
    }
    uint16_t notbit(uint16_t reg1, uint16_t reg2) {
        return ~reg1;
    }
    uint16_t orbit(uint16_t reg1, uint16_t reg2) {
        return reg1 | reg2;
    }
    uint16_t lsl(uint16_t reg1, uint8_t reg2) {
        return reg1<<reg2;
    }
    uint16_t lsr(uint16_t reg1, uint8_t reg2) {
        return reg1>>reg2;
    }
};

class RAM {
public:
    RAM() {
        memory = new uint16_t[size];
    }

    ~RAM() {
        delete[] memory;
    }

    int write(uint16_t address, uint16_t value) {
        if (address < size) {
            memory[address] = value;
        } else {
            std::cerr << "Error: Address out of bounds." << std::endl;
            return 1;
        }
        return 0;
    }

    uint16_t read(uint16_t address) const {
        if (address < size) {
            return memory[address];
        } else {
            return 0;
        }
    }

    int get_size() {
        return size;
    }

private:
    uint16_t* memory;
    int size = 65536;
};


std::string uint16ToBinaryString(uint16_t value) {
    std::bitset<16> bits(value);
    return bits.to_string();
}

#endif
