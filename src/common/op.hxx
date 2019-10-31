/*
 *  op.hxx
 *
 *  Copyright (C) 2019  Patrick "p2k" Schneider
 *
 *  This file is part of SoloMIPS.
 *
 *  SoloMIPS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SoloMIPS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SoloMIPS.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HEADER_SOLOMIPS_OP_HXX
#define HEADER_SOLOMIPS_OP_HXX

#include <cstdint>
#include <string>
#include <ostream>
#include <exception>

namespace SoloMIPS {

/*
Decoder and container for R2000 instructions. Will throw an execption if the
opcode is invalid.
*/

class InvalidOPException : public std::exception
{
public:
    InvalidOPException();
    explicit InvalidOPException(const std::string &msg);
    const char *what() const noexcept;

private:
    std::string _msg;
};

enum class Opcode : unsigned int
{
    SPECIAL = 0b000000,
    REGIMM = 0b000001,

    J     = 0b000010,
    JAL   = 0b000011,
    BEQ   = 0b000100,
    BNE   = 0b000101,
    BLEZ  = 0b000110,
    BGTZ  = 0b000111,
    
    ADDI  = 0b001000,
    ADDIU = 0b001001,
    SLTI  = 0b001010,
    SLTIU = 0b001011,
    ANDI  = 0b001100,
    ORI   = 0b001101,
    XORI  = 0b001110,

    LUI   = 0b001111,

    LB    = 0b100000,
    LH    = 0b100001,
    LW    = 0b100011,
    LBU   = 0b100100,
    LHU   = 0b100101,

    SB    = 0b101000,
    SH    = 0b101001,
    SW    = 0b101011,
};

enum class Funct : unsigned int
{
    SLL  = 0b000000,
    SRL  = 0b000010,
    SRA  = 0b000011,
    SLLV = 0b000100,
    SRLV = 0b000110,
    SRAV = 0b000111,

    JR   = 0b001000,
    JALR = 0b001001,

    SYSCALL = 0b001100,

    MFHI = 0b010000,
    MTHI = 0b010001,
    MFLO = 0b010010,
    MTLO = 0b010011,
    MULT = 0b011000,
    MULTU= 0b011001,
    DIV  = 0b011010,
    DIVU = 0b011011,

    ADD  = 0b100000,
    ADDU = 0b100001,
    SUB  = 0b100010,
    SUBU = 0b100011,
    AND  = 0b100100,
    OR   = 0b100101,
    XOR  = 0b100110,
    NOR  = 0b100111,

    SLT  = 0b101010,
    SLTU = 0b101011,
};

struct OP
{
    OP();
    explicit OP(uint32_t word);

    static void disassemble(const uint8_t *data, uint32_t size, std::ostream &out);

    void decode(const uint8_t *p);
    void decode(uint32_t word);
    uint32_t encode() const;
    void encode(uint8_t *p) const;

    Opcode opcode;

    uint8_t rs;
    uint8_t rt;

    uint8_t rd;
    uint8_t shamt;
    Funct funct;

    union {
        uint16_t imm;
        int16_t simm;
    };

    uint32_t addr;
};

}

std::ostream &operator<<(std::ostream &out, const SoloMIPS::OP &op);

#endif /* HEADER_SOLOMIPS_OP_HXX */
