/*
 *  ram.cxx
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

#include "ram.hxx"

using namespace SoloMIPS;

RAMMapper::RAMMapper() {}

RAMMapper::~RAMMapper() {}

bool RAMMapper::respondsTo(uint32_t addr) const
{
    (void)addr;
    return false;
}

void RAMMapper::storeByte(uint32_t addr, uint8_t value)
{
    (void)addr;
    (void)value;
    throw MemoryException("Memory not accessible for writing");
}

void RAMMapper::storeHalfWord(uint32_t addr, uint16_t value)
{
    (void)addr;
    (void)value;
    throw MemoryException("Memory not accessible for writing");
}

void RAMMapper::storeWord(uint32_t addr, uint32_t value)
{
    (void)addr;
    (void)value;
    throw MemoryException("Memory not accessible for writing");
}

uint8_t RAMMapper::loadByte(uint32_t addr) const
{
    (void)addr;
    throw MemoryException("Memory not accessible for reading");
}

uint16_t RAMMapper::loadHalfWord(uint32_t addr) const
{
    (void)addr;
    throw MemoryException("Memory not accessible for reading");
}

uint32_t RAMMapper::loadWord(uint32_t addr) const
{
    (void)addr;
    throw MemoryException("Memory not accessible for reading");
}

uint32_t RAMMapper::loadInstructionWord(uint32_t addr) const
{
    (void)addr;
    throw MemoryException("Memory not accessible for executing");
}


RAMMapperFlag SoloMIPS::operator|(RAMMapperFlag lhs, RAMMapperFlag rhs)
{
    return static_cast<RAMMapperFlag>(static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
}

RAMMapperFlag SoloMIPS::operator&(RAMMapperFlag lhs, RAMMapperFlag rhs)
{
    return static_cast<RAMMapperFlag>(static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs));
}

RAMMapperFlag SoloMIPS::operator~(RAMMapperFlag f)
{
    return static_cast<RAMMapperFlag>(static_cast<unsigned int>(f) ^ 0x7);
}


ArrayRAMMapper::ArrayRAMMapper(uint32_t offset, const std::vector<uint8_t> &data, RAMMapperFlag flags)
    : _offset(offset), _data(data), _flags(flags) {}

ArrayRAMMapper::ArrayRAMMapper(uint32_t offset, std::vector<uint8_t> &&data, RAMMapperFlag flags)
    : _offset(offset), _data(std::move(data)), _flags(flags) {}

ArrayRAMMapper::ArrayRAMMapper(uint32_t offset, uint32_t length, RAMMapperFlag flags)
    : _offset(offset), _data(length, 0), _flags(flags) {}

bool ArrayRAMMapper::respondsTo(uint32_t addr) const
{
    return (addr >= this->_offset && addr < this->_offset + this->_data.size());
}

uint8_t ArrayRAMMapper::loadByte(uint32_t addr) const
{
    if (this->isReadable())
        return this->_data[addr - this->_offset];
    return RAMMapper::loadByte(addr);
}

uint16_t ArrayRAMMapper::loadHalfWord(uint32_t addr) const
{
    if (!this->respondsTo(addr + 1))
        throw MemoryException("Segmentation fault");
    if (this->isReadable()) {
        size_t i = addr - this->_offset;
        return (this->_data[i] << 8) | this->_data[i+1];
    }
    return RAMMapper::loadHalfWord(addr);
}

uint32_t ArrayRAMMapper::loadWord(uint32_t addr) const
{
    if (!this->respondsTo(addr + 3))
        throw MemoryException("Segmentation fault");
    if (this->isReadable()) {
        size_t i = addr - this->_offset;
        return (this->_data[i] << 24) | (this->_data[i+1] << 16) | (this->_data[i+2] << 8) | this->_data[i+3];
    }
    return RAMMapper::loadWord(addr);
}

void ArrayRAMMapper::storeByte(uint32_t addr, uint8_t value)
{
    if (this->isWriteable())
        this->_data[addr - this->_offset] = value;
    else
        RAMMapper::storeByte(addr, value);
}

void ArrayRAMMapper::storeHalfWord(uint32_t addr, uint16_t value)
{
    if (!this->respondsTo(addr + 1))
        throw MemoryException("Segmentation fault");
    if (this->isWriteable()) {
        size_t i = addr - this->_offset;
        this->_data[i] = value >> 8;
        this->_data[i+1] = value & 0xff;
    }
    else {
        RAMMapper::storeHalfWord(addr, value);
    }
}

void ArrayRAMMapper::storeWord(uint32_t addr, uint32_t value)
{
    if (!this->respondsTo(addr + 3))
        throw MemoryException("Segmentation fault");
    if (this->isWriteable()) {
        size_t i = addr - this->_offset;
        this->_data[i] = value >> 24;
        this->_data[i+1] = (value >> 16) & 0xff;
        this->_data[i+2] = (value >> 8) & 0xff;
        this->_data[i+3] = value & 0xff;
    }
    else {
        RAMMapper::storeWord(addr, value);
    }
}

uint32_t ArrayRAMMapper::loadInstructionWord(uint32_t addr) const
{
    if (this->isExecutable())
        return this->loadWord(addr);
    return RAMMapper::loadInstructionWord(addr);
}

RAMMapperFlag ArrayRAMMapper::flags() const
{
    return this->_flags;
}

void ArrayRAMMapper::setFlags(RAMMapperFlag flags)
{
    this->_flags = flags;
}

bool ArrayRAMMapper::isReadable() const
{
    return (this->_flags & RAMMapperFlag::Readable) == RAMMapperFlag::Readable;
}

void ArrayRAMMapper::setReadable(bool readable)
{
    if (readable)
        this->_flags = this->_flags | RAMMapperFlag::Readable;
    else
        this->_flags = this->_flags & ~RAMMapperFlag::Readable;
}

bool ArrayRAMMapper::isWriteable() const
{
    return (this->_flags & RAMMapperFlag::Writable) == RAMMapperFlag::Writable;
}

void ArrayRAMMapper::setWriteable(bool readable)
{
    if (readable)
        this->_flags = this->_flags | RAMMapperFlag::Writable;
    else
        this->_flags = this->_flags & ~RAMMapperFlag::Writable;
}

bool ArrayRAMMapper::isExecutable() const
{
    return (this->_flags & RAMMapperFlag::Executable) == RAMMapperFlag::Executable;
}

void ArrayRAMMapper::setExecutable(bool readable)
{
    if (readable)
        this->_flags = this->_flags | RAMMapperFlag::Executable;
    else
        this->_flags = this->_flags & ~RAMMapperFlag::Executable;
}

uint32_t ArrayRAMMapper::offset() const
{
    return this->_offset;
}

void ArrayRAMMapper::setOffset(uint32_t offset)
{
    this->_offset = offset;
}

uint8_t *ArrayRAMMapper::data()
{
    return this->_data.data();
}

uint32_t ArrayRAMMapper::size() const
{
    return static_cast<uint32_t>(this->_data.size());
}


InputRAMMapper::InputRAMMapper(uint32_t offset, std::istream *input)
    : _offset(offset), _input(input) {}

bool InputRAMMapper::respondsTo(uint32_t addr) const
{
    return (addr == this->_offset);
}

uint8_t InputRAMMapper::loadByte(uint32_t addr) const
{
    (void)addr;
    return static_cast<uint8_t>(this->_input->get());
}

uint16_t InputRAMMapper::loadHalfWord(uint32_t addr) const
{
    return static_cast<uint16_t>(this->loadByte(addr));
}

uint32_t InputRAMMapper::loadWord(uint32_t addr) const
{
    return static_cast<uint32_t>(this->loadByte(addr));
}

uint32_t InputRAMMapper::offset() const
{
    return this->_offset;
}

void InputRAMMapper::setOffset(uint32_t offset)
{
    this->_offset = offset;
}

std::istream *InputRAMMapper::input() const
{
    return this->_input;
}

void InputRAMMapper::setInput(std::istream *input)
{
    this->_input = input;
}


OutputRAMMapper::OutputRAMMapper(uint32_t offset, std::ostream *output)
    : _offset(offset), _output(output) {}

bool OutputRAMMapper::respondsTo(uint32_t addr) const
{
    return (addr == this->_offset);
}

void OutputRAMMapper::storeByte(uint32_t addr, uint8_t value)
{
    (void)addr;
    this->_output->put(static_cast<char>(value));
}

void OutputRAMMapper::storeHalfWord(uint32_t addr, uint16_t value)
{
    this->storeByte(addr, static_cast<uint8_t>(value));
}

void OutputRAMMapper::storeWord(uint32_t addr, uint32_t value)
{
    this->storeByte(addr, static_cast<uint8_t>(value));
}

uint32_t OutputRAMMapper::offset() const
{
    return this->_offset;
}

void OutputRAMMapper::setOffset(uint32_t offset)
{
    this->_offset = offset;
}

std::ostream *OutputRAMMapper::output() const
{
    return this->_output;
}

void OutputRAMMapper::setOutput(std::ostream *output)
{
    this->_output = output;
}


RAMPointer::RAMPointer(RAMMapper *mapper, uint32_t addr)
    : _mapper(mapper), _addr(addr) {}

RAMPointer::operator uint8_t() const
{
    return this->_mapper->loadByte(this->_addr);
}

RAMPointer::operator uint16_t() const
{
    return this->_mapper->loadHalfWord(this->_addr);
}

RAMPointer::operator uint32_t() const
{
    return this->_mapper->loadWord(this->_addr);
}

RAMPointer::operator int8_t() const
{
    return static_cast<int8_t>(this->_mapper->loadByte(this->_addr));
}

RAMPointer::operator int16_t() const
{
    return static_cast<int16_t>(this->_mapper->loadHalfWord(this->_addr));
}

RAMPointer::operator int32_t() const
{
    return static_cast<int8_t>(this->_mapper->loadWord(this->_addr));
}

RAMPointer &RAMPointer::operator=(uint8_t value)
{
    this->_mapper->storeByte(this->_addr, value);
    return *this;
}

RAMPointer &RAMPointer::operator=(uint16_t value)
{
    this->_mapper->storeHalfWord(this->_addr, value);
    return *this;
}

RAMPointer &RAMPointer::operator=(uint32_t value)
{
    this->_mapper->storeWord(this->_addr, value);
    return *this;
}

RAMPointer &RAMPointer::operator=(int8_t value)
{
    this->_mapper->storeByte(this->_addr, static_cast<uint8_t>(value));
    return *this;
}

RAMPointer &RAMPointer::operator=(int16_t value)
{
    this->_mapper->storeHalfWord(this->_addr, static_cast<uint16_t>(value));
    return *this;
}

RAMPointer &RAMPointer::operator=(int32_t value)
{
    this->_mapper->storeWord(this->_addr, static_cast<uint32_t>(value));
    return *this;
}

uint32_t RAMPointer::instr() const
{
    return this->_mapper->loadInstructionWord(this->_addr);
}

RAMPointer::operator OP() const
{
    return OP(this->instr());
}


RAM::RAM() {}

void RAM::addMapper(RAMMapper *mapper)
{
    this->_mappers.push_back(mapper);
}

void RAM::removeMapper(RAMMapper *mapper)
{
    for (auto i = this->_mappers.begin(); i != this->_mappers.end(); ++i) {
        if (*i == mapper) {
            this->_mappers.erase(i);
            break;
        }
    }
}

void RAM::removeAllMappers()
{
    this->_mappers.clear();
}

RAMPointer RAM::operator[](uint32_t addr)
{
    for (auto i = this->_mappers.rbegin(); i != this->_mappers.rend(); ++i) {
        if ((*i)->respondsTo(addr))
            return RAMPointer(*i, addr);
    }

    throw MemoryException("Segmentation fault");
}
