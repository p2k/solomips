/*
 *  io.cxx
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

#include <fstream>

#include "io.hxx"

using namespace SoloMIPS;

IOException::IOException(const std::string &msg)
    : _msg(msg) {}

const char *IOException::what() const noexcept
{
    return this->_msg.data();
}


std::vector<uint8_t> SoloMIPS::loadBinaryFile(const std::string &fileName, size_t maxSize, size_t chunkSize)
{
    std::ifstream bin;
    bin.open(fileName, std::ios::in | std::ios::binary);
    if (!bin.is_open()) {
        throw IOException("could not open file '" + fileName + "'");
    }
    std::vector<uint8_t> data;
    size_t offset = 0;
    while (!bin.eof()) {
        data.resize(offset + chunkSize);
        bin.read(reinterpret_cast<char *>(data.data()) + offset, chunkSize);
        if (bin.fail() && !bin.eof()) {
            bin.close();
            throw IOException("could not read file '" + fileName + "'");
        }
        offset += bin.gcount();
        if (offset > maxSize || (offset == maxSize && !bin.eof())) {
            bin.close();
            throw IOException("file '" + fileName + "' too large");
        }
    }
    bin.close();
    data.resize(offset);
    if (data.empty())
        throw IOException("file '" + fileName + "' is empty or could not be read");
    return data;
}
