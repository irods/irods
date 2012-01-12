@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by INQUISITOR.HPJ. >"hlp\Inquisitor.hm"
echo. >>"hlp\Inquisitor.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\Inquisitor.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\Inquisitor.hm"
echo. >>"hlp\Inquisitor.hm"
echo // Prompts (IDP_*) >>"hlp\Inquisitor.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\Inquisitor.hm"
echo. >>"hlp\Inquisitor.hm"
echo // Resources (IDR_*) >>"hlp\Inquisitor.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\Inquisitor.hm"
echo. >>"hlp\Inquisitor.hm"
echo // Dialogs (IDD_*) >>"hlp\Inquisitor.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\Inquisitor.hm"
echo. >>"hlp\Inquisitor.hm"
echo // Frame Controls (IDW_*) >>"hlp\Inquisitor.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\Inquisitor.hm"
REM -- Make help for Project INQUISITOR


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\Inquisitor.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\Inquisitor.hlp" goto :Error
if not exist "hlp\Inquisitor.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\Inquisitor.hlp" Debug
if exist Debug\nul copy "hlp\Inquisitor.cnt" Debug
if exist Release\nul copy "hlp\Inquisitor.hlp" Release
if exist Release\nul copy "hlp\Inquisitor.cnt" Release
echo.
goto :done

:Error
echo hlp\Inquisitor.hpj(1) : error: Problem encountered creating help file

:done
echo.
