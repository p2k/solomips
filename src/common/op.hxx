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

#ifndef HEADER_SOLOMIPS_OP_H
#define HEADER_SOLOMIPS_OP_H

#include <cstdint>
#include <exception>

namespace SoloMIPS {

struct RAMPointer;

/*
Decoder and container for R2000 instructions. Will throw an execption if the
opcode is invalid.
*/

struct InvalidOPException : public std::exception
{
    const char *what() const noexcept
    {
        return "Invalid instruction";
    }
};

enum class Opcode : unsigned int
{
    REG   = 0b000000,

    BGTLT = 0b000001,

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

    MTFC0 = 0b010000,

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

    void decode(uint32_t word);
    OP &operator=(const RAMPointer &p);
    uint32_t encode() const;

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

#endif /* HEADER_SOLOMIPS_OP_H */
