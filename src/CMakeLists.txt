# ----------------------------------------------------------------------
# File: src/CMakeList.txt
# Author: Andreas-Joachim Peters - CERN
# ----------------------------------------------------------------------

#/************************************************************************
# * EOS DIAMOND - the CERN Disk Storage System                           *
# * Copyright (C) 2014 CERN/Switzerland                                  *
# *                                                                      *
# * This program is free software: you can redistribute it and/or modify *
# * it under the terms of the GNU General Public License as published by *
# * the Free Software Foundation, either version 3 of the License, or    *
# * (at your option) any later version.                                  *
# *                                                                      *
# * This program is distributed in the hope that it will be useful,      *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
# * GNU General Public License for more details.                         *
# *                                                                      *
# * You should have received a copy of the GNU General Public License    *
# * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
# ************************************************************************/

find_package( XRootD REQUIRED )
find_package( z REQUIRED )

add_library( diamond_ofs SHARED
             DiamondFs.cc 
             DiamondFile.cc 
             DiamondDir.cc 
)

include_directories( ${XROOTD_INCLUDE_DIR} ${XROOTD_INCLUDE_DIR}/private ${Z_INCLUDES} )

add_definitions( -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DVERSION="${VERSION-MAJOR}.${VERSION_MINOR}.${VERSION_PATH}" )

set(CMAKE_CXX_FLAGS "-O2 -g -Wall -Wno-deprecated-declarations -msse4.2 -std=gnu++0x ")


target_link_libraries( diamond_ofs XrdCl ${Z_LIBRARIES})

if( Linux )
  set_target_properties( dimaond_ofs PROPERTIES
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
  )
endif( Linux )

install( TARGETS diamond_ofs LIBRARY DESTINATION ${LIB_INSTALL_DIR} )
