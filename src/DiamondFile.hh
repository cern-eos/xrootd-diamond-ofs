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
#include "XrdOuc/XrdOucString.hh"

class DiamondFile : public XrdOfsFile {
private:
  bool isRW;

public:
  using XrdSfsFile::fctl;

  DiamondFile (const char *user, int MonID) : XrdOfsFile (user, MonID),
  isRW (false) {
    tpcFlag = kTpcNone;
    tpcState = kTpcIdle;
  }

  ~DiamondFile () { }

  //----------------------------------------------------------------------------
  //! Overloaded Functions
  //----------------------------------------------------------------------------
  int open (const char* path,
            XrdSfsFileOpenMode open_mode,
            mode_t create_mode,
            const XrdSecEntity* client,
            const char* opaque);
  //----------------------------------------------------------------------------
  int close ();
  //----------------------------------------------------------------------------
  int sync ();
  //----------------------------------------------------------------------------

  //----------------------------------------------------------------------------
  //! TPC Functionality
  //----------------------------------------------------------------------------
  //! Check if the TpcKey is still valid e.g. member of gOFS.TpcMap
  //----------------------------------------------------------------------------
  bool
  TpcValid ();
  //----------------------------------------------------------------------------
  XrdOucString TpcKey; //! TPC key for a tpc file operation
  //----------------------------------------------------------------------------

  enum {
    kTpcNone = 0, //! no TPC access
    kTpcSrcSetup = 1, //! access setting up a source TPC session
    kTpcDstSetup = 2, //! access setting up a destination TPC session
    kTpcSrcRead = 3, //! read access from a TPC destination
    kTpcSrcCanDo = 4, //! read access to evaluate if source available
  };
  //----------------------------------------------------------------------------
  int tpcFlag; //! uses kTpcXYZ enums above to identify TPC access
  //----------------------------------------------------------------------------

  enum {
    kTpcIdle = 0, //! TPC is not enabled and not running (no sync received)
    kTpcEnabled = 1, //! TPC is enabled, but not running (1st sync received)
    kTpcRun = 2, //! TPC is running (2nd sync received)
    kTpcDone = 3, //! TPC has finished
  };
  //----------------------------------------------------------------------------
  int tpcState; //! uses kTPCXYZ enumgs above to tag the TPC state
  //----------------------------------------------------------------------------
};

#endif
