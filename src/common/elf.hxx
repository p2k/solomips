/*
 *  elf.hxx
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

#ifndef HEADER_SOLOMIPS_ELF_HXX
#define HEADER_SOLOMIPS_ELF_HXX

#include <cstdint>
#include <vector>
#include <string>

namespace SoloMIPS {

#define EI_NIDENT 16

enum class ELFDataEncoding : uint8_t
{
    None = 0,
    LSB = 1,
    MSB = 2
};

enum class ELFObjectType : uint16_t
{
    None = 0,
    Rel = 1,
    Exec = 2,
    Dyn = 3,
    Core = 4
};

enum class ELFMachineType : uint16_t
{
    None = 0,
    SPARC = 2,
    i386 = 3,
    m68K = 4,
    MIPS = 8,
    SPARC32PLUS = 18,
    PPC = 20,
    PPC64 = 21,
    ARM = 40,
    SPARCV9 = 43,
    X86_64 = 62,
    Z80 = 220
};

enum class ELFSectionType : uint32_t
{
    Null = 0,
    ProgBits = 1,
    SymTab = 2,
    StrTab = 3,
    RelA = 4,
    Hash = 5,
    Dynamic = 6,
    Note = 7,
    NoBits = 8,
    Rel = 9,
    ShLib = 10,
    DynSym = 11
};

enum class ELFSymbolType : uint8_t
{
    NoType = 0,
    Object = 1,
    Func = 2,
    Section = 3,
    File = 4,
    Common = 5,
    TLS = 6
};

enum class ELFRelType : uint8_t
{
    MIPS_NONE = 0,
    MIPS_16 = 1,
    MIPS_32 = 2,
    MIPS_REL32 = 3,
    MIPS_26 = 4,
    MIPS_HI16 = 5,
    MIPS_LO16 = 6,
    MIPS_GPREL16 = 7,
    MIPS_LITERAL = 8,
    MIPS_GOT16 = 9,
    MIPS_PC16 = 10,
    MIPS_CALL16 = 11,
    MIPS_GPREL32 = 12
};

struct ELF32Object;

struct ELFSymbolTableEntry
{
    ELFSymbolTableEntry();
    ELFSymbolTableEntry(const ELF32Object *obj, const std::vector<uint8_t> &data, size_t offset, size_t link);

    bool isLocal() const;
    bool isGlobal() const;
    bool isWeak() const;

    bool isVisible() const;

    ELFSymbolType type() const;

    std::string name;
	uint32_t value;
	uint32_t size;
	uint8_t info;
	uint8_t other;
	uint16_t shndx;
};

struct ELFRelTableEntry
{
    ELFRelTableEntry();
    ELFRelTableEntry(const ELF32Object *obj, const std::vector<uint8_t> &data, size_t offset, bool hasAddend);

    uint32_t sym() const;
    ELFRelType type() const;

    uint32_t offset;
	uint32_t info;
	int32_t addend;
};

struct ELF32Section
{
    ELF32Section();
    ELF32Section(const ELF32Object *obj, const std::vector<uint8_t> &data, size_t offset);
    void parseHeader(const ELF32Object *obj, const std::vector<uint8_t> &data, size_t offset);
    void readSymbolTable(const ELF32Object *obj, const std::vector<uint8_t> &data);
    void readRelTable(const ELF32Object *obj, const std::vector<uint8_t> &data);

    uint32_t nameIndex;
    std::string name;
    ELFSectionType type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;

    std::vector<ELFSymbolTableEntry> symbolTable;
    std::vector<ELFRelTableEntry> relTable;
};

struct ELF32Object
{
    friend struct ELF32Section;
    friend struct ELFSymbolTableEntry;
    friend struct ELFRelTableEntry;

    ELF32Object();
    bool parse(const std::vector<uint8_t> &data);

    size_t indexOfSection(const std::string &name);

    ELFDataEncoding enc;
    ELFObjectType type;
    ELFMachineType machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;

    std::vector<ELF32Section> sections;

private:
    uint16_t readHalf(const std::vector<uint8_t> &data, size_t offset) const;
    uint32_t readWord(const std::vector<uint8_t> &data, size_t offset) const;
    std::string readStringTable(const std::vector<uint8_t> &data, size_t index) const;
    std::string readStringTable(const std::vector<uint8_t> &data, size_t tableIndex, size_t index) const;
};

}

#endif /* HEADER_SOLOMIPS_ELF_HXX */
