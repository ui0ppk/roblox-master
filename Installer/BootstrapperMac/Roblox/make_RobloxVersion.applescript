on run argv
	set BuildConfig to (item 1 of argv)
	set AppType to (item 2 of argv)
	-- your script using the parameters follows
	tell application "Finder"
		set myFolder to (container of (path to me)) as text
		set thePListPath to POSIX path of (myFolder & "build:" & BuildConfig & ":" & AppType & ".app:Contents:Info.plist")
		set VersionFile to POSIX path of (myFolder & "build:" & BuildConfig & ":" & AppType & "Version.txt")
		
		tell application "System Events"
			tell property list file thePListPath
				tell contents
					set bVersion to value of property list item "CFBundleVersion"
					
				end tell
			end tell
		end tell
		try
			open for access VersionFile with write permission
			set eof of VersionFile to 0
			write bVersion to VersionFile starting at eof
			close access VersionFile
		on error
			try
				close access VersionFile
			end try
		end try
	end tell
end run
