==================================================================================
== Instruments for gathering and comparing the settings on a tested environment ==
==================================================================================

I. Binaries used.
   1. SettingsReader.Web.exe : a Roblox in-house tool written by Yuri Belotserkovsky that gets the settings values from the database on the given instance (sitetest1/2/3 in case of web settings, gametest* in case of client FastFlags)
      * CommandLine.dll is a dependency for the tool, it also uses Newtonsoft.Json.dll that is copied from Automation\External\DLLs
   2. diff.exe : a GNU open-source diff tool implementation for Windows, provides a well-formatted and very readable output of the difference between 2 files. 
      * Source: http://www.gnu.org/software/diffutils/
      * libiconv2.dll, libintl3.dll are dependencies for diff.exe, source is the same: http://www.gnu.org/software/diffutils/

II. Syntax (as used in TeamCity)
> GetSettings.bat <SERVER> <SETTINGS FILE> <PREVIOUS SETTINGS> <DIFF FILE>
  * <SERVER> 		: the name of the instance the tests are run on, specified in build parameters on TC
  * <SETTINGS FILE>     : the file where the settings are stored; is compared to previous settings file and then exported to build artifacts
  * <PREVIOUS SETTINGS> : the settings from previous build, fetched from previous build's artifacts; compared with current settings dump
  * <DIFF FILE>		: the file where the diff output is stored; exported to artifacts
