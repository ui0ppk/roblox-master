@echo off
echo.
echo gamechat-trace-stop.cmd [optional address of console]
echo.
echo Using tracelog.exe to stop trace
xbrun /x %1/title /o tracelog.exe -stop gamechat

mkdir c:\temp
xbcp /x %1/title xd:\gamechat.etl c:\temp\gamechat%1.etl

set xperfpath="%ProgramFiles(x86)%\Windows Kits\8.1\Windows Performance Toolkit\xperf.exe" 
if NOT EXIST %xperfpath% set xperfpath="%ProgramFiles(x86)%\Windows Kits\8.0\Windows Performance Toolkit\xperf.exe" 
if NOT EXIST %xperfpath% goto help

%xperfpath% -symbols verbose -i c:\temp\gamechat%1.etl -o c:\temp\gamechat%1.csv

echo Open in c:\temp\gamechat%1.csv in Excel
start c:\temp\gamechat%1.csv

goto done

:help

echo.
echo Can not find xperf.exe.  Install xperf by installing the Windows Performance Toolkit that is found
echo inside the Windows SDK: http://msdn.microsoft.com/en-US/windows/desktop/aa904949.aspx
echo.

:done