// ----------------------------------------------------------------------
// File: DiamondFile.hh
// Author: Andreas-Joachim Peters - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * EOS DIAMOND - the CERN Disk Storage System                           *
 * Copyright (C) 2014 CERN/Switzerland                                  *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 ************************************************************************/

#ifndef __DIAMONDFILE_API_H__
#define __DIAMONDFILE_API_H__
#include "XrdOfs/XrdOfs.hh"

class DiamondFile : public XrdOfsFile
{
public:
  using          XrdSfsFile::fctl;
  
  DiamondFile(const char *user, int MonID) : XrdOfsFile(user,MonID) {}

  
  ~DiamondFile() {}
};

#endif
