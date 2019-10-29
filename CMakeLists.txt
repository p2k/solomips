#
# CMakeLists.txt
#
# Copyright (C) 2019  Patrick "p2k" Schneider
#
# This file is part of SoloMIPS.
#
# SoloMIPS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# SoloMIPS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with SoloMIPS.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(SoloMIPS CXX)

set(CXX_STANDARD_REQUIRED ON)
set(CXX_STANDARD 11)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /EHsc")
else()
    message(WARNING "Unknown compiler ID: ${CMAKE_CXX_COMPILER_ID}")
endif()

file(GLOB src_common "src/common/*.cxx" "src/common/*.hxx")
file(GLOB src_emulator "src/emulator/*.cxx" "src/emulator/*.hxx")
file(GLOB src_linker "src/linker/*.cxx" "src/linker/*.hxx")

file(GLOB src_testbench "src/testbench/*.cxx" "src/testbench/*.hxx")

include_directories("src/common")

add_executable(solomips-emu ${src_common} ${src_emulator})

add_executable(solomips-ld ${src_common} ${src_linker})

add_executable(solomips-test ${src_testbench})
