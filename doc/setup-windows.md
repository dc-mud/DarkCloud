# Documentation

### Setup for Windows (Visual Studio)

The current supported version is through Visual Studio 2017 Community (which is free)

#### Prerequisites

1. Windows 10
2. Visual Studio 2017 Community (Free)
3. Git (I use SourceTree as a Windows Git Client)

#### Setup Steps

1. Clone or download the git repository into the directory of yours choice (git clone https://github.com/cs-mud/CrimsonSkies cs-mud)
1. Open the "DarkCloud.sln" solution file (from Windows Explorer or from Visual Studio).
1. Click "Build" or the "Local Windows Debugger".  Build makes an executable, the debugger allows you to run it through Visual Studio.
1. Test a connection to your mud locally, execute "telnet localhost 4000" or "telnet 127.0.0.1 4000" and you should be greeted with the login menu.
    2. From the login menu, create a new character (this will be a level 1 character and we will edit your pfile to make you an immortal or game admin)
    3. After creating, edit the "../player/-your player name-" file.  Format is crucial in this file so just change the field values.  Change Level to 60 and Security to 10.  (Control-X to exit and save from Nano)
    4. telnet again to the game (telnet localhost 4000) and login again, you are an immortal at the maximum level (type 'wiz' to see all your new immortal commands)

#### Troubleshooting Commong Issues

- To fix: error E1696: cannot open source file "winsock2.h"
	- Open the Project -> DarkCloud Properties -> Configuration Properties -> General -> Set the Platform Toolset to v141_xp (or any versions that target xp, various may work)

- To fix: Project 'DarkCloud' could not be loaded because it's missing install components. To fix this launch Visual Studio setup with the following selections: Microsoft.VisualStudio.ComponentGroup.NativeDesktop.WinXP
	- In the Visual Studio Setup, Install "Windows XP support for C++".

- To fix: sprintf errors in VS
    - Project Properties -> C/C++ -> Preprocessor -> Preprocessor Defintions
    - Add Disable _CRT_SECURE_NO_DEPRECATE

- To fix: net errors add winsock:
    - Project Properties -> Linker -> Input -> Additional Dependencies
    - Add wsock32.lib

- To fix: Error C4996: 'unlink': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _unlink
	- Put _WIN32 directive in to call _unlink for Windows, link for anything else

- To fix: Error C4996: 'close': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _close
	- Put _WIN32 directive in to call _close for Windows, link for anything else

- To fix: error C1083: Cannot open include file: 'sys/time.h': No such file or directory
	- Put directives in all files for _WIN32 to point to time.h.  A pain but I want to do it in the code and not be copying
	  to the sys directory so this works out of the box for people who fork it.

- To fix: error C2079: 'now_time' uses undefined struct 'timeval'
	- Add _WIN32 include for winsock.h

- To fix: error C4996: 'write': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _write. See online help for details.
	- Add _WIN32 code and change write to _write
	- Alt fix, use send instead of write in Windows.

#### Additional Notes:

- crypt has been replaced with a sha256 hashing function for passwords.  Depending on the endianness of the machine, this could potentially return a different hash.
- rename which renames a file works differently in Windows/VS.  In most POSIX implementations you can rename a file to one that already exists and it will
  overwrite.  In Windows, it fails, which means stuff like pfiles never save after the first time.  In Windows the file you are renaming too must be deleted
  first (with _unlink).
- In order to run in Debug, a Visual Studio project properties change has to be made to tell VS to start in the same working directory as the executable.  Under
	  "Project->Properties->Working Directory" from "$(ProjectDir)" to "$(SolutionDir)$(Configuration)\".


[Back to Table of Contents](index.md)
