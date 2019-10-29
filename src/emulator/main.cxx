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

#include "ram.hxx"
#include "cpu.hxx"
#include "elf.hxx"

using namespace SoloMIPS;

#define READ_CHUNK_SIZE 0x010000u
#define MAX_ROM_SIZE 0x1000000u

static std::vector<uint8_t> loadBinary(const char *filename)
{
    std::ifstream bin;
    bin.open(filename, std::ios::in | std::ios::binary);
    if (!bin.is_open()) {
        std::cerr << "error: could not open input file" << std::endl;
        return {};
    }
    std::vector<uint8_t> data;
    size_t offset = 0;
    while (!bin.eof()) {
        data.resize(offset + READ_CHUNK_SIZE);
        bin.read(reinterpret_cast<char *>(data.data()) + offset, READ_CHUNK_SIZE);
        if (bin.fail() && !bin.eof()) {
            std::cerr << "error: could not load input file" << std::endl;
            bin.close();
            return {};
        }
        offset += bin.gcount();
        if (offset >= MAX_ROM_SIZE) {
            std::cerr << "error: file larger than maximum size" << std::endl;
            bin.close();
            return {};
        }
    }
    data.resize(offset);
    if (data.empty()) {
        std::cerr << "error: could not load input file or empty input file" << std::endl;
        return {};
    }
    bin.close();

    // Test if we got an ELF32 object file
    ELF32Object obj;
    if (obj.parse(data)) {
        if (obj.type == ELFObjectType::Rel && obj.machine == ELFMachineType::MIPS) {
            size_t ti = obj.indexOfSection(".text");
            size_t si = obj.indexOfSection(".symtab");
            if (ti != SIZE_T_MAX && si != SIZE_T_MAX) {
                for (ELFSymbolTableEntry &entry : obj.sections[si].symbolTable) {
                    if (entry.name == "main" && entry.shndx == ti) {
                        if (entry.type() == ELFSymbolType::Object) {
                            if (entry.value == 0) {
                                uint8_t *raw = data.data();
                                std::memmove(raw, raw + obj.sections[ti].offset, obj.sections[ti].size);
                                data.resize(obj.sections[ti].size);
                                return data;
                            }
                            else {
                                std::cerr << "error: \"main\" symbol in ELF object file must point to the first instruction" << std::endl;
                                return {};
                            }
                        }
                        else {
                            std::cerr << "error: \"main\" symbol in ELF object file must be an object (functions not supported yet)" << std::endl;
                            return {};
                        }
                    }
                }
                std::cerr << "error: no \"main\" symbol found in ELF object file" << std::endl;
                return {};
            }
            std::cerr << "error: ELF object file is required to contain both .text and .symtab sections" << std::endl;
        }
        else {
            std::cerr << "error: invalid ELF object file type or invalid machine type" << std::endl;
        }
        return {};
    }

    return data;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << "<path>" << std::endl;
        return -20;
    }

    // Load program into ROM
    ArrayRAMMapper rom(0x10000000u, loadBinary(argv[1]), RAMMapperFlag::Readable | RAMMapperFlag::Executable);

    if (rom.size() == 0)
        return -21;

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
