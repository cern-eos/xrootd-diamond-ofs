// ----------------------------------------------------------------------
// File: DiamondFs.cc
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

#include "DiamondFs.hh"
#include "XrdSfs/XrdSfsInterface.hh"
#include "XrdSys/XrdSysLogger.hh"
#include "XrdVersion.hh"
#include "XrdOuc/XrdOucString.hh"
#include "XrdOuc/XrdOucTrace.hh"
#include "XrdOfs/XrdOfsTrace.hh"
#include <zlib.h>

#include <sstream>

XrdOfs *XrdOfsFS = 0;

DiamondFs DiamondFS;

std::string diamond_ofs_log (const char* func, const char* file, int line, const char* msg, ...) 
{
  static XrdSysMutex lMutex;
  static char buffer[1024*1024];
  XrdSysMutexHelper lLock(lMutex);

  va_list args;
  va_start(args, msg);
  vsnprintf(buffer, 1024*1024, msg, args);

  std::stringstream s;
  std::string ffile = file;
  ffile.erase(0, ffile.rfind("/")+1);
  s << "[ " << ffile << ":" << line << " ] " << buffer;
  return s.str().c_str();
}


XrdVERSIONINFO (XrdSfsGetFileSystem, "diamondfs" VERSION);

extern "C"
XrdSfsFileSystem *
XrdSfsGetFileSystem (XrdSfsFileSystem *native_fs,
                     XrdSysLogger *lp,
                     const char *configfn,
                     XrdOucEnv *EnvInfo)
{
  extern XrdSysError OfsEroute;
  // No need to herald this as it's now the default filesystem
  //
  OfsEroute.SetPrefix("diamond_");
  OfsEroute.logger(lp);

  // Initialize the subsystems
  //
  XrdOfsFS = &DiamondFS;

  XrdOfsFS->ConfigFN = (configfn && *configfn ? strdup(configfn) : 0);
  if (XrdOfsFS->Configure(OfsEroute, EnvInfo)) return 0;

  // All done, we can return the callout vector to these routines.
  //
  return XrdOfsFS;
}

DiamondFs::~DiamondFs () { }

int
DiamondFs::Configure (XrdSysError &err)
{
  return XrdOfs::Configure(err);
}

int
DiamondFs::Configure (XrdSysError &err, XrdOucEnv *env)
{
  return XrdOfs::Configure(err, env);
}

int
DiamondFs::ConfigXeq (char *var, XrdOucStream &str, XrdSysError &err)
{
  return XrdOfs::ConfigXeq(var, str, err);
}

int
DiamondFs::chksum (csFunc Func,
                   const char *csName,
                   const char *path,
                   XrdOucErrInfo &error,
                   const XrdSecEntity *client,
                   const char *opaque)
{
  EPNAME("cksum");
  const char* tident = error.getErrUser();

  diamond_log("name=%s path=\"%s\"", csName, path);

  char buff[MAXPATHLEN + 8];
  int rc;

  XrdOucString CheckSumName = csName;
  unsigned int adler = adler32(0L, Z_NULL, 0);

  rc = 0;

  if (Func == XrdSfsFileSystem::csSize)
  {
    if (CheckSumName != "adler32")
    {
      strcpy(buff, csName);
      strcat(buff, "checksum - checksum not supported.");
      error.setErrInfo(ENOTSUP, buff);
      return SFS_ERROR;
    }
    else
    {
      // just return the length of an adler checksum
      error.setErrCode(4);
      return SFS_OK;
    }
  }

  if (!path)
  {
    strcpy(buff, csName);
    strcat(buff, "checksum - checksum path not specified.");
    error.setErrInfo(EINVAL, buff);
    return SFS_ERROR;
  }

  // ---------------------------------------------------------------------------
  // Now determine what to do
  // ---------------------------------------------------------------------------
  if ((Func == XrdSfsFileSystem::csCalc) ||
      (Func == XrdSfsFileSystem::csGet))
  {
  }
  else
  {
    error.setErrInfo(EINVAL, "checksum - invalid checksum function.");
    return SFS_ERROR;
  }

  // compute the checksum scrubbing this file

  DiamondFile* file = (DiamondFile*) newFile();
  if (file)
  {
    if (file->open(path, 0, 0, client, opaque))
    {
      error.setErrInfo(EINVAL, "checksum - unable to open file.");
      return SFS_ERROR;
    }
    char* buffer = (char*) malloc(4 * 1024 * 1024);
    off_t offset = 0;
    size_t chunksize = 4 * 1024 * 1024;
    size_t nread = 0;

    if (buffer)
    {
      do
      {
        nread = file->read(offset, buffer, chunksize);
        if (nread > 0)
        {
          adler = adler32(adler, (const Bytef*) buffer, nread);
          offset += nread;
        }
      }
      while (nread == chunksize);
      free(buffer);
    }
    else
    {
      error.setErrInfo(EINVAL, "checksum - buffer allocation failed.");
      return SFS_ERROR;
    }
  }
  else
  {
    error.setErrInfo(EINVAL, "checksum - file allocation failed.");
    return SFS_ERROR;
  }
  snprintf(buff, 9, "%08x", adler);
  error.setErrInfo(0, buff);
  return SFS_OK;
}

const char *
DiamondFs::getVersion ()
{

  static XrdOucString FullVersion = XrdVERSION;
  FullVersion += " DiamondFs ";
  FullVersion += VERSION;
  return FullVersion.c_str();
}

uint64_t 
DiamondFs::parseUnit(const char* instring)
{
  if (!instring)
  {
    errno = EINVAL;
    return 0;
  }
  
  XrdOucString sizestring = instring;
  errno = 0;
  unsigned long long convfactor;
  convfactor = 1ll;
  if (!sizestring.length())
  {
      errno = EINVAL;
      return 0;
  }

  if (sizestring.endswith("B") || sizestring.endswith("b"))
  {
    sizestring.erase(sizestring.length() - 1);
  }
  
  if (sizestring.endswith("E") || sizestring.endswith("e"))
  {
    convfactor = 1024ll * 1024ll * 1024ll * 1024ll * 1024ll * 1024ll;
  }

  if (sizestring.endswith("P") || sizestring.endswith("p"))
  {
    convfactor = 1024ll * 1024ll * 1024ll * 1024ll * 1024ll;
  }

  if (sizestring.endswith("T") || sizestring.endswith("t"))
  {
    convfactor = 1024ll * 1024ll * 1024ll * 1024ll;
  }

  if (sizestring.endswith("G") || sizestring.endswith("g"))
  {
    convfactor = 1024ll * 1024ll * 1024ll;
  }

  if (sizestring.endswith("M") || sizestring.endswith("m"))
  {
    convfactor = 1024ll * 1024ll;
  }

  if (sizestring.endswith("K") || sizestring.endswith("k"))
  {
    convfactor = 1024ll;
  }

  if (sizestring.endswith("S") || sizestring.endswith("s"))
  {
    convfactor = 1ll;
  }

  if ((sizestring.length() > 3) && (sizestring.endswith("MIN") || sizestring.endswith("min")))
  {
    convfactor = 60ll;
  }

  if (sizestring.endswith("H") || sizestring.endswith("h"))
  {
    convfactor = 3600ll;
  }

  if (sizestring.endswith("D") || sizestring.endswith("d"))
  {
    convfactor = 86400ll;
  }
  
  if (sizestring.endswith("W") || sizestring.endswith("w"))
  {
    convfactor = 7 * 86400ll;
  }
  
  if ((sizestring.length() > 2) && (sizestring.endswith("MO") || sizestring.endswith("mo")))
  {
    convfactor = 31 * 86400ll;
  }
  
  if (sizestring.endswith("Y") || sizestring.endswith("y"))
  {
    convfactor = 365 * 86400ll;
  }

  if (convfactor > 1)
    sizestring.erase(sizestring.length() - 1);
  
  if ((sizestring.find(".")) != STR_NPOS)
  {
    return ((unsigned long long) (strtod(sizestring.c_str(), NULL) * convfactor));
  }
  else
  {
    return (strtoll(sizestring.c_str(), 0, 10) * convfactor);
  }
}
