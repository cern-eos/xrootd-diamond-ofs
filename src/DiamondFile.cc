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

#include <memory>

#include "DiamondFile.hh"
#include "DiamondFs.hh"

#include "XrdOuc/XrdOucEnv.hh"
#include "XrdOfs/XrdOfsTrace.hh"
#include "XrdSec/XrdSecEntity.hh"
#include "XrdSys/XrdSysTimer.hh"
#include "XrdNet/XrdNetAddrInfo.hh"
#include "XrdCl/XrdClFile.hh"

// poor man's debug just for the initial implementation
#define DIAMOND_DEBUG 1

int
DiamondFile::open (const char* path,
                   XrdSfsFileOpenMode open_mode,
                   mode_t create_mode,
                   const XrdSecEntity* client,
                   const char* opaque)
{
  EPNAME("open");
  // const char* tident = error.getErrUser();
  // ----------------------------------------------------------------------------
  // extract tpc keys
  // ----------------------------------------------------------------------------
  XrdOucEnv tmpOpaque(opaque ? opaque : "");

  XrdOucString stringOpaque = opaque ? opaque : "";
  XrdOucString Path = path;
  XrdOucString sec_protocol = client ? client->prot : "";

  isRW = (open_mode) ? true : false;

  std::string tpc_stage = tmpOpaque.Get("tpc.stage") ?
    tmpOpaque.Get("tpc.stage") : "";
  std::string tpc_key = tmpOpaque.Get("tpc.key") ?
    tmpOpaque.Get("tpc.key") : "";
  std::string tpc_src = tmpOpaque.Get("tpc.src") ?
    tmpOpaque.Get("tpc.src") : "";
  std::string tpc_dst = tmpOpaque.Get("tpc.dst") ?
    tmpOpaque.Get("tpc.dst") : "";
  std::string tpc_org = tmpOpaque.Get("tpc.org") ?
    tmpOpaque.Get("tpc.org") : "";
  std::string tpc_lfn = tmpOpaque.Get("tpc.lfn") ?
    tmpOpaque.Get("tpc.lfn") : "";

  if (tpc_stage == "placement")
  {
    tpcFlag = kTpcSrcCanDo;
  }

  if (tpc_key.length())
  {
    time_t now = time(NULL);
    if ((tpc_stage == "placement") ||
        (!DiamondFS.TpcMap[isRW].count(tpc_key.c_str())))
    {
      //........................................................................
      // Create a TPC entry in the TpcMap
      //........................................................................
      XrdSysMutexHelper tpcLock(DiamondFS.TpcMapMutex);
      if (DiamondFS.TpcMap[isRW].count(tpc_key.c_str()))
      {
        //......................................................................
        // TPC key replay go away
        //......................................................................
        return DiamondFS.Emsg(epname,
                              error,
                              EPERM, "open - tpc key replayed",
                              path);
      }
      if (tpc_key == "")
      {
        //......................................................................
        // TPC key missing
        //......................................................................
        return DiamondFS.Emsg(epname,
                              error,
                              EINVAL,
                              "open - tpc key missing",
                              path);
      }

      //........................................................................
      // Compute the tpc origin e.g. <name>:<pid>@<host.domain>
      //........................................................................


      std::string origin_host = client->addrInfo->Name();
      std::string origin_tident = client->tident;
      origin_tident.erase(origin_tident.find(":"));
      tpc_org = origin_tident;
      tpc_org += "@";
      tpc_org += origin_host;

      //........................................................................
      // Store the TPC initialization
      //........................................................................
      DiamondFS.TpcMap[isRW][tpc_key].key = tpc_key;
      DiamondFS.TpcMap[isRW][tpc_key].org = tpc_org;
      DiamondFS.TpcMap[isRW][tpc_key].src = tpc_src;
      DiamondFS.TpcMap[isRW][tpc_key].dst = tpc_dst;
      DiamondFS.TpcMap[isRW][tpc_key].path = path;
      DiamondFS.TpcMap[isRW][tpc_key].lfn = tpc_lfn;
      DiamondFS.TpcMap[isRW][tpc_key].opaque = opaque ? opaque : "";
      DiamondFS.TpcMap[isRW][tpc_key].expires = time(NULL) + 60; // one minute

      TpcKey = tpc_key.c_str();
      if (tpc_src.length())
      {
        // this is a destination session setup
        tpcFlag = kTpcDstSetup;
        if (!tpc_lfn.length())
        {
          return DiamondFS.Emsg(epname,
                                error,
                                EINVAL,
                                "open - tpc lfn missing",
                                path);
        }
      }
      else
      {
        // this is a source session setup
        tpcFlag = kTpcSrcSetup;
      }
      if (tpcFlag == kTpcDstSetup)
      {
        if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc dst session\" key=%s, "
                                  "org=%s, src=%s path=%s lfn=%s expires=%llu\n",
                                  DiamondFS.TpcMap[isRW][tpc_key].key.c_str(),
                                  DiamondFS.TpcMap[isRW][tpc_key].org.c_str(),
                                  DiamondFS.TpcMap[isRW][tpc_key].src.c_str(),
                                  DiamondFS.TpcMap[isRW][tpc_key].path.c_str(),
                                  DiamondFS.TpcMap[isRW][tpc_key].lfn.c_str(),
                                  (unsigned long long) DiamondFS.TpcMap[isRW][tpc_key].expires);
      }
      else
      {
        if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc src session\" key=%s, org=%s, "
                                  "dst=%s path=%s expires=%llu\n",
                                  DiamondFS.TpcMap[isRW][tpc_key].key.c_str(),
                                  DiamondFS.TpcMap[isRW][tpc_key].org.c_str(),
                                  DiamondFS.TpcMap[isRW][tpc_key].dst.c_str(),
                                  DiamondFS.TpcMap[isRW][tpc_key].path.c_str(),
                                  (unsigned long long) DiamondFS.TpcMap[isRW][tpc_key].expires);
      }
    }
    else
    {
      //........................................................................
      // Verify a TPC entry in the TpcMap
      //........................................................................

      // since the destination's open can now come before the transfer has been
      // setup we now have to give some time for the TPC client to deposit the
      // key the not so nice side effect is that this thread stays busy during
      // that time

      bool exists = false;

      for (size_t i = 0; i < 150; i++)
      {
        XrdSysMutexHelper tpcLock(DiamondFS.TpcMapMutex);
        if (DiamondFS.TpcMap[isRW].count(tpc_key))
          exists = true;
        if (!exists)
        {
          XrdSysTimer timer;
          timer.Wait(100);
        }
        else
        {
          break;
        }
      }

      XrdSysMutexHelper tpcLock(DiamondFS.TpcMapMutex);
      if (!DiamondFS.TpcMap[isRW].count(tpc_key))
      {
        return DiamondFS.Emsg(epname,
                              error,
                              EPERM,
                              "open - tpc key not valid",
                              path);
      }
      if (DiamondFS.TpcMap[isRW][tpc_key].expires < now)
      {
        return DiamondFS.Emsg(epname,
                              error,
                              EPERM,
                              "open - tpc key expired",
                              path);
      }

      // we trust 'sss' anyway and we miss the host name in the 'sss' entity
      if ((sec_protocol != "sss") &&
          (DiamondFS.TpcMap[isRW][tpc_key].org != tpc_org))
      {
        return DiamondFS.Emsg(epname,
                              error,
                              EPERM,
                              "open - tpc origin mismatch",
                              path);
      }
      //.........................................................................
      // Grab the open information
      //.........................................................................
      Path = DiamondFS.TpcMap[isRW][tpc_key].path.c_str();
      stringOpaque = DiamondFS.TpcMap[isRW][tpc_key].opaque.c_str();
      //.........................................................................
      // Expire TPC entry
      //.........................................................................
      DiamondFS.TpcMap[isRW][tpc_key].expires = (now - 10);

      // store the provided origin to compare with our local connection
      DiamondFS.TpcMap[isRW][tpc_key].org = tpc_org;
      // this must be a tpc read issued from a TPC target
      tpcFlag = kTpcSrcRead;
      TpcKey = tpc_key.c_str();
      if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc read\" key=%s, org=%s, src=%s, path=%s expires=%llu\n",
                                DiamondFS.TpcMap[isRW][tpc_key].key.c_str(),
                                DiamondFS.TpcMap[isRW][tpc_key].org.c_str(),
                                DiamondFS.TpcMap[isRW][tpc_key].src.c_str(),
                                DiamondFS.TpcMap[isRW][tpc_key].path.c_str(),
                                (unsigned long long) DiamondFS.TpcMap[isRW][tpc_key].expires);
    }
    //..........................................................................
    // Expire keys which are more than one 4 hours expired
    //..........................................................................
    XrdSysMutexHelper tpcLock(DiamondFS.TpcMapMutex);
    DiamondFs::tpc_info_map_t::iterator it = (DiamondFS.TpcMap[isRW]).begin();
    DiamondFs::tpc_info_map_t::iterator del = (DiamondFS.TpcMap[isRW]).begin();
    while (it != (DiamondFS.TpcMap[isRW]).end())
    {
      del = it;
      it++;
      if (now > (del->second.expires + (4 * 3600)))
      {
        if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"expire tpc key\" key=%s\n",
                                  del->second.key.c_str());

        DiamondFS.TpcMap[isRW].erase(del);
      }
    }
  }

  if (DIAMOND_DEBUG)fprintf(stderr, "Opening %s %s %x %x\n", Path.c_str(), stringOpaque.c_str(),
                            open_mode, create_mode);


  XrdOucEnv parseOpaque(stringOpaque.c_str());
  
  // deal and check stripesize parameters
  if (parseOpaque.Get("diamond.stripe")) {
    if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"parsing stripesize\" val=%s\n", parseOpaque.Get("diamond.stripe"));

    unsigned long long stripesize = DiamondFS.parseUnit(parseOpaque.Get("diamond.stripe"));
    if (errno) {
      return DiamondFS.Emsg(epname,
			    error,
			    EINVAL,
			    "open - illegal stripesize parameters",
			    path);      
    }
    stringOpaque.replace("rfs.stripe=","illegal=");
    std::stringstream sstream;
    sstream << "rfs.stripe=" << stripesize;
    if (!stringOpaque.endswith("&")) {
      stringOpaque+= "&";
    }
    stringOpaque += sstream.str().c_str();
    if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"modifying opaque\" val=%s\n", stringOpaque.c_str());
  }
  
  return XrdOfsFile::open(Path.c_str(),
                          open_mode,
                          create_mode,
                          client,
                          stringOpaque.c_str());
}
//----------------------------------------------------------------------------

int
DiamondFile::close ()
{
  //EPNAME("close");
  //const char* tident = error.getErrUser();

  //............................................................................
  // Any close on a file opened in TPC mode invalidates tpc keys

  if (TpcKey.length())
  {
    XrdSysMutexHelper tpcLock(DiamondFS.TpcMapMutex);
    if (DiamondFS.TpcMap[isRW].count(TpcKey.c_str()))
    {
      if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"remove tpc key\" key=%s\n", TpcKey.c_str());
      DiamondFS.TpcMap[isRW].erase(TpcKey.c_str());
    }
  }
  return XrdOfsFile::close();
}

//------------------------------------------------------------------------------
// Verify if a TPC key is still valid
//------------------------------------------------------------------------------

bool
DiamondFile::TpcValid ()
{
  // This call requires to have a lock like
  // 'XrdSysMutexHelper tpcLock(DiamondFS.TpcMapMutex)'
  if (TpcKey.length())
  {
    if (DiamondFS.TpcMap[isRW].count(TpcKey.c_str()))
    {
      return true;
    }
  }
  return false;
}

int
DiamondFile::sync ()
{
  const size_t kDefaultBlocksize = 4 * 1024 * 1024;

  if (tpcFlag == kTpcDstSetup)
  {
    if (tpcState == kTpcIdle)
    {
      if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc enabled - 1st sync\"\n");
      tpcState = kTpcEnabled;
      return SFS_OK;
    }

    if (tpcState == kTpcRun)
    {
      if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc already running - >2nd sync\"\n");
      return SFS_OK;
    }

    if (tpcState == kTpcDone)
    {
      if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc already finisehd - >2nd sync\"\n");
      return SFS_OK;
    }

    if (tpcState == kTpcEnabled)
    {
      tpcState = kTpcRun;
    }

    if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc now running - 2nd sync\"\n");
    std::string src_url = "";
    std::string src_cgi = "";
    {
      XrdSysMutexHelper tpcLock(DiamondFS.TpcMapMutex);
      //...........................................................................
      // The sync initiates the third party copy
      //...........................................................................
      if (!TpcValid())
      {
        tpcState = kTpcDone;
        if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc session invalidated during sync\"\n");
        error.setErrInfo(ECONNABORTED,
                         "sync - TPC session has been closed by disconnect");
        return SFS_ERROR;
      }
      // construct the source URL
      src_url = "root://";
      src_url += DiamondFS.TpcMap[isRW][TpcKey.c_str()].src;
      src_url += "/";
      src_url += DiamondFS.TpcMap[isRW][TpcKey.c_str()].lfn;

      // construct the source CGI
      src_cgi = "tpc.key=";
      src_cgi += TpcKey.c_str();
      src_cgi += "&tpc.org=";
      src_cgi += DiamondFS.TpcMap[isRW][TpcKey.c_str()].org;
    }

    XrdCl::File tpcIO; // the remote IO object

    if (DIAMOND_DEBUG)fprintf(stderr, "sync-url=%s sync-cgi=%s\n",
                              src_url.c_str(),
                              src_cgi.c_str());

    XrdCl::OpenFlags::Flags flags_xrdcl = XrdCl::OpenFlags::Read;
    XrdCl::Access::Mode mode_xrdcl = XrdCl::Access::None;

    std::string src_path = src_url.c_str();
    src_path += "?";
    src_path += src_cgi.c_str();

    XrdCl::XRootDStatus status =
      tpcIO.Open(src_path, flags_xrdcl, mode_xrdcl, 30);

    if (!status.IsOK())
    {
      XrdOucString msg = "sync - TPC open failed for url=";
      msg += src_url.c_str();
      msg += " cgi=";
      msg += src_cgi.c_str();
      error.setErrInfo(EFAULT, msg.c_str());
      tpcState = kTpcDone;
      return SFS_ERROR;
    }

    {
      //........................................................................
      // Re-Check validity of the TPC key
      //........................................................................
      XrdSysMutexHelper tpcLock(DiamondFS.TpcMapMutex);
      if (!TpcValid())
      {
        if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc session invalidated during sync\"\n");
        tpcState = kTpcDone;
        error.setErrInfo(ECONNABORTED,
                         "sync - TPC session has been closed by disconnect");
        return SFS_ERROR;
      }
    }

    uint32_t rbytes = 0;
    uint32_t wbytes = 0;
    off_t offset = 0;
    std::auto_ptr < std::vector<char> > buffer(new std::vector<char>(kDefaultBlocksize));

    if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc pull\" \n");

    do
    {
      //.........................................................................
      // Read the remote file in chunks and check after each chunk if the TPC
      // has been aborted already
      //.........................................................................
      status = tpcIO.Read(offset, kDefaultBlocksize, &((*buffer)[0]), rbytes, 30);
      if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc read\" rbytes=%u request=%lu\n",
                                rbytes,
                                kDefaultBlocksize);

      if (!status.IsOK())
      {
        tpcState = kTpcDone;
        if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc transfer terminated - remote read failed\"\n");
        error.setErrInfo(EIO,
                         "sync - tpc remote read failed");
        return SFS_ERROR;
      }

      if (rbytes > 0)
      {
        //.......................................................................
        // Write the buffer out through the local object
        //.......................................................................
        wbytes = DiamondFile::write(offset, &((*buffer)[0]), rbytes);
        if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc write\" offset=%llu rbytes=%u wbytes=%u\n", (unsigned long long)offset, rbytes, wbytes);
        if (rbytes != wbytes)
        {
          tpcState = kTpcDone;
          if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc transfer terminated - local write failed\"\n");
          error.setErrInfo(EIO,
                           "sync - tpc local write failed");
          return SFS_ERROR;
        }
        offset += rbytes;
      }

      {
        //.......................................................................
        // Re-Check validity of the TPC key
        //.......................................................................
        XrdSysMutexHelper tpcLock(DiamondFS.TpcMapMutex);
        if (!TpcValid())
        {
          tpcState = kTpcDone;
          if (DIAMOND_DEBUG)fprintf(stderr, "msg=\"tpc transfer invalidated during sync\"\n");
          error.setErrInfo(ECONNABORTED,
                           "sync - TPC session has been closed by disconnect");
          return SFS_ERROR;
        }
      }
    }
    while (rbytes > 0);

    // close the remote file, if no error close the local file
    status = tpcIO.Close(60);
    if (status.IsOK())
      return close();
    else
      return status.GetShellCode();
  }
  else
  {
    //...........................................................................
    // Standard file sync
    //...........................................................................
    return XrdOfsFile::sync();
  }
}

