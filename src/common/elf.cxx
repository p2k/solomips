/*
 *  elf.cxx
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

#include "elf.hxx"

using namespace SoloMIPS;

ELF32Section::ELF32Section()
    : nameIndex(0), type(ELFSectionType::Null), flags(0), addr(0), offset(0), size(0), link(0), info(0), addralign(0), entsize(0) {}

ELF32Section::ELF32Section(const ELF32Object *obj, const std::vector<uint8_t> &data, size_t offset)
{
    this->parseHeader(obj, data, offset);
}

void ELF32Section::parseHeader(const ELF32Object *obj, const std::vector<uint8_t> &data, size_t offset)
{
    this->nameIndex = obj->readWord(data, offset);
    this->type = static_cast<ELFSectionType>(obj->readWord(data, offset+4));
    this->flags = obj->readWord(data, offset+8);
    this->addr = obj->readWord(data, offset+12);
    this->offset = obj->readWord(data, offset+16);
    this->size = obj->readWord(data, offset+20);
    this->link = obj->readWord(data, offset+24);
    this->info = obj->readWord(data, offset+28);
    this->addralign = obj->readWord(data, offset+32);
    this->entsize = obj->readWord(data, offset+36);
}

void ELF32Section::readSymbolTable(const ELF32Object *obj, const std::vector<uint8_t> &data)
{
    if (this->type != ELFSectionType::SymTab || this->offset == 0 || this->entsize < 16)
        return;

    this->symbolTable.clear();
    for (size_t s = 0, offset = this->offset; s < this->size; s += this->entsize, offset += this->entsize) {
        this->symbolTable.push_back(ELFSymbolTableEntry(obj, data, offset, this->link));
    }
}

ELFSymbolTableEntry::ELFSymbolTableEntry()
    : value(0), size(0), info(0), other(0), shndx(0) {}

ELFSymbolTableEntry::ELFSymbolTableEntry(const ELF32Object *obj, const std::vector<uint8_t> &data, size_t offset, size_t link)
{
    this->name = obj->readStringTable(data, link, obj->readWord(data, offset));
    this->value = obj->readWord(data, offset+4);
    this->size = obj->readWord(data, offset+8);
    uint16_t infoOther = obj->readHalf(data, offset+12);
    this->info = infoOther >> 8;
    this->other = infoOther & 0xff;
    this->shndx = obj->readHalf(data, offset+14);
}

bool ELFSymbolTableEntry::isLocal()
{
    return ((this->info >> 4) == 0);
}

bool ELFSymbolTableEntry::isGlobal()
{
    return ((this->info >> 4) == 1);
}

bool ELFSymbolTableEntry::isWeak()
{
    return ((this->info >> 4) == 2);
}

bool ELFSymbolTableEntry::isVisible()
{
    return ((this->other & 0x3) == 0);
}

ELFSymbolType ELFSymbolTableEntry::type()
{
    return static_cast<ELFSymbolType>(this->info & 0x0f);
}


ELF32Object::ELF32Object()
   : type(ELFObjectType::None), machine(ELFMachineType::None), version(0), entry(0), phoff(0), shoff(0), flags(0), ehsize(0), phentsize(0),
     phnum(0), shentsize(0), shnum(0), shstrndx(0) {} 

bool ELF32Object::parse(const std::vector<uint8_t> &data)
{
    if (data.size() < 52
            || data[0] != '\x7f' || data[1] != 'E' || data[2] != 'L' || data[3] != 'F' || data[4] != '\x01'
            || data[6] != '\x01' || data[7] != '\x00' || data[8] != '\x00' || data[9] != '\x00' || data[10] != '\x00' || data[11] != '\x00'
            || data[12] != '\x00' || data[13] != '\x00' || data[14] != '\x00' || data[15] != '\x00')
        return false;

    this->enc = static_cast<ELFDataEncoding>(data[5]);
    this->type = static_cast<ELFObjectType>(this->readHalf(data, 16));
    this->machine = static_cast<ELFMachineType>(this->readHalf(data, 18));
    this->version = this->readWord(data, 20);
    this->entry = this->readWord(data, 24);
    this->phoff = this->readWord(data, 28);
    this->shoff = this->readWord(data, 32);
    this->flags = this->readWord(data, 36);
    this->ehsize = this->readHalf(data, 40);
    this->phentsize = this->readHalf(data, 42);
    this->phnum = this->readHalf(data, 44);
    this->shentsize = this->readHalf(data, 46);
    this->shnum = this->readHalf(data, 48);
    this->shstrndx = this->readHalf(data, 50);

    if (this->version != 1 || this->ehsize != 52)
        return false;

    if (this->shoff == 0)
        return true;

    size_t end = this->shoff + this->shnum * this->shentsize;
    if (data.size() < end)
        return false;

    for (size_t i = 0, offset = this->shoff; i < this->shnum; ++i, offset += this->shentsize) {
        this->sections.push_back(ELF32Section(this, data, offset));
    }
    for (ELF32Section &section : this->sections) {
        section.name = this->readStringTable(data, section.nameIndex);
        if (section.type == ELFSectionType::SymTab)
            section.readSymbolTable(this, data);
    }

    return true;
}

size_t ELF32Object::indexOfSection(const std::string &name)
{
    size_t i = 0;
    for (ELF32Section &section : this->sections) {
        if (section.name == name)
            return i;
        else
            ++i;
    }
    return SIZE_T_MAX;
}

uint16_t ELF32Object::readHalf(const std::vector<uint8_t> &data, size_t offset) const
{
    if (this->enc == ELFDataEncoding::MSB)
        return (data[offset] << 8) | data[offset+1];
    else
        return (data[offset+1] << 8) | data[offset];
}

uint32_t ELF32Object::readWord(const std::vector<uint8_t> &data, size_t offset) const
{
    if (this->enc == ELFDataEncoding::MSB)
        return (data[offset] << 24) |(data[offset+1] << 16) | (data[offset+2] << 8) | data[offset+3];
    else
        return (data[offset+3] << 24) |(data[offset+2] << 16) | (data[offset+1] << 8) | data[offset];
}

std::string ELF32Object::readStringTable(const std::vector<uint8_t> &data, size_t index) const
{
    return this->readStringTable(data, this->shstrndx, index);
}

std::string ELF32Object::readStringTable(const std::vector<uint8_t> &data, size_t tableIndex, size_t index) const
{
    if (tableIndex == 0 || this->sections.size() <= tableIndex)
        return {};
    size_t offset = this->sections[tableIndex].offset + index;
    const char *rawData = reinterpret_cast<const char *>(data.data());
    size_t dataSize = data.size();
    size_t strLen = 0;
    for (size_t i = offset; offset < dataSize; ++i, ++strLen) {
        if (rawData[i] == '\0')
            return std::string(&rawData[offset], strLen);
    }
    return {};
}

