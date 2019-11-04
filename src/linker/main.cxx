/*
 *  main.cxx
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
#include <fstream>

#include "defaults.hxx"
#include "linker.hxx"
#include "io.hxx"

using namespace SoloMIPS;

static void showUsage(const char *argv0)
{
    std::cerr << "Usage: " << argv0 << "[options] file..." << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -o FILE, --output FILE      Set output file name (default: a.out)" << std::endl;
    std::cerr << "  -e ADDRESS, --entry ADDRESS Set start address (default: 0x10000000)" << std::endl;
    std::cerr << "  -Tdata ADDRESS              Set address of .data section (default: 0x20000000)" << std::endl;
    std::cerr << "  -Sdata SIZE                 Set size of .data section (default: 0x4000000)" << std::endl;
    std::cerr << "  -d, --disassemble           Print a disassembly of all input files (ignores -o)" << std::endl;
    std::cerr << "  -h, --help                  Print option help" << std::endl;
    std::cerr << "  -v, --version               Print version information" << std::endl;
}

static bool checkArg(const std::vector<std::string> &args, int i, int argc)
{
    if (i+1 == argc) {
        std::cerr << "error: option " << args[i] << " requires an argument" << std::endl;
        return false;
    }
    else if (args[i+1].length() == 0) {
        std::cerr << "error: argument to option " << args[i] << " can't be an empty string" << std::endl;
        return false;
    }
    return true;
}

static bool parseUInt32(const std::string &in, uint32_t *out)
{
    unsigned long ul = std::strtoul(in.data(), NULL, 0);
    if (ul == ULONG_MAX) {
        std::cerr << "error: argument '" << in << "' could not be interpreted as number" << std::endl;
        return false;
    }
    *out = static_cast<uint32_t>(ul);
    return true;
}

int main(int argc, char **argv)
{
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(const_cast<const char *>(argv[i]));
    }

    bool disassemble = false;
    std::string output = "a.out";
    uint32_t entry = SOLOMIPS_DEFAULT_ENTRY;
    uint32_t tdata = SOLOMIPS_DEFAULT_DATA_ADDR;
    uint32_t sdata = SOLOMIPS_DEFAULT_DATA_SIZE;
    std::vector<std::string> input;

    for (int i = 1; i < argc; ++i) {
        if (args[i].length() == 0) {
            std::cerr << "error: parameters cannot be empty strings" << std::endl;
            return 2;
        }

        if (args[i] == "-h" || args[i] == "--help") {
            showUsage(argv[0]);
            return 0;
        }
        if (args[i] == "-v" || args[i] == "--version") {
            std::cout << "SoloMIPS ld 0.0.1" << std::endl;
            return 0;
        }

        if (args[i] == "-o" || args[i] == "--output") {
            if (!checkArg(args, i, argc))
                return 2;
            output = args[i+1];
            ++i;
        }
        else if (args[i] == "-e" || args[i] == "--entry") {
            if (!checkArg(args, i, argc) || !parseUInt32(args[i+1], &entry))
                return 2;
            if (entry == 0) {
                std::cerr << "error: start address cannot be 0" << std::endl;
                return 2;
            }
            ++i;
        }
        else if (args[i] == "-d" || args[i] == "--disassemble") {
            disassemble = true;
        }
        else if (args[i] == "Tdata") {
            if (!checkArg(args, i, argc) || !parseUInt32(args[i+1], &tdata))
                return 2;
            if (tdata == 0) {
                std::cerr << "error: address of .data section cannot be 0" << std::endl;
                return 2;
            }
            ++i;
        }
        else if (args[i] == "Sdata") {
            if (!checkArg(args, i, argc) || !parseUInt32(args[i+1], &sdata))
                return 2;
            if (sdata == 0) {
                std::cerr << "error: size of .data section cannot be 0" << std::endl;
                return 2;
            }
            ++i;
        }
        else if (args[i][0] == '-') {
            std::cerr << "error: unrecognized option '" << args[i] << "'" << std::endl;
            return 2;
        }
        else {
            input.push_back(args[i]);
        }
    }

    Linker ld(input, entry, tdata, sdata);

    if (disassemble) {
        int ret = 0;
        try {
            ld.disassemble(std::cout);
        }
        catch (IOException &e) {
            std::cerr << "error: " << e.what() << std::endl;
            ret = 3;
        }
        catch (LinkerError &e) {
            std::cerr << "error: " << e.what() << std::endl;
            ret = 3;
        }
        return ret;
    }

    std::ofstream out;
    out.open(output, std::ios::out | std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "error: could not open output file for writing" << std::endl;
        return 3;
    }

    int ret = 0;
    try {
        ld.run(out);
    }
    catch (IOException &e) {
        std::cerr << "error: " << e.what() << std::endl;
        ret = 3;
    }
    catch (LinkerError &e) {
        std::cerr << "error: " << e.what() << std::endl;
        ret = 3;
    }

    if (out.fail()) {
        std::cerr << "error: could not write output file" << std::endl;
        ret = 3;
    }

    out.close();

    return ret;
}
