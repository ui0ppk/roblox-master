@ECHO OFF

IF %1.==. GOTO ParametersMissing
IF %2.==. GOTO ParametersMissing
IF %3.==. GOTO ParametersMissing
IF %4.==. GOTO ParametersMissing

DEL %2

SettingsReader.exe extract -S%1 -FUnified -IApiClient -O%2 --no-timer
GOTO End

:ParametersMissing
ECHO Error! Either the server name or the output file name is not defined!
ECHO Usage: %0 [server name] [settings dump file name] [old settings dump file name] [diff file name]
exit 2

:End
IF NOT EXIST %2 (
  ECHO Settings extraction failed for %1
  ECHO Settings extraction failed for %1 > %4
  ECHO Copying settings from previous build to %2 >> %4
  COPY NUL %2
  IF EXIST %3 XCOPY /Y %3 %2
)

IF EXIST %3 (
  ECHO ^< Current settings ^(%2^) > %4
  ECHO --- >> %4
  ECHO ^> Previous settings ^(%3^) >> %4
  ECHO Difference: >> %4
  diff.exe %2 %3 >> %4
)

IF EXIST %4 TYPE %4
