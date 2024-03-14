#include <windows.h>
#include <tchar.h>
#include <cstdio>

#define REGKEYPATH _T("Software\\Gaia\\WinTedPlay")
#define KEY_STANDARD_ACCESS (KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY)
static _TCHAR regKeyPath[MAX_PATH];

bool getRegistryValue(_TCHAR *keyName, unsigned int &value)
{
	// Read settings
	HKEY appKey;
	DWORD keyLength = 4;
	LONG regVal = -1;

	_stprintf(regKeyPath, "%s\\%s", REGKEYPATH, keyName);
	LONG s = ::RegOpenKeyEx(HKEY_CURRENT_USER, REGKEYPATH, 0,
		KEY_ALL_ACCESS, &appKey);
	if (s != ERROR_SUCCESS) {
		if (s == ERROR_FILE_NOT_FOUND) {
			s = ::RegCreateKeyEx(HKEY_CURRENT_USER, REGKEYPATH, 0, 0,
				REG_OPTION_NON_VOLATILE, KEY_STANDARD_ACCESS, 0, &appKey, 0);
			::RegCloseKey(appKey);
		}
	}
	else {
		bool retval;
		s = ::RegQueryValueEx(appKey, keyName, 0,
			NULL, (LPBYTE)&regVal, (LPDWORD)&keyLength);
		retval = (s == ERROR_SUCCESS);
		if (retval && regVal != -1)
			value = regVal;
		::RegCloseKey(appKey);
		return retval;
	}
	return false;
}

bool setRegistryValue(_TCHAR *keyName, unsigned int value)
{
	HKEY appKey;
	LONG s = ::RegCreateKeyEx(HKEY_CURRENT_USER, REGKEYPATH, 0, 0, 
		REG_OPTION_NON_VOLATILE, KEY_STANDARD_ACCESS, 0, &appKey, 0);
	if (s == ERROR_SUCCESS) {
		s = ::RegSetValueEx(appKey, keyName, 0, 
			REG_DWORD, (CONST BYTE *) &value, sizeof(value));
		::RegCloseKey(appKey);
		return true;
	}
	return false;
}
