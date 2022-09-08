@echo off

call :syncfolder glsl-optimizer
call :syncfolder hlsl2glslfork
call :syncfolder mojoshader
goto :eof

:syncfolder
if exist %1 (
    git --git-dir=%1/.git pull -v
) else (
    git clone https://github.com/zeux/%1.git
)
goto :eof
