# Makefile - This is the  makefile for the SRB client src

# The choices for 'BUILD_OPTION' is Debug or Release.

!IF  "$(CFG)" == "release"
BUILD_OPTION = Release
!ELSE
BUILD_OPTION = Debug
!ENDIF



srcDir=..\..\clients\icommands\src

TARGETS = $(binDir)\ils.exe \
	$(binDir)\iadmin.exe  \
	$(binDir)\icd.exe  \
	$(binDir)\ichksum.exe \
	$(binDir)\ichmod.exe \
	$(binDir)\icp.exe \
	$(binDir)\ienv.exe \
	$(binDir)\ierror.exe \
	$(binDir)\iexecmd.exe \
	$(binDir)\iexit.exe \
	$(binDir)\iget.exe \
	$(binDir)\iinit.exe \
	$(binDir)\ilsresc.exe \
	$(binDir)\imeta.exe \
	$(binDir)\imkdir.exe \
	$(binDir)\imv.exe \
	$(binDir)\iphymv.exe \
	$(binDir)\iput.exe \
	$(binDir)\ipwd.exe \
	$(binDir)\iqdel.exe \
	$(binDir)\iqmod.exe \
	$(binDir)\iqstat.exe \
	$(binDir)\iquest.exe \
	$(binDir)\irepl.exe \
	$(binDir)\irm.exe \
	$(binDir)\irmtrash.exe \
	$(binDir)\irsync.exe \
	$(binDir)\irule.exe \
	$(binDir)\isysmeta.exe \
	$(binDir)\itrim.exe \
	$(binDir)\iuserinfo.exe \
	$(binDir)\ireg.exe \
	$(binDir)\imcoll.exe \
	$(binDir)\imiscsvrinfo.exe


CC=cl.exe
LINKER=link.exe

DEBUG_CFLAGS= /Od  \
/I "..\..\server\drivers\include" /I "..\..\lib\core\include" /I "..\..\nt\include" \
/I "..\..\lib\md5\include" /I "..\..\lib\api\include" /I "..\..\server\core\include" \
/I "..\..\server\icat\include" /I "..\..\server\re\include" \
/D "_WIN32" /D "windows_platform" /D "_LARGEFILE_SOURCE" /D "_FILE_OFFSET_BITS=64" /D "_LARGEFILE64_SOURCE" \
/D "_MBCS" \
/Gm /EHsc /RTC1 /MTd /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt

RELEASE_CFLAGS = /O2 \
/I "..\..\server\drivers\include" /I "..\..\lib\core\include" /I "..\..\nt\include" /I "..\..\lib\md5\include" \
/I "..\..\lib\api\include" /I "..\..\server\core\include" \
/I "..\..\server\icat\include" /I "..\..\server\re\include" \
/D "_WIN32" /D "windows_platform" /D "_LARGEFILE_SOURCE" /D "_FILE_OFFSET_BITS=64" \
/D "_LARGEFILE64_SOURCE" /D "_MBCS" \
/GF /FD /EHsc /MT /W3 /nologo /c /Wp64 /TP /errorReport:prompt


!IF "$(BUILD_OPTION)" == "Debug"
CFLAGS = $(DEBUG_CFLAGS)
LINK_FLAG = /INCREMENTAL /nologo /debug /SUBSYSTEM:CONSOLE /machine:X86
binDir=Debug
!ELSE
CFLAGS = $(RELEASE_CFLAGS)
LINK_FLAG = /INCREMENTAL:NO /NOLOGO /SUBSYSTEM:CONSOLE /MACHINE:X86  \
            /ERRORREPORT:PROMPT /NODEFAULTLIB:"LIBCMT.lib"
binDir=Release
!ENDIF

ALL: "$(binDir)" $(TARGETS)
	@echo ""
	@echo "======================="
	@echo "The build is completed!"
	@echo "======================="

$(binDir):
	@if not exist "$(binDir)" mkdir "$(binDir)"

IRODS_LIBS=..\iRODSLib\$(BUILD_OPTION)\iRODSLib.lib ..\iRODSNtUtilLib\$(BUILD_OPTION)\iRODSNtUtilLib.lib

MSDEV_LIBS= ws2_32.lib advapi32.lib winmm.lib \
kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib \
oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

LIBS = $(IRODS_LIBS) $(MSDEV_LIBS) $(LINK_FLAG)


#-------------------------------------------------------------

$(binDir)\iadmin.exe: $(binDir)\iadmin.obj
	$(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iadmin.obj: $(srcDir)\iadmin.c
	$(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\icd.exe: $(binDir)\icd.obj
	$(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\icd.obj: $(srcDir)\icd.c
	$(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\ichksum.exe: $(binDir)\ichksum.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\ichksum.obj: $(srcDir)\ichksum.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\ichmod.exe: $(binDir)\ichmod.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\ichmod.obj: $(srcDir)\ichmod.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\icp.exe: $(binDir)\icp.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\icp.obj: $(srcDir)\icp.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\ienv.exe: $(binDir)\ienv.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\ienv.obj: $(srcDir)\ienv.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\ierror.exe: $(binDir)\ierror.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\ierror.obj: $(srcDir)\ierror.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iexecmd.exe: $(binDir)\iexecmd.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iexecmd.obj: $(srcDir)\iexecmd.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iexit.exe: $(binDir)\iexit.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iexit.obj: $(srcDir)\iexit.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iget.exe: $(binDir)\iget.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iget.obj: $(srcDir)\iget.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iinit.exe: $(binDir)\iinit.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iinit.obj: $(srcDir)\iinit.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\ils.exe: $(binDir)\ils.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\ils.obj: $(srcDir)\ils.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\ilsresc.exe: $(binDir)\ilsresc.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\ilsresc.obj: $(srcDir)\ilsresc.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\imeta.exe: $(binDir)\imeta.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\imeta.obj: $(srcDir)\imeta.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\imkdir.exe: $(binDir)\imkdir.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\imkdir.obj: $(srcDir)\imkdir.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\imv.exe: $(binDir)\imv.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\imv.obj: $(srcDir)\imv.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iphymv.exe: $(binDir)\iphymv.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iphymv.obj: $(srcDir)\iphymv.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iput.exe: $(binDir)\iput.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iput.obj: $(srcDir)\iput.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\ipwd.exe: $(binDir)\ipwd.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\ipwd.obj: $(srcDir)\ipwd.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iqdel.exe: $(binDir)\iqdel.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iqdel.obj: $(srcDir)\iqdel.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iqmod.exe: $(binDir)\iqmod.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iqmod.obj: $(srcDir)\iqmod.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iqstat.exe: $(binDir)\iqstat.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iqstat.obj: $(srcDir)\iqstat.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iquest.exe: $(binDir)\iquest.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iquest.obj: $(srcDir)\iquest.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\irepl.exe: $(binDir)\irepl.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\irepl.obj: $(srcDir)\irepl.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\irm.exe: $(binDir)\irm.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\irm.obj: $(srcDir)\irm.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\irmtrash.exe: $(binDir)\irmtrash.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\irmtrash.obj: $(srcDir)\irmtrash.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\irsync.exe: $(binDir)\irsync.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\irsync.obj: $(srcDir)\irsync.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\irule.exe: $(binDir)\irule.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\irule.obj: $(srcDir)\irule.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\isysmeta.exe: $(binDir)\isysmeta.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\isysmeta.obj: $(srcDir)\isysmeta.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\itrim.exe: $(binDir)\itrim.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\itrim.obj: $(srcDir)\itrim.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\iuserinfo.exe: $(binDir)\iuserinfo.obj
        $(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\iuserinfo.obj: $(srcDir)\iuserinfo.c
        $(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\ireg.exe: $(binDir)\ireg.obj
	$(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\ireg.obj: $(srcDir)\ireg.c
	$(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\imcoll.exe: $(binDir)\imcoll.obj
	$(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\imcoll.obj: $(srcDir)\imcoll.c
	$(CC) $(CFLAGS) /Fo$@ $?

$(binDir)\imiscsvrinfo.exe: $(binDir)\imiscsvrinfo.obj
	$(LINKER) /OUT:$@ $? $(LINK_FLAG) $(IRODS_LIBS) $(MSDEV_LIBS)

$(binDir)\imiscsvrinfo.obj: $(srcDir)\imiscsvrinfo.c
	$(CC) $(CFLAGS) /Fo$@ $?


!IF "$(BUILD_OPTION)" == "Debug"
clean:
	del $(binDir)\*.obj
	del $(binDir)\*.exe
	del $(binDir)\*.ilk
	del $(binDir)\*.pdb
!ELSE
clean:
	del $(binDir)\*.obj
	del $(binDir)\*.exe
	del $(binDir)\*.manifest
!ENDIF
