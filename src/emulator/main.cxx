/*
 *  main.cxx
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

#include <iostream>
#include <fstream>

#include "defaults.hxx"
#include "io.hxx"
#include "ram.hxx"
#include "cpu.hxx"
#include "elf.hxx"

using namespace SoloMIPS;

static void printVersion(const char *argv0)
{
    std::cerr << "usage: " << argv0 << "[-d] <path>" << std::endl;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printVersion(argv[0]);
        return -20;
    }

    bool disassemble = false;
    const char *path = argv[1];
    if (std::strcmp(path, "-d") == 0) {
        disassemble = true;
        if (argc < 3) {
            printVersion(argv[0]);
            return -20;
        }
        path = argv[2];
    }

    // Prepare ROM
    ArrayRAMMapper rom(SOLOMIPS_DEFAULT_ENTRY, RAMMapperFlag::Readable | RAMMapperFlag::Executable);

    // Load program
    try {
        rom.setData(loadBinaryFile(path));
    }
    catch (IOException &e) {
        std::cerr << "error: " << e.what() << std::endl;
        return -21;
    }

    // Disassemble
    if (disassemble) {
        try {
            OP::disassemble(rom.data(), rom.size(), SOLOMIPS_DEFAULT_ENTRY, std::cout);
        }
        catch (InvalidOPException &e) {
            std::cerr << e.what();
            return -12;
        }
        return 0;
    }

    // Allocate work RAM
    ArrayRAMMapper wram(SOLOMIPS_DEFAULT_DATA_ADDR, SOLOMIPS_DEFAULT_DATA_SIZE);

    // Setup i/o RAM
    InputRAMMapper iram(SOLOMIPS_DEFAULT_I_ADDR);
    OutputRAMMapper oram(SOLOMIPS_DEFAULT_O_ADDR);

    // Setup CPU
    R3000 cpu(SOLOMIPS_DEFAULT_ENTRY);
    cpu.ram.addMapper(&rom);
    cpu.ram.addMapper(&iram);
    cpu.ram.addMapper(&oram);
    cpu.ram.addMapper(&wram);

    // Run
    try {
        cpu.run();
    }
    catch (ArithmeticException &e) {
        std::cerr << "error: arithmetic exception at 0x" << std::setfill('0') << std::setw(8) << std::hex << (cpu.pc - 8) << ": " << e.what() << std::endl;
        return -10;
    }
    catch (MemoryException &e) {
        std::cerr << "error: memory exception at 0x" << std::setfill('0') << std::setw(8) << std::hex << (cpu.pc - 8) << ": " << e.what() << std::endl;
        return -11;
    }
    catch (InvalidOPException &) {
        std::cerr << "error: invalid instruction at 0x" << std::setfill('0') << std::setw(8) << std::hex << (cpu.pc - 8) << std::endl;
        return -12;
    }
    catch (std::ios_base::failure &e) {
        std::cerr << "error: i/o exception at 0x" << std::setfill('0') << std::setw(8) << std::hex << (cpu.pc - 8) << ": " << e.what() << std::endl;
        return -21;
    }
    catch (std::exception &e) {
        std::cerr << "error: unknown exception at 0x" << std::setfill('0') << std::setw(8) << std::hex << (cpu.pc - 8) << ": " << e.what() << std::endl;
        return -20;
    }

    // Exit
    return (cpu.r[2] & 0xff);
}
