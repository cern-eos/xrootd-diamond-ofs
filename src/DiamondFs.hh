// ----------------------------------------------------------------------
// File: DiamondFs.hh
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


#ifndef __DIAMONDFS_API_H__
#define __DIAMONDFS_API_H__
#include "XrdOfs/XrdOfs.hh"
#include "XrdSys/XrdSysPthread.hh"

#include "DiamondFile.hh"
#include "DiamondDir.hh"

#include <map>
#include <vector>
#include <string>

struct XrdVersionInfo;

class DiamondFs : public XrdOfs {
private:
  //----------------------------------------------------------------------------
  //! TPC Functionality
  //----------------------------------------------------------------------------

  struct TpcInfo {
    std::string path;
    std::string opaque;
    std::string capability;
    std::string key;
    std::string src;
    std::string dst;
    std::string org;
    std::string lfn;
    time_t expires;
  };

protected:
  friend class DiamondFile;

  XrdSysMutex TpcMapMutex; //< a mutex protecting a Tpc Map
  typedef std::map<std::string, struct TpcInfo> tpc_info_map_t;
  typedef std::vector<tpc_info_map_t > tpc_map_t;

  tpc_map_t TpcMap; //< a vector map pointing from tpc key => tpc information for reads, [0] are readers [1] are writers

public:

  //----------------------------------------------------------------------------
  //! Object Allocation
  //----------------------------------------------------------------------------

  XrdSfsDirectory *
  newDir (char *user = 0, int MonID = 0) {
    return (XrdSfsDirectory *)new DiamondDir(user, MonID);
  }

  XrdSfsFile *
  newFile (char *user = 0, int MonID = 0) {
    return (XrdSfsFile *)new DiamondFile(user, MonID);
  }

  //----------------------------------------------------------------------------
  //! Overloaded Functions
  //----------------------------------------------------------------------------
  virtual int chksum (csFunc Func,
                      const char *csName,
                      const char *Path,
                      XrdOucErrInfo &out_error,
                      const XrdSecEntity *client = 0,
                      const char *opaque = 0);

  DiamondFs () {
    XrdOfs::XrdOfs();
    TpcMap.resize(2);
  }

  virtual ~DiamondFs ();

  virtual int Configure (XrdSysError &err);
  virtual int Configure (XrdSysError &err, XrdOucEnv *env);
  virtual int ConfigXeq (char *var, XrdOucStream &str, XrdSysError &err);

  const char* getVersion ();

};

extern DiamondFs DiamondFS;
#endif
