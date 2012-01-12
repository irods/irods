Setting up environment file for running i-commands:
======================================================================
The iRODS client requires setting up an env file, ".irodsEnv", 
which contains iRODS user name, iRODS server, and other
info except password.

This file should be placed in the following directory (or folder).

%HOMEDRIVE%%HOMEPATH%\.irods

The %HOMEDRIVE%%HOMEPATH% is the home directory (folder) for a XP/Vista
user. After launching the "Command Prompt", a user is automatically
placed in this place.

The directory (folder) is like following:
C:\Documents and Settings\bzhu 
, where 'bzhu' is user name.

In this example, the env file has the following full path.
C:\Documents and Settings\bzhu\.irods\.irodsenv


To create env file:
1. Launch a "Command Prompt" by navigating the menu "Start" -> Accessories -> "Command Prompt".
2. Type the following Windows command to create a folder, ".irods".
   > md .irods
3. > cd .irods
4. > Notepad .irodsEnv
   (This will launch a Notepad and create a text file named ".irodsEnv".)
5. Put the irods content into the Notpad and click save.


Running i-commands
======================================================================
To run i-commands in any directory in a Windows machine, the path where i-commands reside 
should be set in the Windows PATH environment variable. This is the same concept as 
UNIX machine. To do so, launch the System dialogue via: 
Start -> settings -> control panel. 
Click the "System" icon.
In the "Advanced" tab, click the "Environment variables" button. Add the path for
i-commands in the "PATH" either in user category or the system category.
