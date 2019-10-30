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

#include "linker.hxx"
#include "elf.hxx"
#include "io.hxx"

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
            throw LinkerError("'" + input + "' is not an ELF32 object file");

        if (obj.machine != ELFMachineType::MIPS)
            throw LinkerError("unsupported machine type in ELF object file '" + input + "'");
        if (obj.type != ELFObjectType::Rel)
            throw LinkerError("unsupported ELF object type in file '" + input + "'");

        size_t ti = obj.indexOfSection(".text");
        if (ti == SIZE_T_MAX)
            throw LinkerError("object file '" + input + "' does not contain any code");
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

        for (ELFSymbolTableEntry &entry : obj.sections[si].symbolTable) {
            if (entry.name == "main" && entry.shndx == ti) {
                if (entry.type() != ELFSymbolType::Object)
                    throw LinkerError("\"main\" symbol in object file '" + input + "' must be an object (functions not supported yet)");
                if (entry.value != 0)
                    throw LinkerError("\"main\" symbol in object file '" + input + "' must point to the first instruction");

                // Do relocations
                if (tir != SIZE_T_MAX) {

                }

                out.write(reinterpret_cast<char *>(data.data()) + obj.sections[ti].offset, obj.sections[ti].size);
                return;
            }
        }

        throw LinkerError("object file '" + input + "' does not contain a \"main\" symbol");
    }
}
