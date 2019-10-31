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
    ArrayRAMMapper rom(0x10000000u, RAMMapperFlag::Readable | RAMMapperFlag::Executable);

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
            OP::disassemble(rom.data(), rom.size(), std::cout);
        }
        catch (InvalidOPException &e) {
            std::cerr << e.what();
            return -12;
        }
        return 0;
    }

    // Allocate work RAM
    ArrayRAMMapper wram(0x20000000u, 0x4000000u);

    // Setup i/o RAM
    InputRAMMapper iram(0x30000000);
    OutputRAMMapper oram(0x30000004);

    // Setup CPU
    R3000 cpu(0x10000000);
    cpu.ram.addMapper(&rom);
    cpu.ram.addMapper(&iram);
    cpu.ram.addMapper(&oram);
    cpu.ram.addMapper(&wram);

    // Run
    try {
        cpu.run();
    }
    catch (ArithmeticException &e) {
        std::cerr << "error: arithmetic exception at " << (cpu.pc - 4) << ": " << e.what() << std::endl;
        return -10;
    }
    catch (MemoryException &e) {
        std::cerr << "error: memory exception at " << (cpu.pc - 4) << ": " << e.what() << std::endl;
        return -11;
    }
    catch (InvalidOPException &) {
        std::cerr << "error: invalid instruction at " << (cpu.pc - 4) << std::endl;
        return -12;
    }
    catch (std::ios_base::failure &e) {
        std::cerr << "error: i/o exception at " << (cpu.pc - 4) << ": " << e.what() << std::endl;
        return -21;
    }
    catch (std::exception &e) {
        std::cerr << "error: unknown exception at " << (cpu.pc - 4) << ": " << e.what() << std::endl;
        return -20;
    }

    // Exit
    return (cpu.r[2] & 0xff);
}
