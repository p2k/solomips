/*
 *  cpu.hxx
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

#ifndef HEADER_SOLOMIPS_CPU_HXX
#define HEADER_SOLOMIPS_CPU_HXX

#include "op.hxx"
#include "ram.hxx"

#include <exception>
#include <memory>

/*
This emulates a R2000 processor on a high level. It is partially optimized for
speed and memory consumption; tradeoffs are made to improve clarity.

The internal state is publicly exposed and can be manipulated (with obvious
consequences).

Attempting to execute address 0 will halt the processor (throw a HaltException).
*/

namespace SoloMIPS {

struct HaltException : public std::exception {};
struct MisalignedPCException : public std::exception {};

struct ArithmeticException : public std::exception
{
    ArithmeticException(const char *msg) : _msg(msg) {}
    const char *what() const noexcept { return this->_msg; }
    const char *_msg;
};

enum class DelayedException : unsigned int
{
    None = 0,
    MisalignedPCException,
    HaltException,
    InvalidOPException,
    MemoryException
};

class R3000
{
public:
    R3000(uint32_t entrypoint);

    /**
     * Reset all registers to zero, set pc to entrypoint, clear op and nextOp
     * and any delayed exception. Does not clean the work RAM.
     */
    void reset();

    /**
     * Perform one CPU cycle.
     * 
     * Throw delayed exceptions, perform delayed loading, copy nextOp to op,
     * load next instruction from pc, increment pc, execute op.
     */
    void step();

    /**
     * Repeatedly calls `step()` until a HaltException is thrown.
     */
    void run();

    union {
        uint32_t r[32];
        int32_t sr[32];
    };

    RAM ram;
    uint32_t entrypoint;

    uint32_t pc;
    uint32_t hi;
    uint32_t lo;

    OP op;
    OP nextOp;

    Opcode dlOpcode;
    uint8_t dlTarget;
    uint32_t dlAddr;

    DelayedException dex;
    const char *dexWhat;
};

}

#endif /* HEADER_SOLOMIPS_CPU_HXX */
