EOS Diamond XRootD OFS plugin
=======================================

This project contains a plugin for the **XRootD** server which provides the EOS Diamond
storage functionality.

It is published under the GPLv3 license: http://www.gnu.org/licenses/gpl-3.0.html

Configuration
=============

Add the plug-in configuration to the XRootD configuration file:

  xrootd.fslib /path/to/libdiamond_ofs.so

Enable adler32 checksumming in the XRootD configuration file:

  xrootd.chksum adler32
