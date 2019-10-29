/*
 *  op.cxx
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

#include "op.hxx"
#include "ram.hxx"

using namespace SoloMIPS;

OP::OP() : opcode(Opcode::REG), rs(0), rt(0), rd(0), shamt(0), funct(Funct::SLL), imm(0), addr(0) {}

OP::OP(uint32_t word)
{
    this->decode(word);
}

OP &OP::operator=(const RAMPointer &p)
{
    this->decode(p.instr());
    return *this;
}

void OP::decode(uint32_t word)
{
    unsigned int op = (word >> 26);
    if ((op >= 16 && op <= 31)
            || op == 34 || op == 38 || op == 39 || op == 42
            || op >= 44)
        throw InvalidOPException();
    this->opcode = static_cast<Opcode>(op);

    if (this->opcode == Opcode::REG) { // R-Type
        this->rs = (word >> 21) & 0x1f;
        this->rt = (word >> 16) & 0x1f;
        this->rd = (word >> 11) & 0x1f;
        this->shamt = (word >> 6) & 0x1f;
        unsigned int f = word & 0x3f;
        if (f == 1 || f == 5 || f == 10 || f == 11
                || (f >= 13 && f <= 15)
                || f == 18
                || (f >= 20 && f <= 23)
                || (f >= 28 && f <= 31)
                || f == 40 || f == 41
                || f >= 44)
            throw InvalidOPException();
        this->funct = static_cast<Funct>(f);
        this->imm = 0;
        this->addr = 0;
    }
    else if (this->opcode == Opcode::J || this->opcode == Opcode::JAL) { // J-Type
        this->rs = 0;
        this->rt = 0;
        this->rd = 0;
        this->shamt = 0;
        this->funct = Funct::SLL;
        this->imm = 0;
        this->addr = word & 0x3ffffff;
    }
    else { // I-Type
        this->rs = (word >> 21) & 0x1f;
        this->rt = (word >> 16) & 0x1f;
        this->rd = 0;
        this->shamt = 0;
        this->funct = Funct::SLL;
        this->imm = word & 0xffff;
        this->addr = 0;
    }
}

uint32_t OP::encode() const
{
    unsigned int op = static_cast<unsigned int>(this->opcode);
    unsigned int f = static_cast<unsigned int>(this->funct);
    if (this->opcode == Opcode::REG) { // R-Type
        return ((this->rs & 0x1f) << 21)
            | ((this->rt & 0x1f) << 16)
            | ((this->rd & 0x1f) << 11)
            | ((this->shamt & 0x1f) << 6)
            | (f & 0x3f);
    }
    else if (this->opcode == Opcode::J || this->opcode == Opcode::JAL) { // J-Type
        return (op << 26)
            | ((this->rs & 0x1f) << 21)
            | ((this->rt & 0x1f) << 16)
            | this->imm;
    }
    else { // I-Type
        return (op << 26)
            | (this->addr & 0x3ffffff);
    }
}
