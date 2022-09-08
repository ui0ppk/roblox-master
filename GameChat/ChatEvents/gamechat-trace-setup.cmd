@echo off
if "%1"=="" goto help

echo.
@echo on
%SystemRoot%\system32\wevtutil.exe um Microsoft-Xbox-GameChat-Events.man
mkdir c:\temp
copy "%1\Microsoft.Xbox.GameChat.dll" c:\temp
%SystemRoot%\system32\wevtutil.exe im Microsoft-Xbox-GameChat-Events.man /rf:c:\temp\Microsoft.Xbox.GameChat.dll /mf:c:\temp\Microsoft.Xbox.GameChat.dll /pf:c:\temp\Microsoft.Xbox.GameChat.dll
@echo off

goto done
:help
echo.
echo Usage:
echo gamechat-trace-setup.cmd [path to Microsoft.Xbox.GameChat.dll]
echo.
:done