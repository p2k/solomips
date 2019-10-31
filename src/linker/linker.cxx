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

#include <iostream>

#include "linker.hxx"
#include "elf.hxx"
#include "io.hxx"
#include "op.hxx"

using namespace SoloMIPS;

Linker::Linker(const std::vector<std::string> &input, uint32_t entry, uint32_t tdata, uint32_t sdata)
    : _input(input), _entry(entry), _tdata(tdata), _sdata(sdata) {}

void Linker::run(std::ostream &out)
{
    if (this->_input.size() == 0)
        throw LinkerError("no input files");
    if (this->_input.size() > 1)
        throw LinkerError("currently only a single input file is supported");

    for (std::string input : this->_input) {
        std::vector<uint8_t> data = loadBinaryFile(input);

        ELF32Object obj;
        if (!obj.parse(data))
            throw LinkerError("'" + input + "' is not a valid ELF32 object file");

        if (obj.machine != ELFMachineType::MIPS)
            throw LinkerError("unsupported machine type in ELF object file '" + input + "'");
        if (obj.type != ELFObjectType::Rel)
            throw LinkerError("unsupported ELF object type in file '" + input + "'");

        size_t ti = obj.indexOfSection(".text");
        if (ti == SIZE_T_MAX)
            throw LinkerError("object file '" + input + "' does not contain any code");
        ELF32Section &textSection = obj.sections[ti];
        size_t di = obj.indexOfSection(".data");
        if (di != SIZE_T_MAX) {
            uint32_t dataSize = obj.sections[di].size;
            if (dataSize > this->_sdata)
                throw LinkerError("data section of '" + input + "' is too large");
            for (uint8_t *p = data.data() + obj.sections[di].offset, *e = p + dataSize; p != e; ++p) {
                if (*p != 0)
                    throw LinkerError("data section of '" + input + "' is not empty (this is not supported yet)");
            }
        }
        size_t si = obj.indexOfSection(".symtab");
        if (si == SIZE_T_MAX)
            throw LinkerError("object file '" + input + "' does not contain a symbol table");
        size_t tir = obj.indexOfSection(".rel.text");
        if (tir != SIZE_T_MAX) {
            if (obj.sections[tir].link != si)
                throw LinkerError("code relocation table of object file '" + input + "' does not point to the correct symbol table");
            if (obj.sections[tir].info != ti)
                throw LinkerError("code relocation table of object file '" + input + "' does not point to the correct code section");
        }

        std::vector<ELFSymbolTableEntry> &symbolTable = obj.sections[si].symbolTable;
        for (ELFSymbolTableEntry &entry : symbolTable) {
            if (entry.name == "main" && entry.shndx == ti) {
                if (entry.type() != ELFSymbolType::Object)
                    throw LinkerError("\"main\" symbol in object file '" + input + "' must be an object (functions not supported yet)");
                if (entry.value != 0)
                    throw LinkerError("\"main\" symbol in object file '" + input + "' must point to the first instruction");

                uint8_t *text = data.data() + textSection.offset;
                // Do relocations
                if (tir != SIZE_T_MAX) {
                    bool hasAddendum = (obj.sections[tir].type == ELFSectionType::RelA);
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
                        uint32_t addr = rentry.addend;
                        if (targetSym.shndx == ti)
                            addr += this->_entry;
                        else if (targetSym.shndx == di)
                            addr += this->_tdata;
                        else
                            throw LinkerError("code relocation table of object file '" + input + "' contains an unsupported relocation target");

                        switch (rentry.type()) {
                            case ELFRelType::MIPS_GOT16: {
                                // Requires the next rel entry to be a LO16 with same symbol
                                if (i+1 == relTable.size() || relTable[i+1].type() != ELFRelType::MIPS_LO16 || relTable[i+1].sym() != rsym)
                                    throw LinkerError("code relocation table of object file '" + input + "' is invalid (GOT16 not followed by valid LO16)");
                                ELFRelTableEntry &rentry2 = relTable[i+1];
                                if (rentry2.offset+4 > textSection.size)
                                    throw LinkerError("code relocation table of object file '" + input + "' contains an out-of-bounds offset");

                                OP op;
                                op.decode(&text[rentry.offset]);
                                std::cout << op << std::endl;
                                op.decode(&text[rentry2.offset]);
                                std::cout << op << std::endl;

                                if (!hasAddendum)
                                    addr += (text[rentry.offset + 2] << 24) | (text[rentry.offset + 3] << 16) | (text[rentry2.offset + 2] << 8) | text[rentry2.offset + 3];
                                text[rentry.offset + 2] = addr >> 24;
                                text[rentry.offset + 3] = (addr >> 16) & 0xff;
                                text[rentry2.offset + 2] = (addr >> 8) & 0xff;
                                text[rentry2.offset + 3] = addr & 0xff;
                                ++i;
                                break;
                            }

                            default:
                                throw LinkerError("code relocation table of object file '" + input + "' contains an unsupported relocation type");
                        }
                    }
                }

                out.write(reinterpret_cast<char *>(data.data()) + obj.sections[ti].offset, obj.sections[ti].size);
                return;
            }
        }

        throw LinkerError("object file '" + input + "' does not contain a \"main\" symbol");
    }
}
