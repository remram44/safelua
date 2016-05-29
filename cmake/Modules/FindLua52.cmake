# - Find Lua 5.2
# Find the Lua 5.2 libraries
#
#  This module defines the following variables:
#     LUA52_FOUND        - True if LUA52_INCLUDE_DIR & LUA52_LIBRARY are found
#     LUA52_LIBRARIES    - Set when LUA52_LIBRARY is found
#     LUA52_INCLUDE_DIRS - Set when LUA52_INCLUDE_DIR is found
#
#     LUA52_INCLUDE_DIR - where to find lua.h and lauxlib.h.
#     LUA52_LIBRARY     - the Lua 5.2 library
#

#=============================================================================
# Adapted from FindALSA.cmake by Remi Rampin, 2016.
#=============================================================================
# Copyright 2009-2011 Kitware, Inc.
# Copyright 2009-2011 Philip Lowman <philip@yhbt.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(LUA52_INCLUDE_DIR NAMES lua.h lauxlib.h
    PATH_SUFFIXES lua-5.2 lua5.2 lua52
    DOC "The Lua 5.2 include directory"
)

find_library(LUA52_LIBRARY NAMES lua-5.2 lua5.2 lua52
    PATH_SUFFIXES lua-5.2 lua5.2 lua52
    DOC "The Lua 5.2 library"
)

# handle the QUIETLY and REQUIRED arguments and set LUA52_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Lua52
    REQUIRED_VARS LUA52_LIBRARY LUA52_INCLUDE_DIR)

if(LUA52_FOUND)
    set( LUA52_LIBRARIES ${LUA52_LIBRARY} )
    set( LUA52_INCLUDE_DIRS ${LUA52_INCLUDE_DIR} )
endif()

mark_as_advanced(LUA52_INCLUDE_DIR LUA52_LIBRARY)
