#
# config/directories.mk
#
# Set paths to directories used by some or all iRODS makefiles.
# All directory paths are relative to 'buildDir'.
#


# If not already set, set the build directory to the current directory
ifndef buildDir
export buildDir = $(CURDIR)
endif





#
# Source directories for servers and libraries
#
export svrCoreSrcDir	= $(buildDir)/server/core/src
export svrApiSrcDir	= $(buildDir)/server/api/src
export svrIcatSrcDir	= $(buildDir)/server/icat/src
export svrReSrcDir	= $(buildDir)/server/re/src
export svrDriversSrcDir	= $(buildDir)/server/drivers/src
export svrTestSrcDir	= $(buildDir)/server/test/src

export libCoreSrcDir	= $(buildDir)/lib/core/src
export libMd5SrcDir	= $(buildDir)/lib/md5/src
export libRbudpSrcDir	= $(buildDir)/lib/rbudp/src
export libApiSrcDir	= $(buildDir)/lib/api/src





#
# Include directories
#
export svrCoreIncDir	= $(buildDir)/server/core/include
export svrApiIncDir	= $(buildDir)/server/api/include
export svrIcatIncDir	= $(buildDir)/server/icat/include
export svrExtIcatIncDir	= $(buildDir)/modules/extendedICAT
export svrReIncDir	= $(buildDir)/server/re/include
export svrDriversIncDir	= $(buildDir)/server/drivers/include
export svrTestIncDir	= $(buildDir)/server/test/include

export libCoreIncDir	= $(buildDir)/lib/core/include
export libMd5IncDir	= $(buildDir)/lib/md5/include
export libRbudpIncDir	= $(buildDir)/lib/rbudp/include
export libApiIncDir	= $(buildDir)/lib/api/include





#
# Object directories
#
export svrCoreObjDir	= $(buildDir)/server/core/obj
export svrApiObjDir	= $(buildDir)/server/api/obj
export svrIcatObjDir	= $(buildDir)/server/icat/obj
export svrReObjDir	= $(buildDir)/server/re/obj
export svrDriversObjDir	= $(buildDir)/server/drivers/obj
export svrTestObjDir	= $(buildDir)/server/test/obj

export libCoreObjDir	= $(buildDir)/lib/core/obj
export libMd5ObjDir	= $(buildDir)/lib/md5/obj
export libRbudpObjDir	= $(buildDir)/lib/rbudp/obj
export libApiObjDir	= $(buildDir)/lib/api/obj





#
# Binary directories
#
export serverBinDir	= $(buildDir)/server/bin
export svrTestBinDir	= $(buildDir)/server/test/bin





#
# Other directories
#
export configDir	= $(buildDir)/config
export svrConfigDir	= $(buildDir)/server/config
export modulesDir	= $(buildDir)/modules
export perlScriptsDir	= $(buildDir)/scripts/perl
