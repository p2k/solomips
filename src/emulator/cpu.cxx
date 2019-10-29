/*
 *  cpu.cxx
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

#include "cpu.hxx"

using namespace SoloMIPS;

R3000::R3000(uint32_t _entrypoint) : entrypoint(_entrypoint)
{
    this->reset();
}

void R3000::reset()
{
    // Reset registers
    std::memset(this->r, 0, sizeof(uint32_t) * 32);
    this->hi = 0;
    this->lo = 0;
    // Produce NOPs
    this->op.decode(0);
    this->nextOp.decode(0);
    // Set program counter
    this->pc = entrypoint;
    // Clear delayed exception
    this->dex = DelayedException::None;
    this->dexWhat = NULL;
}

// Author's note: Although I prefer to use "this->" everywhere I can, for this
// function I will not use it in order to improve readability.
void R3000::step()
{
    // Raise delayed exceptions
    switch (dex) {
        case DelayedException::None:
            break;
        case DelayedException::MisalignedPCException:
            throw MisalignedPCException();
        case DelayedException::HaltException:
            throw HaltException();
        case DelayedException::InvalidOPException:
            throw InvalidOPException();
        case DelayedException::MemoryException:
            throw MemoryException(dexWhat);
    }

    // Fetch next instruction
    op = nextOp;
    if (pc & 0x03) {
        dex = DelayedException::MisalignedPCException;
    }
    else if (pc == 0) {
        dex = DelayedException::HaltException;
    }
    else {
        try {
            nextOp = ram[pc];
        }
        catch (InvalidOPException &) {
            dex = DelayedException::InvalidOPException;
        }
        catch (MemoryException &e) {
            dex = DelayedException::MemoryException;
            dexWhat = e.what();
        }
        pc += 4;
    }

    // Run instruction
    switch (op.opcode) {
        case Opcode::REG:
            switch (op.funct) {
                case Funct::SLL:
                    r[op.rd] = r[op.rt] << op.shamt;
                    break;
                case Funct::SRL:
                    r[op.rd] = r[op.rt] >> op.shamt;
                    break;
                case Funct::SRA:
                    sr[op.rd] = sr[op.rt] >> op.shamt;
                    break;
                case Funct::SLLV:
                    r[op.rd] = r[op.rt] << r[op.rs];
                    break;
                case Funct::SRLV:
                    r[op.rd] = r[op.rt] >> r[op.rs];
                    break;
                case Funct::SRAV:
                    sr[op.rd] = sr[op.rt] >> r[op.rs];
                    break;
                case Funct::JALR:
                    r[op.rd] = pc+4;
                case Funct::JR:
                    pc = r[op.rs];
                    break;
                case Funct::SYSCALL:
                    throw InvalidOPException();
                    break;
                case Funct::MFHI:
                    r[op.rd] = hi;
                    break;
                case Funct::MTHI:
                    hi = r[op.rs];
                    break;
                case Funct::MFLO:
                    r[op.rd] = lo;
                    break;
                case Funct::MTLO:
                    lo = r[op.rs];
                    break;
                case Funct::MULT: {
                    int64_t prod = sr[op.rs] * sr[op.rt];
                    hi = static_cast<uint64_t>(prod) >> 32;
                    lo = static_cast<uint64_t>(prod) & 0xffffffff;
                    break;
                }
                case Funct::MULTU: {
                    uint64_t prod = r[op.rs] * r[op.rt];
                    hi = prod >> 32;
                    lo = prod & 0xffffffff;
                    break;
                }
                case Funct::DIV: {
                    if (sr[op.rt] == 0)
                        throw ArithmeticException("Divided by zero");
                    hi = static_cast<uint32_t>(sr[op.rs] % sr[op.rt]);
                    lo = static_cast<uint32_t>(sr[op.rs] / sr[op.rt]);
                    break;
                }
                case Funct::DIVU: {
                    if (sr[op.rt] == 0)
                        throw ArithmeticException("Divided by zero");
                    hi = r[op.rs] % r[op.rt];
                    lo = r[op.rs] / r[op.rt];
                    break;
                }
                case Funct::ADD:
                    sr[op.rd] = sr[op.rs] + sr[op.rt];
                    break;
                case Funct::ADDU:
                    r[op.rd] = r[op.rs] + r[op.rt];
                    break;
                case Funct::SUB:
                    sr[op.rd] = sr[op.rs] - sr[op.rt];
                    break;
                case Funct::SUBU:
                    r[op.rd] = r[op.rs] - r[op.rt];
                    break;
                case Funct::AND:
                    r[op.rd] = r[op.rs] & r[op.rt];
                    break;
                case Funct::OR:
                    r[op.rd] = r[op.rs] | r[op.rt];
                    break;
                case Funct::XOR:
                    r[op.rd] = r[op.rs] ^ r[op.rt];
                    break;
                case Funct::NOR:
                    r[op.rd] = ~(r[op.rs] | r[op.rt]);
                    break;
                case Funct::SLT:
                    r[op.rd] = sr[op.rs] < sr[op.rt];
                    break;
                case Funct::SLTU:
                    r[op.rd] = r[op.rs] < r[op.rt];
                    break;
            }
            break;
        case Opcode::BGTLT:
            switch (op.rt) {
                case 0b10000: // BLTZAL
                    r[31] = pc+4;
                case 0b00000: // BLTZ
                    if (r[op.rs] < 0)
                        pc += static_cast<int32_t>(op.simm) << 2;
                    break;
                case 0b10001: // BGEZAL
                    r[31] = pc+4;
                case 0b00001: // BGEZ
                    if (r[op.rs] >= 0)
                        pc += static_cast<int32_t>(op.simm) << 2;
                    break;
                default:
                    throw InvalidOPException();
            }
            break;
        case Opcode::JAL:
            r[31] = pc+4;
        case Opcode::J:
            pc = (pc & 0xf0000000) | (op.addr << 2);
            break;
        case Opcode::BEQ:
            if (r[op.rs] == r[op.rt])
                pc += static_cast<int32_t>(op.simm) << 2;
            break;
        case Opcode::BNE:
            if (r[op.rs] != r[op.rt])
                pc += static_cast<int32_t>(op.simm) << 2;
            break;
        case Opcode::BLEZ:
            if (r[op.rs] <= 0)
                pc += static_cast<int32_t>(op.simm) << 2;
            break;
        case Opcode::BGTZ:
            if (r[op.rs] > 0)
                pc += static_cast<int32_t>(op.simm) << 2;
            break;
        case Opcode::ADDI:
            sr[op.rt] = sr[op.rs] + op.simm;
            break;
        case Opcode::ADDIU:
            r[op.rt] = r[op.rs] + op.imm;
            break;
        case Opcode::SLTI:
            r[op.rt] = (sr[op.rs] < op.simm);
            break;
        case Opcode::SLTIU:
            r[op.rt] = (r[op.rs] < op.imm);
            break;
        case Opcode::ANDI:
            r[op.rt] = r[op.rs] & op.imm;
            break;
        case Opcode::ORI:
            r[op.rt] = r[op.rs] | op.imm;
            break;
        case Opcode::XORI:
            r[op.rt] = r[op.rs] ^ op.imm;
            break;
        case Opcode::LUI:
            r[op.rt] = op.imm << 16;
            break;
        case Opcode::MTFC0:
            // Not supported, does nothing
            break;
        case Opcode::LB:
        case Opcode::LH:
        case Opcode::LW:
        case Opcode::LBU:
        case Opcode::LHU:
            // Delayed
            break;
        case Opcode::SB:
            ram[op.simm+r[op.rs]] = static_cast<uint8_t>(r[op.rt]);
            break;
        case Opcode::SH:
            ram[op.simm+r[op.rs]] = static_cast<uint16_t>(r[op.rt]);
            break;
        case Opcode::SW:
            ram[op.simm+r[op.rs]] = r[op.rt];
            break;
    }

    // Perform delay load
    switch (dlOpcode) {
        case Opcode::LB:
            sr[dlTarget] = static_cast<int8_t>(ram[dlAddr]);
            dlOpcode = Opcode::REG;
            break;
        case Opcode::LH:
            sr[dlTarget] = static_cast<int16_t>(ram[dlAddr]);
            dlOpcode = Opcode::REG;
            break;
        case Opcode::LW:
            r[dlTarget] = ram[dlAddr];
            dlOpcode = Opcode::REG;
            break;
        case Opcode::LBU:
            r[dlTarget] = static_cast<uint8_t>(ram[dlAddr]);
            dlOpcode = Opcode::REG;
            break;
        case Opcode::LHU:
            r[dlTarget] = static_cast<uint16_t>(ram[dlAddr]);
            dlOpcode = Opcode::REG;
            break;
        default:
            break;
    }

    // Always clear zero register
    r[0] = 0;

    // Prepare next delay load
    switch (op.opcode) {
        case Opcode::LB:
        case Opcode::LH:
        case Opcode::LW:
        case Opcode::LBU:
        case Opcode::LHU:
            dlOpcode = op.opcode;
            dlTarget = op.rt;
            dlAddr = op.simm+r[op.rs];
            break;
        default:
            break;
    }
}

void R3000::run()
{
    try {
        for (;/*_*/;)
            this->step();
    }
    catch (HaltException &) {
        // pass
    }
}
