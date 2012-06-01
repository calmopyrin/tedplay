// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include <string>
#include "Aboutdlg.h"
#include "WtlTedPlay.h"
#include "MainFrm.h"
#include "PlayList.h"
#include "CFileOpenDialog.h"
#include "PropertiesDlg.h"
#include "Audio.h"
#include "Psid.h"
#include "TedPlay.h"

#include "registry.h"

#define APPNAME _T("WinTedPlay")

//Get EXE directory.
void CMainFrame::MakePathName(LPTSTR lpFileName)
{
   int length;

   length = ::GetModuleFileName( NULL, lpFileName, MAX_PATH);
   while (length > 0) {
		if (lpFileName[length] == TEXT('\\'))
			break;

        lpFileName[length] = TEXT('\0');
        length--;
   }
}

void CMainFrame::getDefaultPlayListPath(_TCHAR *sFullPath)
{
	_TCHAR dir[MAX_PATH];

	MakePathName(dir);
	::PathCombine(sFullPath, dir, _T("default.pls"));
}

LRESULT CMainFrame::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    //Bind keys...
    m_haccelerator = AtlLoadAccelerators(IDR_ACCELERATORS);
	//
	SetWindowText(APPNAME);
	CenterWindow();
	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_APPICON), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_APPICON), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);
	HINSTANCE inst = _Module.GetResourceInstance();
	HMENU hmenu = ::LoadMenu(inst, MAKEINTRESOURCE(IDR_MENU));
	menu.Attach(hmenu);
	SetMenu(menu);

	// Edit controls
	stTitle.Attach(GetDlgItem(IDC_EDIT_MODULE));
	stAuthor.Attach(GetDlgItem(IDC_EDIT_AUTHOR3));
	stCopyright.Attach(GetDlgItem(IDC_EDIT_COPYRIGHT));
	stSubsong.Attach(GetDlgItem(IDC_EDIT_SUBSONG));
	// Buttons
	btnPrev.Attach(GetDlgItem(IDC_BUTTON_PREV));
	btnNext.Attach(GetDlgItem(IDC_BUTTON_NEXT));
	btnPlay.Attach(GetDlgItem(IDC_BUTTON_PLAY));
	btnPause.Attach(GetDlgItem(IDC_BUTTON_PAUSE));
	btnStop.Attach(GetDlgItem(IDC_BUTTON_STOP));
	// Trackbars/sliders
	trackBars[TB_VOLUME].Attach(GetDlgItem(IDC_SLIDER_VOLUME));
	trackBars[TB_VOLUME].SetRange(0, 10, TRUE);
	trackBars[TB_VOLUME].SetTic(8);
	//trackBars[TB_VOLUME].SetTicFreq(1);
	trackBars[TB_VOLUME].SetPos(8);
	trackBars[TB_SPEED].Attach(GetDlgItem(IDC_SLIDER_SPEED));
	trackBars[TB_SPEED].SetRange(1, 5, TRUE);
	trackBars[TB_SPEED].SetTic(1);
	trackBars[TB_SPEED].SetPos(3);
	DoDataExchange();
	// checkboxes
	cbChannels[0].Attach(GetDlgItem(IDC_CHECK1));
	cbChannels[1].Attach(GetDlgItem(IDC_CHECK2));
	cbChannels[0].SetCheck(1); // tedPlayIsChannelEnabled(0)
	cbChannels[1].SetCheck(1);

	// fire up the playlist
#if 1 // _DEBUG
	EnableMenuItem(GetMenu(), IDM_VIEW_PLAYLIST, MF_ENABLED);
#endif
	// put the playlist window right below the main one
	//RECT rc;
	//GetWindowRect(&rc);
	//rc.top += (rc.bottom - rc.top + 1);
	//rc.bottom = rc.top + 159;
	//rc.right = rc.left + 357;
	playListViewDialog.Create(m_hWnd); //, rc, 0);
	// I needed this because GetParent wouldn't work...
	playListViewDialog.setParent(m_hWnd);

	// get the playlist
	_TCHAR plPath[MAX_PATH];
	getDefaultPlayListPath(plPath);
	playListViewDialog.loadPlaylist(plPath);
	// the control buttons
	enableButtons(0);
	// set focus to the volume control so no caret is shown
	trackBars[TB_VOLUME].SetFocus();

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);
	//
		
	// Read settings
	unsigned int regVal = 0;
	if (getRegistryValue(_T("ShowPlayList"), regVal) && regVal) {
		playListViewDialog.ShowWindow(SW_NORMAL);
		::CheckMenuItem(GetMenu(), IDM_VIEW_PLAYLIST, MF_CHECKED);
	}

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// save the playlist
	_TCHAR plPath[MAX_PATH];
	getDefaultPlayListPath(plPath);
	playListViewDialog.savePlaylist(plPath);
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);
	// save settings
	LONG regVal = playListViewDialog.IsWindowVisible();
	setRegistryValue(_T("ShowPlayList"), regVal);
	//
	bHandled = FALSE;
	DestroyWindow();
	::PostQuitMessage(0);
	return 1;
}

LRESULT CMainFrame::OnMoving(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LPRECT newRect = (LPRECT) lParam;
	// Move along playlist window
	//if (playListViewDialog.IsWindowVisible()) 
	{
		RECT rc;
		GetWindowRect(&rc);
        // get how much we moved
		int  newXdelta   = newRect->left - rc.left;
        int  newYdelta   = newRect->top - rc.top;
		// now get the playlist window's metrics
		::GetWindowRect(playListViewDialog.m_hWnd, &rc);
		int  width  = rc.right - rc.left;
        int  height = rc.bottom - rc.top;
		// Move along with the main window
		::MoveWindow(playListViewDialog.m_hWnd, 
			rc.left + newXdelta, rc.top + newYdelta, width, height, TRUE);
	}
	return 0;
}

LRESULT CMainFrame::OnTrackBar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	int value;

	bHandled = FALSE;
	for(int i = 0; i < TB_COUNT; i++) {
		if ((HWND) lParam == trackBars[i].m_hWnd) {

			switch ((int)LOWORD(wParam)) {
				case SB_THUMBTRACK:
				case SB_THUMBPOSITION:
					value = (short)HIWORD(wParam);
					bHandled = TRUE;
					break;
				case SB_ENDSCROLL:
				case SB_LINELEFT:
				case SB_LINERIGHT:
				case SB_PAGELEFT:
				case SB_PAGERIGHT:
					value = trackBars[i].GetPos();
					bHandled = TRUE;
					break;
				default:
					break;
			} // end switch
			switch (i) {
				case 0:
					if (tedPlayGetState()) {
						tedplayPause();
						tedPlaySetVolume(value);
						tedplayPlay();
					} else {
						tedPlaySetVolume(value);
					}
					break;
				case 1:
					if (tedPlayGetState()) {
						tedplayPause();
						tedPlaySetSpeed(value);
						tedplayPlay();
					} else {
						tedPlaySetSpeed(value);
					}
					break;
				default: ;
			}
			break;
		}
	}
	if (bHandled) {
	} // end if
	return bHandled;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	DestroyWindow();
	::PostQuitMessage(0);
	return 0;
}

LRESULT CMainFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	_TCHAR szFilter[] = _T("TED tunes (*.c8m;*.ted;*.prg)\0"
						   "*.c8m;*.ted;*.prg\0"
						   "All Files (*.*)\0*.*\0\0");
	WTL::CFileDialog wndFileDialog ( TRUE, NULL, NULL, 
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER, 
		szFilter, m_hWnd );

	tedplayPause();
	::Sleep(10);
	if (IDOK == wndFileDialog.DoModal() ) {
		_TCHAR tmp[MAX_PATH];

		_tcscpy(tmp, wndFileDialog.m_szFileName);
		tedplayMain(tmp, NULL);
		UpdateSubsong();
	}
	tedplayPlay();

	//CFileOpenDialog dialog;
	//if (IDOK == dialog.DoModal()) {
	//	CString filePath;
	//	COM_VERIFY(dialog.GetFilePath(filePath));
	//	// Use file here...
	//}
	return 0;
}

LRESULT CMainFrame::OnFileProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CPropertiesDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnViewPlaylist(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (!playListViewDialog.IsWindowVisible()) {
		playListViewDialog.ShowWindow(SW_SHOWNOACTIVATE);
		::CheckMenuItem(GetMenu(), IDM_VIEW_PLAYLIST, MF_CHECKED);
	} else {
		playListViewDialog.ShowWindow(SW_HIDE);
		::CheckMenuItem(GetMenu(), IDM_VIEW_PLAYLIST, MF_UNCHECKED);
	}
	return 0;
}

LRESULT CMainFrame::OnDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	HDROP hDrop = (HDROP) wParam;
	_TCHAR namebuffer[MAX_PATH];

	::DragQueryFile(hDrop, 0, namebuffer, MAX_PATH);
	if (tedplayMain(namebuffer, NULL)) 
		return 1;
	UpdateSubsong();
	::DragFinish(hDrop);
	::SetForegroundWindow(m_hWnd);
	return 0;
}

LRESULT CMainFrame::OnUpdateSongFromChildWnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	UpdateSubsong();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

void CMainFrame::enableButtons(unsigned int mask)
{
	btnPrev.EnableWindow(mask & 1);
	btnNext.EnableWindow(mask & 2);
	btnPlay.EnableWindow(mask & 4);
	btnPause.EnableWindow(mask & 8);
	btnStop.EnableWindow(mask & 0x10);
}

unsigned int CMainFrame::getButtonStates()
{
	unsigned int state;
	state = btnPrev.IsWindowEnabled() & 1;
	state |= (btnNext.IsWindowEnabled() << 1) & 2;
	state |= (btnPlay.IsWindowEnabled() << 2) & 4;
	state |= (btnPause.IsWindowEnabled() << 3) & 8;
	state |= (btnStop.IsWindowEnabled() << 4) & 0x10;
	return state;
}

void CMainFrame::UpdateSubsong()
{
	unsigned int c, t;
	_TCHAR txt[512];

	PsidHeader psid = getPsidHeader();

	_tcscpy(txt, psid.fileName.c_str());
	// strip path from file name
	PathStripPath(txt);
	// prepare window title
	std::string title(APPNAME);
	title += " - ";
	title += txt;
	SetWindowText(title.c_str());

	stAuthor.SetWindowText(psid.author);
	stTitle.SetWindowText(psid.title);
	stCopyright.SetWindowText(psid.copyright);

	tedPlayGetSongs(c, t);
	if (t) {
		_stprintf(txt, _T("%u of %u"), c, t);
		stSubsong.SetWindowText(txt);
	} else {
		stSubsong.SetWindowText(_T(""));
	}
	if (getPsidHeader().tracks > 1)
		enableButtons(0x1f - 4);
	else
		enableButtons(0x1c - 4);
}

LRESULT CMainFrame::OnClickedPrev(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tedplayPause();
	psidChangeTrack(-1);
	UpdateSubsong();
	tedplayPlay();
	return 0L;
}

LRESULT CMainFrame::OnClickedNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tedplayPause();
	psidChangeTrack(+1);
	UpdateSubsong();
	tedplayPlay();
	return 0L;
}

LRESULT CMainFrame::OnClickedPlay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tedplayPlay();
	UpdateSubsong();
	unsigned int bmask = getButtonStates() & ~(4 + 8);
	enableButtons(bmask | 8);
	return 0L;
}

LRESULT CMainFrame::OnClickedPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tedplayPause();
	unsigned int bmask = getButtonStates() & ~(4 + 8);
	enableButtons(bmask | 4);
	return 0L;
}

LRESULT CMainFrame::OnClickedStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tedplayPause();
	unsigned int bmask = getButtonStates() & ~(4 + 8);
	enableButtons(bmask | 4);
	return 0L;
}

LRESULT CMainFrame::OnCheckBox1Clicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	unsigned int enabled = cbChannels[0].GetCheck();
	tedPlayChannelEnable(0, enabled);
	bHandled = TRUE;
	return 0L;
}

LRESULT CMainFrame::OnCheckBox2Clicked(WORD wNotifyCode, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled)
{
	unsigned int enabled = cbChannels[1].GetCheck();
	tedPlayChannelEnable(1, enabled);
	bHandled = TRUE;
	return 0L;
}

LRESULT CMainFrame::OnFileMemDump(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static unsigned int s = 0;
	char name[256];
	sprintf(name, "dump%04X.bin", s++);
	dumpMem(name);
	return 0L;
}

LRESULT CMainFrame::OnToolsResetplayer(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
	OnClickedStop(0, 0, 0, bHandled);
	//tedplayStop();
	machineReset();
	machineDoSomeFrames(2 * 800000);
	UpdateSubsong();
	//tedplayPlay();
	return 0;
}
