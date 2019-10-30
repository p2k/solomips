/*
 *  io.hxx
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

#ifndef HEADER_SOLOMIPS_IO_HXX
#define HEADER_SOLOMIPS_IO_HXX

#include <cstdint>
#include <string>
#include <vector>
#include <exception>

namespace SoloMIPS {

class IOException : public std::exception
{
public:
    IOException(const std::string &msg);
    const char *what() const noexcept;

private:
    std::string _msg;
};

std::vector<uint8_t> loadBinaryFile(const std::string &fileName, size_t maxSize = 0x1000000u, size_t chunkSize = 0x010000u);

}

#endif /* HEADER_SOLOMIPS_IO_HXX */
