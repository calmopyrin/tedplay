// WtlTedPlay.cpp : main source file for WtlTedPlay.exe
//

#include "stdafx.h"
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>

#include "resource.h"
#include "MainFrm.h"
#include "PlayList.h"
#include "registry.h"

#include "tedmem.h"
#include "tedplay.h"

#define HAVE_SDL
#ifdef HAVE_SDL
#include <SDL.h>
#include "AudioSDL.h"
#else
#include "AudioDirectSound.h"
#endif

CAppModule _Module;

// make sure the message loop is destroyed before CoUninitialize gets called
static int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame dlgMain;
	dlgMain.Create(NULL);

	unsigned int defaultSampleRate = 0;
	getRegistryValue(_T("SampleRate"), defaultSampleRate);
	if (!defaultSampleRate || defaultSampleRate > 110860) defaultSampleRate = 110860;

	try {
#ifdef HAVE_SDL
		int retval = tedplayMain( __argv[1], new AudioSDL(machineInit(defaultSampleRate), defaultSampleRate));
#else
		int retval = tedplayMain( __argv[1], new AudioDirectSound(machineInit(), 110860));
#endif
	} catch (_TCHAR *str) {
		MessageBox(NULL, str, _T("Exception occured."), MB_OK | MB_ICONERROR);
	}
	int nRet = dlgMain.ShowWindow(SW_NORMAL);
	nRet = theLoop.Run();
	tedplayClose();
	setRegistryValue(_T("SampleRate"), defaultSampleRate);

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
#if _DEBUG // start memory leak checker
	/*_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF |
		  _CRTDBG_LEAK_CHECK_DF );*/
#endif
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
	//HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES
		| ICC_WIN95_CLASSES
		);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
