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
#include <zlib.h>

XrdOfs *XrdOfsFS = 0;


XrdVERSIONINFO(XrdSfsGetFileSystem,"diamondfs" VERSION );

extern "C"
XrdSfsFileSystem *
XrdSfsGetFileSystem (XrdSfsFileSystem *native_fs,
                     XrdSysLogger *lp,
                     const char *configfn,
		     XrdOucEnv        *EnvInfo)
{
  extern XrdSysError OfsEroute;
  static DiamondFs DiamondFS;

  // No need to herald this as it's now the default filesystem
  //
  OfsEroute.SetPrefix("diamond_");
  OfsEroute.logger(lp);

  // Initialize the subsystems
  //
  XrdOfsFS = &DiamondFS;
  XrdOfsFS->ConfigFN = (configfn && *configfn ? strdup(configfn) : 0);
  if ( XrdOfsFS->Configure(OfsEroute, EnvInfo) ) return 0;
  
  // All done, we can return the callout vector to these routines.
  //
  return XrdOfsFS;
}

DiamondFs::~DiamondFs() {}

int 
DiamondFs::Configure(XrdSysError &err ) 
{ 
  XrdOfs::Configure(err); 
}

int            
DiamondFs::Configure(XrdSysError &err, XrdOucEnv *env) 
{
  XrdOfs::Configure(err,env); 
}

int   
DiamondFs::ConfigXeq(char *var, XrdOucStream &str, XrdSysError &err) 
{
  XrdOfs::ConfigXeq(var,str,err);
}

int
DiamondFs::chksum(csFunc Func,
		  const char             *csName,
		  const char             *path,
		  XrdOucErrInfo          &error,
		  const XrdSecEntity     *client,
		  const char             *opaque)
{
  static const char *epname = "chksumbla";
  const char *tident = error.getErrUser();

  char buff[MAXPATHLEN + 8];
  int rc;

  XrdOucString CheckSumName = csName;
  unsigned int adler = adler32(0L, Z_NULL, 0);

  rc = 0;

  if (Func == XrdSfsFileSystem::csSize) {
    if (CheckSumName != "adler32") {
      strcpy(buff, csName);
      strcat(buff, "checksum - checksum not supported.");
      error.setErrInfo(ENOTSUP, buff);
      return SFS_ERROR;
    } else {
      // just return the length of an adler checksum
      error.setErrCode(4);
      return SFS_OK;
    }
  }

  if (!path) {
    strcpy(buff, csName);
    strcat(buff, "checksum - checksum path not specified.");
    error.setErrInfo(EINVAL, buff);
    return SFS_ERROR;
  }

  // ---------------------------------------------------------------------------
  // Now determine what to do
  // ---------------------------------------------------------------------------
  if ((Func == XrdSfsFileSystem::csCalc) ||
      (Func == XrdSfsFileSystem::csGet)) {
  } else {
    error.setErrInfo(EINVAL, "checksum - invalid checksum function.");
    return SFS_ERROR;
  }

  // compute the checksum scrubbing this file

  DiamondFile* file = (DiamondFile*) newFile();
  if (file) {
    if (file->open(path, 0, 0, client, opaque)) {
      error.setErrInfo(EINVAL, "checksum - unable to open file.");
      return SFS_ERROR;
    }
    char* buffer = (char*)malloc(4*1024*1024);
    off_t offset = 0 ;
    size_t chunksize = 4*1024*1024;
    size_t nread = 0;

    if (buffer) {
      do {
	nread = file->read(offset, buffer, chunksize);
	if (nread>0) {
	  adler = adler32(adler, (const Bytef*) buffer, nread);
	  offset+=nread;
	}
      } while (nread == chunksize);
      free(buffer);
    } else {
      error.setErrInfo(EINVAL, "checksum - buffer allocation failed.");
      return SFS_ERROR;
    }
  } else {
    error.setErrInfo(EINVAL, "checksum - file allocation failed.");
    return SFS_ERROR;
  }
  snprintf(buff,9, "%08x",adler);
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
