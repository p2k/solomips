/*
 *  ram.hxx
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

#ifndef HEADER_SOLOMIPS_RAM_HXX
#define HEADER_SOLOMIPS_RAM_HXX

#include <cstdint>
#include <iostream>
#include <exception>
#include <vector>

#include "op.hxx"

namespace SoloMIPS {

/*
This is a RAM implementation based on operator overloading. The RAM object first
must be configured by installing mappers which handle loading, storing and
ability to decode appropriatly.

RAM mappers can be added and removed at runtime. The most recently added mapper
will be asked first; if no mapper responds, an exception is thrown.
*/

struct MemoryException : public std::exception
{
    explicit MemoryException(const char *msg) : _msg(msg) {}
    const char *what() const noexcept { return this->_msg; }
    const char *_msg;
};

// Base implementation; generates exceptions on everything
class RAMMapper
{
public:
    virtual ~RAMMapper();

    virtual bool respondsTo(uint32_t addr) const;

    virtual uint8_t loadByte(uint32_t addr) const;
    virtual uint16_t loadHalfWord(uint32_t addr) const;
    virtual uint32_t loadWord(uint32_t addr) const;

    virtual void storeByte(uint32_t addr, uint8_t value);
    virtual void storeHalfWord(uint32_t addr, uint16_t value);
    virtual void storeWord(uint32_t addr, uint32_t value);

    virtual uint32_t loadInstructionWord(uint32_t addr) const;

protected:
    RAMMapper();
};


enum class RAMMapperFlag : unsigned int
{
    Intangible = 0,
    Readable = 1<<0,
    Writable = 1<<1,
    Executable = 1<<2
};

RAMMapperFlag operator|(RAMMapperFlag lhs, RAMMapperFlag rhs);
RAMMapperFlag operator&(RAMMapperFlag lhs, RAMMapperFlag rhs);
RAMMapperFlag operator~(RAMMapperFlag f);


// Array-backed RAM Mapper; general purpose
class ArrayRAMMapper : public RAMMapper
{
public:
    ArrayRAMMapper(uint32_t offset, RAMMapperFlag flags = RAMMapperFlag::Readable);
    ArrayRAMMapper(uint32_t offset, const std::vector<uint8_t> &data, RAMMapperFlag flags = RAMMapperFlag::Readable);
    ArrayRAMMapper(uint32_t offset, std::vector<uint8_t> &&data, RAMMapperFlag flags = RAMMapperFlag::Readable);
    ArrayRAMMapper(uint32_t offset, uint32_t length, RAMMapperFlag flags = RAMMapperFlag::Readable | RAMMapperFlag::Writable);

    bool respondsTo(uint32_t addr) const;

    uint8_t loadByte(uint32_t addr) const;
    uint16_t loadHalfWord(uint32_t addr) const;
    uint32_t loadWord(uint32_t addr) const;

    void storeByte(uint32_t addr, uint8_t value);
    void storeHalfWord(uint32_t addr, uint16_t value);
    void storeWord(uint32_t addr, uint32_t value);

    uint32_t loadInstructionWord(uint32_t addr) const;

    RAMMapperFlag flags() const;
    void setFlags(RAMMapperFlag flags);
    bool isReadable() const;
    void setReadable(bool readable);
    bool isWriteable() const;
    void setWriteable(bool readable);
    bool isExecutable() const;
    void setExecutable(bool readable);

    uint32_t offset() const;
    void setOffset(uint32_t offset);

    uint8_t *data();
    void setData(const std::vector<uint8_t> &data);
    void setData(std::vector<uint8_t> &&data);
    uint32_t size() const;

private:
    uint32_t _offset;
    std::vector<uint8_t> _data;
    RAMMapperFlag _flags;
};


// Mapper for stream reading
class InputRAMMapper : public RAMMapper
{
public:
    explicit InputRAMMapper(uint32_t offset, std::istream *input = &std::cin);

    bool respondsTo(uint32_t addr) const;

    uint8_t loadByte(uint32_t addr) const;
    uint16_t loadHalfWord(uint32_t addr) const;
    uint32_t loadWord(uint32_t addr) const;

    uint32_t offset() const;
    void setOffset(uint32_t offset);

    std::istream *input() const;
    void setInput(std::istream *input);

private:
    uint32_t _offset;
    std::istream *_input;
};


// Mapper for stream writing
class OutputRAMMapper : public RAMMapper
{
public:
    explicit OutputRAMMapper(uint32_t offset, std::ostream *output = &std::cout);

    bool respondsTo(uint32_t addr) const;

    void storeByte(uint32_t addr, uint8_t value);
    void storeHalfWord(uint32_t addr, uint16_t value);
    void storeWord(uint32_t addr, uint32_t value);

    uint32_t offset() const;
    void setOffset(uint32_t offset);

    std::ostream *output() const;
    void setOutput(std::ostream *output);

private:
    uint32_t _offset;
    std::ostream *_output;
};


struct RAMPointer
{
public:
    RAMPointer(RAMMapper *mapper, uint32_t addr);

    operator uint8_t() const;
    operator uint16_t() const;
    operator uint32_t() const;
    operator int8_t() const;
    operator int16_t() const;
    operator int32_t() const;

    RAMPointer &operator=(uint8_t value);
    RAMPointer &operator=(uint16_t value);
    RAMPointer &operator=(uint32_t value);
    RAMPointer &operator=(int8_t value);
    RAMPointer &operator=(int16_t value);
    RAMPointer &operator=(int32_t value);

    uint32_t instr() const;
    operator OP() const;

private:
    RAMMapper *_mapper;
    uint32_t _addr;
};


class RAM
{
public:
    RAM();

    void addMapper(RAMMapper *mapper);
    void removeMapper(RAMMapper *mapper);
    void removeAllMappers();

    RAMPointer operator[](uint32_t addr);

private:
    std::vector<RAMMapper *> _mappers;
};

}

#endif /* HEADER_SOLOMIPS_CPU_HXX */
