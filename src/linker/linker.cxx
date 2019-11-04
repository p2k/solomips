/*
 *  linker.cxx
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

#include <climits>
#include <iostream>

#include "linker.hxx"
#include "elf.hxx"
#include "io.hxx"
#include "op.hxx"

using namespace SoloMIPS;

static void parseCheckObjectData(const std::string &input, const std::vector<uint8_t> &data, ELF32Object &obj)
{
    if (!obj.parse(data))
        throw LinkerError("'" + input + "' is not a valid ELF32 object file");

    if (obj.machine != ELFMachineType::MIPS)
        throw LinkerError("unsupported machine type in ELF object file '" + input + "'");
    if (obj.type != ELFObjectType::Rel)
        throw LinkerError("unsupported ELF object type in file '" + input + "'");

    if (obj.indexOfSection(".text") == SIZE_MAX)
        throw LinkerError("object file '" + input + "' does not contain any code");
    if (obj.indexOfSection(".symtab") == SIZE_MAX)
        throw LinkerError("object file '" + input + "' does not contain a symbol table");
}

static size_t findRelocationTable(size_t textSectionIndex, const ELF32Object &obj)
{
    for (size_t tir = 0; tir < obj.sections.size(); ++tir) {
        if (obj.sections[tir].type == ELFSectionType::Rel && obj.sections[tir].info == textSectionIndex) {
            return tir;
        }
    }

    return SIZE_MAX;
}

Linker::Linker(const std::vector<std::string> &input, uint32_t entry, uint32_t tdata, uint32_t sdata)
    : _input(input), _entry(entry), _tdata(tdata), _sdata(sdata) {}

void Linker::run(std::ostream &out) const
{
    out.setf(std::ios::binary);

    if (this->_input.size() == 0u)
        throw LinkerError("no input files");
    if (this->_input.size() > 1u)
        throw LinkerError("currently only a single input file is supported");

    for (std::string input : this->_input) {
        std::vector<uint8_t> data = loadBinaryFile(input);

        // Parse and do all sorts of checks
        ELF32Object obj;
        parseCheckObjectData(input, data, obj);

        size_t di = obj.indexOfSection(".data");
        if (di != SIZE_MAX) {
            uint32_t dataSize = obj.sections[di].size;
            if (dataSize > this->_sdata - 4)
                throw LinkerError("data section of '" + input + "' is too large");

            if (dataSize > 0) {
                for (uint8_t *p = data.data() + obj.sections[di].offset, *e = p + dataSize; p != e; ++p) {
                    if (*p != 0)
                        throw LinkerError("data section of '" + input + "' is not empty (this is not supported yet)");
                }

                // Emit code to setup the Global Offset Table and prepare $gp
                uint32_t gp = this->_tdata + this->_sdata - 4;
                out << OP::LUI(28, gp >> 16);
                if (gp & 0xffff)
                    out << OP::ORI(28, 28, gp & 0xffff);

                out << OP::LUI(1, this->_tdata >> 16);
                if (this->_tdata & 0xffff)
                    out << OP::ORI(1, 1, this->_tdata & 0xffff);
                out << OP::SW(1, 0, 28)
                    << OP::OR(1, 0, 0);
            }
        }

        bool foundMain = false;
        size_t si = obj.indexOfSection(".symtab");
        std::vector<ELFSymbolTableEntry> &symbolTable = obj.sections[si].symbolTable;
        for (ELFSymbolTableEntry &entry : symbolTable) {
            if (entry.name == "main") {
                ELFSymbolType est = entry.type();
                if (entry.value != 0 && est != ELFSymbolType::Func)
                    throw LinkerError("\"main\" symbol in object file '" + input + "', if not a function, must point to the first instruction");

                size_t ti = entry.shndx;
                ELF32Section &textSection = obj.sections[ti];
                if (textSection.type != ELFSectionType::ProgBits)
                    throw LinkerError("\"main\" symbol in object file '" + input + "' does not point to a text section");

                size_t tir = findRelocationTable(ti, obj);
                if (tir != SIZE_MAX) {
                    if (obj.sections[tir].link != si)
                        throw LinkerError("code relocation table of object file '" + input + "' does not point to the correct symbol table");
                }

                if (est == ELFSymbolType::Func) {
                    // Setup stack
                    uint32_t sp = this->_tdata + this->_sdata - 8;
                    out << OP::LUI(29, sp >> 16);
                    if (sp & 0xffff)
                        out << OP::ORI(29, 29, sp & 0xffff);
                    // Emit call and exit code
                    out << OP::BGEZAL(0, 3)
                        << OP()
                        << OP::JR(0)
                        << OP();
                }

                uint8_t *text = data.data() + textSection.offset;
                // Do relocations
                if (tir != SIZE_MAX) {
                    std::vector<ELFRelTableEntry> &relTable = obj.sections[tir].relTable;
                    for (size_t i = 0; i < relTable.size(); ++i) {
                        ELFRelTableEntry &rentry = relTable[i];
                        if (rentry.offset+4 > textSection.size)
                            throw LinkerError("code relocation table of object file '" + input + "' contains an out-of-bounds offset");
                        uint32_t rsym = rentry.sym();
                        if (rsym >= symbolTable.size())
                            throw LinkerError("code relocation table of object file '" + input + "' contains an out-of-bounds relocation target");
                        ELFSymbolTableEntry &targetSym = symbolTable[rsym];
                        if (targetSym.type() != ELFSymbolType::Section)
                            throw LinkerError("code relocation table of object file '" + input + "' contains an unsupported relocation target type");
                        if (targetSym.shndx != di)
                            throw LinkerError("code relocation table of object file '" + input + "' contains an unsupported relocation target");

                        switch (rentry.type()) {
                            case ELFRelType::MIPS_GOT16: {
                                // Requires the next rel entry to be a LO16 with same symbol
                                if (i+1 == relTable.size() || relTable[i+1].type() != ELFRelType::MIPS_LO16 || relTable[i+1].sym() != rsym)
                                    throw LinkerError("code relocation table of object file '" + input + "' is invalid (GOT16 not followed by valid LO16)");
                                if (relTable[i+1].offset+4 > textSection.size)
                                    throw LinkerError("code relocation table of object file '" + input + "' contains an out-of-bounds offset");

                                // Force to zero
                                text[rentry.offset + 2] = 0;
                                text[rentry.offset + 3] = 0;
                                // Leave GP offset alone
                                ++i;
                                break;
                            }

                            default:
                                throw LinkerError("code relocation table of object file '" + input + "' contains an unsupported relocation type");
                        }
                    }
                }

                out.write(reinterpret_cast<char *>(data.data()) + textSection.offset, textSection.size);
                foundMain = true;
                break;
            }
        }

        if (!foundMain)
            throw LinkerError("object file '" + input + "' does not contain a \"main\" symbol");
    }
}

void Linker::disassemble(std::ostream &out) const
{
    if (this->_input.size() == 0u)
        throw LinkerError("no input files");
    if (this->_input.size() > 1u)
        throw LinkerError("currently only a single input file is supported");

    for (std::string input : this->_input) {
        std::vector<uint8_t> data = loadBinaryFile(input);

        ELF32Object obj;
        parseCheckObjectData(input, data, obj);

        bool foundMain = false;
        size_t si = obj.indexOfSection(".symtab");
        std::vector<ELFSymbolTableEntry> &symbolTable = obj.sections[si].symbolTable;
        for (ELFSymbolTableEntry &entry : symbolTable) {
            if (entry.name == "main") {
                size_t ti = entry.shndx;
                ELF32Section &textSection = obj.sections[ti];
                if (textSection.type != ELFSectionType::ProgBits)
                    throw LinkerError("\"main\" symbol in object file '" + input + "' does not point to a text section");

                const uint8_t *text = data.data() + textSection.offset;
                out << input << ":" << std::endl;
                OP::disassemble(text, textSection.size, out);
                out << std::endl;
                foundMain = true;
            }
        }

        if (!foundMain)
            throw LinkerError("object file '" + input + "' does not contain a \"main\" symbol");
    }
}
