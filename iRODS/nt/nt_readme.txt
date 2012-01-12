======================================================================
Building Windows i-commands using MS studio
======================================================================
Two library files need to be built before building s-commands. They are: 
(1) iRODSLib.lib - iRODS library built from main iRODS code from core, 
(2) iRODSNtUtilLib.lib - Windows handy utility functions for porting    
    UNIX application to Windows.

The building sequence should be: iRODSLib.lib, iRODSNtUtilLib.lib, i-commands.

To build "iRODSLib.lib", open the .net project file in the "iRODSLib" 
directory and build the library file in the MS visual studio.

Take the similar procedure for building "iRODSNtUtilLib.lib".

To build i-commands, a "Makefile" was created in the "icmds" directory 
for batch build of all i-commands. Start a "Visual Studio Command 
Prompt" from the "Visual Studio Tools", which can be found in Visual 
Studio group of startup menu. Within the Visual Studio command prompt, 
change the current working directory to ...\IRODS\nt\icmds and then 
run the build command, nmake.

The i-commands, ils.exe, iget.exe, iput.exe, etc, can be found in the 
"Debug" directory.


======================================================================
Setting up iRODS environment file in Windows to run i-commands
======================================================================
The iRODS client requires setting up an env file, ".irodsEnv", 
which contains iRODS user name, iRODS server, and other info except 
password.

This file should be placed in the following directory (or folder).

%HOMEDRIVE%%HOMEPATH%\.irods

The %HOMEDRIVE%%HOMEPATH% is the home directory (folder) for a 
XP/Vista user. After launching the "Command Prompt", a user is 
automatically placed in this place.

The directory (folder) is usually like following:
C:\Documents and Settings\bzhu 
, where 'bzhu' is user name.

In this example, the env file has the following full path.
C:\Documents and Settings\bzhu\.irods\.irodsenv


To create env file:
1. Launch a "Command Prompt" by navigating the menu "Start" -> 
Accessories -> "Command Prompt".
2. Type the following Windows command to create a folder, ".irods".
   > md .irods
3. > cd .irods
4. > Notepad .irodEnv
     This will launch a Notepad and create a text file named 
   ".irodsEnv".), Put the content of iRODS envs into the Notepad and 
   click "Save".


======================================================================
Running i-commands
======================================================================
To run i-commands in any directory in a Windows machine, the path 
where i-commands reside should be set in the Windows PATH environment 
variable. This is the same concept as UNIX machine. To do so, launch 
the System dialogue via: 
Start -> settings -> control panel. 
Click the "System" icon. In the "Advanced" tab, click the "Environment 
variables" button. Add the path for i-commands in the "PATH" either in 
user category or the system category.



Please contact us via email at iROD-Chat@googlegroups.com if 
encountering any problem.


Bing Zhu, Ph.D.
SRB Team
San Diego Supercomputer Center
9500 Gilman Drive, MC 0505
La Jolla, CA 92093
email: bzhu@sdsc.edu
Phone: (858)534-8373
