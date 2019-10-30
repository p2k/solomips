/*
 *  linker.hxx
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

#ifndef HEADER_SOLOMIPS_LINKER_HXX
#define HEADER_SOLOMIPS_LINKER_HXX

#include <cstdint>
#include <string>
#include <vector>
#include <exception>
#include <ostream>

namespace SoloMIPS {

class LinkerError : public std::exception
{
public:
    LinkerError(const std::string &msg) : _msg(msg) {}
    const char *what() const noexcept { return this->_msg.data(); }

private:
    std::string _msg;
};


class Linker
{
public:
    Linker(const std::vector<std::string> &input, uint32_t entry = 0x10000000, uint32_t tdata = 0x20000000, uint32_t sdata = 0x4000000);

    void run(std::ostream &out);

    const std::vector<uint8_t> &output() const;

    const char *outputData() const;
    size_t outputSize() const;

private:
    std::vector<std::string> _input;
    uint32_t _entry;
    uint32_t _tdata;
    uint32_t _sdata;
};

}

#endif /* HEADER_SOLOMIPS_LINKER_HXX */
