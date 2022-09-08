@echo off

b2 stage ^
-a ^
--reconfigure ^
--toolset=msvc-14.0 ^
--variant=release ^
-s ZLIB_SOURCE=D:\Roblox\Source\Library\zlib\include\zlib ^
-s ZLIB_BINARY=D:\Roblox\Source\Library\zlib\lib\release ^
--prefix=D:\Roblox\Source\Library\boost ^
--libdir=D:\Roblox\Source\Library\boost\lib ^
--includedir=D:\Roblox\Source\Library\boost\include

PAUSE