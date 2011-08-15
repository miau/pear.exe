pear.exe
========

In the Windows environment, pear command uses %SystemRoot%\pear.ini as a
default configuration file. Since it is a system global file, pear.ini
is corrupted when there are some installation of php/pear.

Once you put pear.exe at the same directory of pear.bat, each pear
command use independent configuration file.

I carefully implements pear.exe not to depend on C Run-Time Libraries
(CRT). Not depending on msvcrXXX.dll, it doesn't require any version of
VC++ Redistributable Packages to be installed.

Implementation
--------------

pear.exe just do two things below.

1. Set %PHP_PEAR_SYSCONF_DIR% to parent directory of pear.exe, if not set
2. Call pear.bat that exists at the same directory of pear.exe

When you type "pear", pear.exe is executed prior to pear.bat because of
%PATHEXT% setting.
