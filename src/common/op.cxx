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

#include <iomanip>
#include <sstream>

#include "op.hxx"

using namespace SoloMIPS;

InvalidOPException::InvalidOPException()
    : _msg("Invalid instruction") {}

InvalidOPException::InvalidOPException(const std::string &msg)
    : _msg(msg) {}

const char *InvalidOPException::what() const noexcept
{
    return this->_msg.data();
}


OP::OP() : opcode(Opcode::SPECIAL), rs(0), rt(0), rd(0), shamt(0), funct(Funct::SLL), imm(0), addr(0) {}

OP::OP(uint32_t word)
{
    this->decode(word);
}

void OP::disassemble(const uint8_t *data, uint32_t size, std::ostream &out)
{
    std::ios::fmtflags flags = out.flags();
    out.unsetf(std::ios::binary);
    OP op;
    for (uint32_t i = 0; i+3 < size; i += 4) {
        try {
            op.decode(&data[i]);
            out << op << std::endl;
        }
        catch (InvalidOPException &) {
            std::stringstream str;
            str << "Invalid instruction at offset 0x" << std::setfill('0') << std::setw(4) << std::hex << i;
            throw InvalidOPException(str.str());
        }
    }
    out.flags(flags);
}

OP OP::ADDIU(uint8_t rt, uint8_t rs, uint16_t imm)
{
    OP op;
    op.opcode = Opcode::ADDIU;
    op.rs = rs;
    op.rt = rt;
    op.imm = imm;
    return op;
}

OP OP::LUI(uint8_t rt, uint16_t imm)
{
    OP op;
    op.opcode = Opcode::LUI;
    op.rt = rt;
    op.imm = imm;
    return op;
}

OP OP::SW(uint8_t rt, int16_t offset, uint8_t base)
{
    OP op;
    op.opcode = Opcode::SW;
    op.rt = rt;
    op.rs = base;
    op.simm = offset;
    return op;
}

OP OP::OR(uint8_t rd, uint8_t rs, uint8_t rt)
{
    OP op;
    op.opcode = Opcode::SPECIAL;
    op.funct = Funct::OR;
    op.rs = rs;
    op.rt = rt;
    op.rd = rd;
    return op;
}

OP OP::JR(uint8_t rs)
{
    OP op;
    op.opcode = Opcode::SPECIAL;
    op.funct = Funct::JR;
    op.rs = rs;
    return op;
}

OP OP::JAL(uint32_t addr)
{
    OP op;
    op.opcode = Opcode::JAL;
    op.addr = addr & 0x3ffffff;
    return op;
}

void OP::decode(const uint8_t *p)
{
    this->decode((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

void OP::decode(uint32_t word)
{
    unsigned int op = (word >> 26);
    if ((op >= 16 && op <= 31)
            || op == 34 || op == 38 || op == 39 || op == 42
            || op >= 44)
        throw InvalidOPException();
    this->opcode = static_cast<Opcode>(op);

    if (this->opcode == Opcode::SPECIAL) { // R-Type
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
    if (this->opcode == Opcode::SPECIAL) { // R-Type
        return ((this->rs & 0x1f) << 21)
            | ((this->rt & 0x1f) << 16)
            | ((this->rd & 0x1f) << 11)
            | ((this->shamt & 0x1f) << 6)
            | (f & 0x3f);
    }
    else if (this->opcode == Opcode::J || this->opcode == Opcode::JAL) { // J-Type
        return (op << 26)
            | (this->addr & 0x3ffffff);
    }
    else { // I-Type
        return (op << 26)
            | ((this->rs & 0x1f) << 21)
            | ((this->rt & 0x1f) << 16)
            | this->imm;
    }
}

void OP::encode(uint8_t *p) const
{
    uint32_t word = this->encode();
    p[0] = word >> 24;
    p[1] = (word >> 16) & 0xff;
    p[2] = (word >> 8) & 0xff;
    p[3] = word & 0xff;
}

std::ostream &operator<<(std::ostream &out, const OP &op)
{
    if (out.flags() & std::ios::binary) {
        uint32_t word = op.encode();
        out
            << static_cast<uint8_t>(word >> 24)
            << static_cast<uint8_t>((word >> 16) & 0xff)
            << static_cast<uint8_t>((word >> 8) & 0xff)
            << static_cast<uint8_t>(word & 0xff);
        return out;
    }

    // Names
    switch (op.opcode) {
        case Opcode::ADDI:  out << "addi   "; break;
        case Opcode::ADDIU: out << "addiu  "; break;
        case Opcode::ANDI:  out << "andi   "; break;
        case Opcode::BEQ:   out << "beq    "; break;
        case Opcode::BGTZ:  out << "bgtz   "; break;
        case Opcode::BLEZ:  out << "blez   "; break;
        case Opcode::BNE:   out << "bne    "; break;
        case Opcode::REGIMM:
            switch (op.rt) {
                case 0b00000: out << "bltz   "; break;
                case 0b00001: out << "bgez   "; break;
                case 0b10000: out << "bltzal "; break;
                case 0b10001: out << "bgezal "; break;
                default: throw InvalidOPException();
            }
            break;
        case Opcode::J:     out << "j      "; break;
        case Opcode::JAL:   out << "jal    "; break;
        case Opcode::LB:    out << "lb     "; break;
        case Opcode::LBU:   out << "lbu    "; break;
        case Opcode::LH:    out << "lh     "; break;
        case Opcode::LHU:   out << "lhu    "; break;
        case Opcode::LUI:   out << "lui    "; break;
        case Opcode::LW:    out << "lw     "; break;
        case Opcode::ORI:   out << "ori    "; break;
        case Opcode::SB:    out << "sb     "; break;
        case Opcode::SH:    out << "sh     "; break;
        case Opcode::SLTI:  out << "slti   "; break;
        case Opcode::SLTIU: out << "sltiu  "; break;
        case Opcode::SPECIAL:
            switch (op.funct) {
                case Funct::ADD:     out << "add    "; break;
                case Funct::ADDU:    out << "addu   "; break;
                case Funct::AND:     out << "and    "; break;
                case Funct::DIV:     out << "div    "; break;
                case Funct::DIVU:    out << "divu   "; break;
                case Funct::JALR:    out << "jalr   "; break;
                case Funct::JR:      out << "jr     "; break;
                case Funct::MFHI:    out << "mfhi   "; break;
                case Funct::MFLO:    out << "mflo   "; break;
                case Funct::MTHI:    out << "mthi   "; break;
                case Funct::MTLO:    out << "mtlo   "; break;
                case Funct::MULT:    out << "mult   "; break;
                case Funct::MULTU:   out << "multu  "; break;
                case Funct::NOR:     out << "nor    "; break;
                case Funct::OR:      out << "or     "; break;
                case Funct::SLL:
                    if (op.rd == 0 && op.rt == 0 && op.shamt == 0)
                        out << "nop";
                    else
                        out << "sll    ";
                    break;
                case Funct::SLLV:    out << "sllv   "; break;
                case Funct::SLT:     out << "slt    "; break;
                case Funct::SLTU:    out << "sltu   "; break;
                case Funct::SRA:     out << "sra    "; break;
                case Funct::SRAV:    out << "srav   "; break;
                case Funct::SRL:     out << "srl    "; break;
                case Funct::SRLV:    out << "srlv   "; break;
                case Funct::SUB:     out << "sub    "; break;
                case Funct::SUBU:    out << "subu   "; break;
                case Funct::SYSCALL: out << "syscall"; break;
                case Funct::XOR:     out << "xor    "; break;
            }
            break;
        case Opcode::SW:   out << "sw     "; break;
        case Opcode::XORI: out << "xori   "; break;
    }

    // Arguments
    switch (op.opcode) {
        case Opcode::ADDI:
        case Opcode::BEQ:
        case Opcode::BNE:
            out << " r" << static_cast<uint16_t>(op.rs) << ", r" << static_cast<uint16_t>(op.rt) << ", " << op.simm;
            break;
        case Opcode::ADDIU:
        case Opcode::ANDI:
        case Opcode::ORI:
        case Opcode::XORI:
            out << " r" << static_cast<uint16_t>(op.rt) << ", r" << static_cast<uint16_t>(op.rs) << ", " << op.imm;
            break;
        case Opcode::LB:
        case Opcode::LBU:
        case Opcode::LH:
        case Opcode::LHU:
        case Opcode::LW:
        case Opcode::SB:
        case Opcode::SH:
        case Opcode::SW:
            out << " r" << static_cast<uint16_t>(op.rt) << ", " << op.simm << "(r" << static_cast<uint16_t>(op.rs) << ")";
            break;
        case Opcode::LUI:
            out << " r" << static_cast<uint16_t>(op.rt) << ", " << op.imm;
            break;
        case Opcode::BGTZ:
        case Opcode::BLEZ:
        case Opcode::REGIMM:
            out << " r" << static_cast<uint16_t>(op.rs) << ", " << op.simm;
            break;
        case Opcode::J:
        case Opcode::JAL: {
            std::stringstream str;
            str << " 0x" << std::setfill('0') << std::setw(8) << std::hex << op.addr;
            out << str.str();
            break;
        }
        case Opcode::SLTI:
            out << " r" << static_cast<uint16_t>(op.rs) << ", r" << static_cast<uint16_t>(op.rt) << ", " << op.simm;
            break;
        case Opcode::SLTIU:
            out << " r" << static_cast<uint16_t>(op.rs) << ", r" << static_cast<uint16_t>(op.rt) << ", " << op.imm;
            break;
        case Opcode::SPECIAL:
            switch (op.funct) {
                case Funct::ADD:
                case Funct::ADDU:
                case Funct::AND:
                case Funct::NOR:
                case Funct::OR:
                case Funct::SLT:
                case Funct::SLTU:
                case Funct::SUB:
                case Funct::SUBU:
                case Funct::XOR:
                    out << " r" << static_cast<uint16_t>(op.rd) << ", r" << static_cast<uint16_t>(op.rs) << ", r" << static_cast<uint16_t>(op.rt);
                    break;
                case Funct::DIV:
                case Funct::DIVU:
                case Funct::MULT:
                case Funct::MULTU:
                    out << " r" << static_cast<uint16_t>(op.rs) << ", r" << static_cast<uint16_t>(op.rt);
                    break;
                case Funct::SLL:
                    if (op.rd == 0 && op.rt == 0 && op.shamt == 0)
                        break;
                case Funct::SLLV:
                case Funct::SRA:
                case Funct::SRAV:
                case Funct::SRL:
                case Funct::SRLV:
                    out << " r" << static_cast<uint16_t>(op.rd) << ", r" << static_cast<uint16_t>(op.rt) << ", " << static_cast<uint16_t>(op.shamt);
                    break;
                case Funct::JALR:
                case Funct::JR:
                    out << " r" << static_cast<uint16_t>(op.rs);
                    break;
                case Funct::MFHI:
                case Funct::MFLO:
                    out << " r" << static_cast<uint16_t>(op.rd);
                    break;
                case Funct::MTHI:
                case Funct::MTLO:
                    out << " r" << static_cast<uint16_t>(op.rs);
                    break;
                case Funct::SYSCALL:
                    break;
            }
            break;
    }

    return out;
}
