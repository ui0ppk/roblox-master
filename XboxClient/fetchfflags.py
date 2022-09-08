'''
This small tool fetches fastflags from prod and prints to standard output.

Usage:

python fetchfflags.py

'''

import sys;
import urllib;

test = "http://clientsettings.api.roblox.com/Setting/QuietGet/ClientAppSettings/?apiKey=76E5A40C-3AE1-4028-9F10-7C62520BD94F"

BASE_SITE = "roblox.com"
#BASE_SITE = "gametest5.robloxlabs.com"
API_KEY   = "76E5A40C-3AE1-4028-9F10-7C62520BD94F"

def main():
    sys.stderr.write("Fetching fast flags...\n")
    url = "http://clientsettings.api.%s/Setting/QuietGet/ClientAppSettings/?apiKey=%s" % (BASE_SITE, API_KEY)
    result = urllib.urlopen(url)
    if result.getcode() != 200:
        sys.exit(1)
    try:
        for line in result:
            print line,
    finally:
        result.close()

main()
