#include <iostream>
#include <cstdint>
#include <bitset>
#include <iomanip>
#include <memory>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>

#include "internals.h"
#include "config.h"

class Core {
public:
    std::shared_ptr<REGISTERS> regs;
    std::shared_ptr<RAM> ram;
    std::shared_ptr<ALU> alu;
    std::shared_ptr<OPCODE> opcode;

    // for debugging
    std::shared_ptr<InstructionLookup> lut;

    Core() {
        ram = std::make_shared<RAM>();
        alu = std::make_shared<ALU>();
        regs = std::make_shared<REGISTERS>();
        opcode = std::make_shared<OPCODE>(0, 0, 0, 0);
        lut = std::make_shared<InstructionLookup>();
        
    }
    int run() {
        auto err = boot(code_file);
        if (err != 0){
            return err;
        }
        
        while(true) {
            disassembly();
            std::cout << lut->getMnemonic(opcode->mnemonic) << std::endl;
            executeInstruction();
            if (opcode->mnemonic == 0b1111){
                std::cout << "END EXCEUTION" << std::endl;
                break;
            }
            incrementPC();
        }
        printRegs();
        return 0;
    }
    
    int boot(const std::string& filename) {
        std::ifstream file(filename);
        std::vector<uint16_t> data;

        if (!file.is_open()) {
            std::cerr << "Unable to open file: " << filename << std::endl;
            return 1;
        }

        std::string line;
        uint16_t count = 0;
        while (std::getline(file, line)) {
            size_t commentPos = line.find(';');
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }
            line.erase(0, line.find_first_not_of(' '));
            line.erase(line.find_last_not_of(' ') + 1);

            if (line.size() != 16) {
                if (line.empty()) {
                    continue;
                }
                std::cerr << "Invalid line length: " << line.size() << " in line: " << line << std::endl;
                return 1;
            }

            for (char c : line) {
                if (c != '0' && c != '1') {
                    std::cerr << "Invalid character in line: " << line << std::endl;
                    return 1;
                }
            }

            uint16_t value = std::bitset<16>(line).to_ulong();
            auto err = ram->write(count, value);
            if (err != 0) {
                return err;
            }
            count++;
        }

        file.close();
        
        setPC();
        setStackPointer();

        return 0;
    }

    void setPC() {
        regs->write_reg(PC_REG, 0);
    }

    void setStackPointer() {
        int ram_size = ram->get_size();
        regs->write_reg(ST_REG, (uint16_t) ram_size-1);
    }

    void incrementPC(){
        uint16_t PC = regs->read_reg(PC_REG);
        regs->write_reg(PC_REG, PC + 1);
    }
    
    uint16_t loadInstruction(){
        uint16_t PC = regs->read_reg(PC_REG);
        auto instr = ram->read(uint32_t(PC));
        return instr;
    }

    void disassembly(){
        uint16_t instr = loadInstruction();
        opcode->mnemonic = (uint8_t) ((instr>>12) & 0b1111);
        opcode->dest = (uint8_t) ((instr>>8) & 0b1111);
        opcode->reg1 = (uint8_t) ((instr>>4) & 0b1111);
        opcode->reg2 = (uint8_t) instr & 0b1111;
    }

    int executeInstruction() {   // uint8_t opcode, uint8_t dest, uint8_t reg1, uint8_t reg2
        uint16_t res;
        switch (opcode->mnemonic) {
        case 0b0000: { // add
            uint16_t res = alu->add(regs->read_reg(opcode->reg1), regs->read_reg(opcode->reg2));
            regs->write_reg(opcode->dest, res);
            break;}
        case 0b0001:{ // store
            uint16_t value = regs->read_reg(opcode->dest);
            uint16_t addr1 = regs->read_reg(opcode->reg1);
            uint16_t offset = regs->read_reg(opcode->reg2);
            uint16_t address = addr1 + offset;
            auto err = ram->write(address, value);
            return err;
            break; }
        case 0b0010:{ // sub
            res = alu->sub(regs->read_reg(opcode->reg1), regs->read_reg(opcode->reg2));
            regs->write_reg(opcode->dest, res);
            break;}
        case 0b0011: {  // and
            res = alu->andbit(regs->read_reg(opcode->reg1), regs->read_reg(opcode->reg2));
            regs->write_reg(opcode->dest, res);
            break; }
        case 0b0100: { // not
            res = alu->notbit(regs->read_reg(opcode->reg1), regs->read_reg(opcode->reg2));
            regs->write_reg(opcode->dest, res);
            break; }
        case 0b0101: { // or 
            res = alu->orbit(regs->read_reg(opcode->reg1), regs->read_reg(opcode->reg2));
            regs->write_reg(opcode->dest, res);
            break; }
        case 0b0110: { // mul
            res = alu->mul(regs->read_reg(opcode->reg1), regs->read_reg(opcode->reg2));
            regs->write_reg(opcode->dest, res);
            break; }
        case 0b0111: { // lsl
            res = alu->lsl(regs->read_reg(opcode->reg1), opcode->reg2);
            regs->write_reg(opcode->dest, res);
            break; }
        case 0b1000:{ // lsr
            res = alu->lsr(regs->read_reg(opcode->reg1), opcode->reg2);
            regs->write_reg(opcode->dest, res);
            break; }
        case 0b1001: {  // load from RAM
            uint16_t addr1 = regs->read_reg(opcode->reg1);
            uint16_t addr2 = (uint16_t) opcode->reg2;
            uint16_t address = addr1 + addr2;
            uint16_t value = ram->read(address);
            regs->write_reg(opcode->dest, (uint16_t) value);
            break; }
        case 0b1010: { // addi
            uint16_t dest = regs->read_reg(opcode->dest);
            uint8_t reg1_shifted = opcode->reg1 << 4;
            uint8_t reg2 = opcode->reg2;
            int16_t combined_val = (int16_t) (static_cast<int8_t>(reg1_shifted | reg2));
            uint16_t result = alu->add(dest, static_cast<uint16_t>(combined_val));
            regs->write_reg(opcode->dest, result);
            break; }
        case 0b1011:{  // BRA
            uint16_t PC = regs->read_reg(PC_REG);
            uint16_t val1 = (static_cast<uint16_t>(opcode->dest) & 0x0F) << 8; // Lower 4 bits of dest, shifted left by 8
            uint16_t val2 = (static_cast<uint16_t>(opcode->reg1) & 0x0F) << 4; // Lower 4 bits of reg1, shifted left by 4
            uint16_t val3 = static_cast<uint16_t>(opcode->reg2) & 0x0F;        // Lower 4 bits of reg2
            uint16_t combinedVal = val1 | val2 | val3;
            int16_t signedVal = combinedVal;
            if (combinedVal & 0x800) {
                signedVal |= 0xF000;
            }
            uint16_t val = (uint16_t) (signedVal + (int16_t) PC - 1);
            regs->write_reg(PC_REG, val);
            break;}
        case 0b1100:{ // BEQ
            uint16_t PC = regs->read_reg(PC_REG);
            uint16_t cond = regs->read_reg(opcode->dest);
            uint8_t val2 =  opcode->reg1 << 4;
            uint8_t val3 = opcode->reg2;
            int16_t bra_val = (int16_t) ((int8_t) (val2 | val3));
            uint16_t val = (uint16_t) (bra_val + (int16_t) PC - 1);
            if (cond == 0) {
                regs->write_reg(PC_REG, val);
            }
            break;}
        case 0b1101:{ // BNE
            uint16_t PC = regs->read_reg(PC_REG);
            uint16_t cond = regs->read_reg(opcode->dest);
            uint8_t val2 =  opcode->reg1 << 4;
            uint8_t val3 = opcode->reg2;
            int16_t bra_val = (int16_t) ((int8_t) (val2 | val3));
            uint16_t val = (uint16_t) (bra_val + (int16_t) PC - 1);
            
            if (cond != 0) {
                regs->write_reg(PC_REG, val);
            }
            break;}
        case 0b1110:{  // JMP
            uint16_t val1 = regs->read_reg(opcode->dest);
            uint16_t val2 = ((uint16_t) opcode->reg1) << 4;
            uint16_t val3 = ((uint16_t) opcode->reg2);
            uint16_t val = val1 | val2 | val3;
            regs->write_reg(PC_REG, val-1);
            break;}
        case 0b1111: { // HALT
            uint16_t PC = regs->read_reg(PC_REG);
            regs->write_reg(PC_REG, PC - 1);
            break; }
        default:
            break;
        }
        return 0;
    }

    void printMem(uint16_t start_adr, uint16_t num){
        for (uint16_t i = start_adr; i < start_adr+num; i++){
            std::cout << "ADDR " << std::bitset<16>(i) << ": "<< std::bitset<16>(ram->read(i)) << std::endl;
        }
    }
    void printRegs(){
        for (uint8_t i = 0; i < 16; i++) {
            std::cout << "REG " << std::setw(2) << std::setfill('0') << int(i) << ": "<<std::bitset<16>(regs->read_reg(i)) << std::endl;
        }
    }

    void printOpcode() {
        std::cout << "MNEM " << ": "<<std::bitset<8>(opcode->mnemonic);
        std::cout << " " << lut->getMnemonic(opcode->mnemonic) << std::endl;
        std::cout << "DEST " << ": "<<std::bitset<8>(opcode->dest) << std::endl;
        std::cout << "REG1 " << ": "<<std::bitset<8>(opcode->reg1) << std::endl;
        std::cout << "REG2 " << ": "<<std::bitset<8>(opcode->reg2) << std::endl;
    }
};

int main() {
    Core core;
    int err = core.run();
    return 0;
}
