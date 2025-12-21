# LayraPS4 Emulator Project

SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project

SPDX-License-Identifier: GPL-2.0-or-later
cmakeminimumrequired(VERSION 3.22.1)
project(LayraPS4 CXX)

set(CMAKECXXSTANDARD 20)
set(CMAKECXXSTANDARDREQUIRED ON)

Find SDL2 and its required libraries
 findpackage(SDL2 REQUIRED)
findpackage(SDL2image REQUIRED)
findpackage(SDL2package(Vulkan REQUIRED) findpackage(toml11 REQUIRED) file(GLOBRECURSE LAYRAPS4SOURCES
 "src/.cpp"
 "src/.h"
 "src/common/.cpp"
 "src/common/.h"
 "src/core/.cpp"
 "src/core/.h"
 "src/core/networking/.cpp"
 "src/core/networking/.h"
 "src/core/ libraries/network/.cpp"
 "src/core/libraries/network/.h"
 "src/core/libraries/np/.cpp"
 "src/core/libraries/np/.h"
) includedirectories(
 src
 ${SDL2INCLUDEDIRS}
 ${VulkanINCLUDEDIRS}
 ${ImGui INCLUDEDIRS}
 ${toml11INCLUDEexecutable(LayraPS4 ${LAYRAPS4linklibraries(LayraPS4
 PRIVATE
 SDL2::SDL2
 SDL2::SDL2image
 SDL2::SDL2targetproperties(LayraPS4 PROPERTIES RUNTIMEOUTPUTDIRECTORY "${CMAKEBINARYcustomtarget(VFS PLACEHOLDER ALL)ttf REQUIRED)
Find Vulkan SDK

find
Find ImGui dependencies
package(ImGui REQUIRED)
Find TOML library

find
Define source files
Add include directories
DIRS}
)
Add the executable

addSOURCES})
Link libraries

targetttf
    Vulkan::Vulkan
    ImGui::ImGui
    toml11::toml11
)
Set output directory

setDIR}/bin")
Add a placeholder for VFS implementation

This will be replaced with actual VFS code in a later phase

add
