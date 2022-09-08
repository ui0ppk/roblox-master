rem File to generate all flavors of the bootstrapper

call:generateFlavorFunc Gametest setup-gametest.roblox.com gametest.roblox.com %1
call:generateFlavorFunc Gametest1 setup.gametest1.robloxlabs.com www.gametest1.robloxlabs.com %1
call:generateFlavorFunc Gametest2 setup.gametest2.robloxlabs.com www.gametest2.robloxlabs.com %1
call:generateFlavorFunc Gametest3 setup.gametest3.robloxlabs.com www.gametest3.robloxlabs.com %1
call:generateFlavorFunc Gametest4 setup.gametest4.robloxlabs.com www.gametest4.robloxlabs.com %1
call:generateFlavorFunc Gametest5 setup.gametest5.robloxlabs.com www.gametest5.robloxlabs.com %1
call:generateFlavorFunc Sitetest setup-sitetest.roblox.com sitetest.roblox.com %1
call:generateFlavorFunc Sitetest1 setup.sitetest1.robloxlabs.com www.sitetest1.robloxlabs.com %1
call:generateFlavorFunc Sitetest2 setup.sitetest2.robloxlabs.com www.sitetest2.robloxlabs.com %1
call:generateFlavorFunc Sitetest3 setup.sitetest3.robloxlabs.com www.sitetest3.robloxlabs.com %1
call:generateFlavorFunc Sitetest4 setup.sitetest4.robloxlabs.com www.sitetest4.robloxlabs.com %1

goto:eof

:generateFlavorFunc
echo Generating bootstrappers for %~4

rmdir /S /Q ..\..\UploadBits\Win32-%~4%~1-BootstrapperClient\ 
rmdir /S /Q ..\..\UploadBits\Win32-%~4%~1-BootstrapperQTStudio\ 
rmdir /S /Q ..\..\UploadBits\Win32-%~4%~1-BootstrapperRccService\

xcopy /Y /S ..\..\UploadBits\Win32-%~4-BootstrapperClient\* ..\..\UploadBits\Win32-%~4%~1-BootstrapperClient\ 
xcopy /Y /S ..\..\UploadBits\Win32-%~4-BootstrapperQTStudio\* ..\..\UploadBits\Win32-%~4%~1-BootstrapperQTStudio\ 
xcopy /Y /S ..\..\UploadBits\Win32-%~4-BootstrapperRccService\* ..\..\UploadBits\Win32-%~4%~1-BootstrapperRccService\

Resources\rtc.exe /plhd01="%~2" /plhd02="%~3" /plhd03="..\..\UploadBits\Win32-%~4%~1-BootstrapperClient\RobloxPlayerLauncher.exe" /plhd04="..\..\UploadBits\Win32-%~4%~1-BootstrapperClient\RobloxPlayerLauncher.exe" /F:"Resources\updateBootstrapperRC.rts"
Resources\rtc.exe /plhd01="%~2" /plhd02="%~3" /plhd03="..\..\UploadBits\Win32-%~4%~1-BootstrapperQTStudio\RobloxStudioLauncherBeta.exe" /plhd04="..\..\UploadBits\Win32-%~4%~1-BootstrapperQTStudio\RobloxStudioLauncherBeta.exe" /F:"Resources\updateBootstrapperRC.rts"
Resources\rtc.exe /plhd01="%~2" /plhd02="%~3" /plhd03="..\..\UploadBits\Win32-%~4%~1-BootstrapperRccService\Roblox.exe" /plhd04="..\..\UploadBits\Win32-%~4%~1-BootstrapperRccService\Roblox.exe" /F:"Resources\updateBootstrapperRC.rts"

goto:eof

