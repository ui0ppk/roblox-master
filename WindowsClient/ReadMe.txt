============================================================================

Helpful info:

Run the player from the command-line with the --help parameter to learn the
command-line parameters.  To easily connect to a place use -id followed by a
place id (such as 1818 for crossroads).  To spoof MD5 hash use -md5, please note
this will NOT work on release builds.

To test local scripts use -adminScripts, also will not work on release builds.

============================================================================
Security

Continually review and implement any missing security code:

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/ietechcol/cols/dnexpie/activex_security.asp

============================================================================
Settings


----------------------------------------------------------------------------
Default locations:

AppSettings.xml:           <<Module directory>> (e.g. sibling file to %SystemDrive%/Program Files (x86)/Roblox/Versions/virsion-[####]/RobloxApp.exe)

Content Folder:            <<Module directory>>\Content

Cache Folder:              CSIDL_COMMON_APPDATA\Roblox\Cache\

Log files:                 CSIDL_LOCAL_APPDATA\Roblox\logs\

Alternate log files:       CSIDL_COMMON_APPDATA\Roblox\logs\

User IDESettings.xml:      CSIDL_LOCAL_APPDATA\Roblox\


----------------------------------------------------------------------------
Determining BaseUrl (in order of priority):

    1) AppSettings.xml:       Settings->BaseUrl

	
----------------------------------------------------------------------------
Determining the content folder (in order of priority):

	1) Command-line (--content)

    2) AppSettings.xml:   Settings->ContentFolder  (might be a relative path w.r.t. Module)
    
    3) <<Default location>> (see "Default Location" section above)


----------------------------------------------------------
Determining CrashReport (in order of priority):

	1) HKEY_CURRENT_USER\Software\ROBLOX Corporation\Roblox	CrashReport (REG_DWORD)
	
	2) TRUE


----------------------------------------------------------
Determining SilentCrashReport (in order of priority):

	1) HKEY_CURRENT_USER\Software\ROBLOX Corporation\Roblox	SilentCrashReport (REG_DWORD)
	
    2) AppSettings.xml:   Settings->SilentCrashReport

	3) FALSE


