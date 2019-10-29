#
# Dockerfile
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

FROM debian:buster

RUN apt-get update \
    && apt-get install -y --no-install-recommends gcc-mips-linux-gnu \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
