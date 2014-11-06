EOS Diamond XRootD OFS plugin
=======================================

This project contains a plugin for the **XRootD** server which provides the EOS Diamond
storage functionality.

It is published under the GPLv3 license: http://www.gnu.org/licenses/gpl-3.0.html

Configuration
=============

Add the plug-in configuration to the XRootD configuration file:

```
   xrootd.fslib /path/to/libdiamond_ofs.so
```

Enable adler32 checksumming in the XRootD configuration file:

```
   xrootd.chksum adler32
```

To enable thirdparty copy (TPC) add this line to the XRootD configuration file:

```
   ofs.tpc pgm /usr/bin/xrdcp
```

CGI Support
===========

The plug-in supports the following CGI tags "?diamond.<key1>=<value1>&diamond.<key2>=<value2>
```
   diamond.stripe=<size>
   <size> can be a plain number (bytes) or e.g. 1k,2K,3M,4m,1G,2g etc ...
   
   Example: "root://localhost//myfile?diamond.stripe=64M"
```
If the **diamond.stripe** is unspecified the internal default stripe size will be used.



