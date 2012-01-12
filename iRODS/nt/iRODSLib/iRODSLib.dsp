# Microsoft Developer Studio Project File - Name="iRODSLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=iRODSLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "iRODSLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "iRODSLib.mak" CFG="iRODSLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "iRODSLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "iRODSLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "iRODSLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../clientlib/include" /I "../clientlib/include/api" /I "../clientLib\src\md5" /I "..\server\include" /I "..\nt\include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "windows_platform" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "iRODSLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../clientlib/include" /I "../clientlib/include/api" /I "../clientLib\src\md5" /I "..\server\include" /I "..\nt\include" /D "_DEBUG" /D "_WIN32" /D "_MBCS" /D "_LIB" /D "windows_platform" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "iRODSLib - Win32 Release"
# Name "iRODSLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\clientLib\src\clientLogin.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\cpUtil.c
# End Source File
# Begin Source File

SOURCE=..\nt\src\dirent.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\getRodsEnv.c
# End Source File
# Begin Source File

SOURCE=..\nt\src\gettimeofday.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\getUtil.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\lsUtil.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\md5\md5c.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\md5\md5Checksum.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\miscUtil.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\mkdirUtil.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\obf.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\packStruct.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\parseCommandLine.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\procApiRequest.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\putUtil.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcAuthCheck.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcAuthRequest.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcAuthResponse.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcChkNVPathPerm.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcCollCreate.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\rcConnect.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataCopy.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataGet.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjClose.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjCopy.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjCreate.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjGet.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjLseek.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjOpen.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjPut.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjRead.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjRepl.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjUnlink.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataObjWrite.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcDataPut.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileChksum.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileChmod.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileClose.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileClosedir.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileCreate.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileFstat.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileFsync.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileGet.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileGetFsFreeSpace.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileLseek.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileMkdir.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileOpen.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileOpendir.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFilePut.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileRead.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileReaddir.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileRename.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileRmdir.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileStage.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileStat.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileUnlink.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcFileWrite.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcGeneralAdmin.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcGenQuery.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcGetMiscSvrInfo.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\rcMisc.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcModAccessControl.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcModAVUMetadata.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcModDataObjMeta.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\rcPortalOpr.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcRegColl.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcRegDataObj.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcRegReplica.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcRmColl.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcSimpleQuery.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\api\rcUnregDataObj.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\replUtil.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\rmUtil.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\rodsLog.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\rodsPath.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\sockComm.c
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\stringOpr.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\clientLib\include\apiHandler.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\apiHeaderAll.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\apiNumber.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\apiPackTable.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\apiTable.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\authCheck.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\authenticate.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\authRequest.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\authResponse.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\chkNVPathPerm.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\collCreate.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\cpUtil.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataCopy.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataGet.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjClose.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjCopy.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjCreate.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjGet.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjInpOut.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjLseek.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjOpen.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjPut.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjRead.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjRepl.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjUnlink.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataObjWrite.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\dataPut.h
# End Source File
# Begin Source File

SOURCE=..\nt\include\dirent.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileChksum.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileChmod.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileClose.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileClosedir.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileCreate.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileFstat.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileFsync.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileGet.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileGetFsFreeSpace.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileLseek.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileMkdir.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileOpen.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileOpendir.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\filePut.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileRead.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileReaddir.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileRename.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileRmdir.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileStage.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileStat.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileUnlink.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\fileWrite.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\generalAdmin.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\genQuery.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\getMiscSvrInfo.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\getRodsEnv.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\getUtil.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\md5\global.h
# End Source File
# Begin Source File

SOURCE=..\nt\include\IRodsLib3.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\lsUtil.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\src\md5\md5.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\md5Checksum.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\miscUtil.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\mkdirUtil.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\modAccessControl.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\modAVUMetadata.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\modDataObjMeta.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\obf.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\objInfo.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\packStruct.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\parseCommandLine.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\procApiRequest.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\putUtil.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rcConnect.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rcGlobal.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rcGlobalExtern.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rcMisc.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rcPortalOpr.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\regColl.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\regDataObj.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\regReplica.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\replUtil.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\rmColl.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rmUtil.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rods.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsClient.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsDef.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsError.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsErrorTable.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsGenQuery.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsKeyWdDef.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsLog.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsPackInstruct.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsPackTable.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsPath.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsType.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsUser.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\rodsVersion.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\simpleQuery.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\sockComm.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\stringOpr.h
# End Source File
# Begin Source File

SOURCE=..\nt\include\Unix2Nt.h
# End Source File
# Begin Source File

SOURCE=..\clientLib\include\api\unregDataObj.h
# End Source File
# End Group
# End Target
# End Project
