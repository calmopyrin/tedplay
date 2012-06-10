#include "stdafx.h"
#include "resource.h"

#include <string>
#include "PlayList.h"
#include "Psid.h"
#include "TedPlay.h"

#define STRING_EXCEPTION_EN _T("Exception occurred!")

bool CPlayList::EnumerateFolder(LPCTSTR lpcszFolder, LPTSTR ext, int nLevel)
{
	WIN32_FIND_DATA ffd;
	TCHAR szDirOnly[MAX_PATH];
	TCHAR szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;

	_tcscpy(szDirOnly, lpcszFolder);
	_tcscpy(szDir, lpcszFolder);
	_tcscat(szDir, ext);

	// Find the first file in the directory.
	hFind = FindFirstFile(szDir, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) {
		return false;
	} 

	// List all the files in the directory with some info about them.
	_TCHAR szOutLine[MAX_PATH] = {0};
	for (int ii = 0; ii < nLevel; ++ii)
		szOutLine[ii] = _T('\t');

	do {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if ( _T('.') != ffd.cFileName[0] ) {
				_tprintf(_T("%s%s   <DIR>\n"), szOutLine, ffd.cFileName);
				_tcscpy(szDir + _tcslen(lpcszFolder)+1, ffd.cFileName);
				EnumerateFolder(szDir, ext, nLevel + 1);    // Here's the recursive call!
			}
		} else {
			_TCHAR filePath[MAX_PATH];
			_tprintf(TEXT("%s%s\n"), szOutLine, ffd.cFileName);
			sprintf(filePath, _T("%s\\%s"), szDirOnly, ffd.cFileName);
			// add to playlist
			AddFileToPlaylist(filePath);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
	return true;
}

LRESULT CPlayList::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// Init the CDialogResize code
	DlgResize_Init();
    //Bind keys...
    m_haccelerator = AtlLoadAccelerators(IDR_ACCELERATORS_PLAYLIST);
	
	// bind buttons with objects
	btnAdd.Attach(GetDlgItem(IDC_BTN_ADD));
	btnRemove.Attach(GetDlgItem(IDC_BTN_REMOVE));
	btnAddFolder.Attach(GetDlgItem(IDC_BTN_ADDFOLDER));
	btnLoadPlayList.Attach(GetDlgItem(IDC_BTN_LOADPL));
	btnSavePlayList.Attach(GetDlgItem(IDC_BTN_SAVEPL));
	HINSTANCE inst = _Module.GetResourceInstance();
	// load & set button icons
	HICON icon = (HICON) ::LoadImage(inst, MAKEINTRESOURCE(IDI_ICON_OPEN),
		IMAGE_ICON, 16, 16, LR_SHARED);
	btnLoadPlayList.SetIcon(icon);
	icon = (HICON) ::LoadImage(inst, MAKEINTRESOURCE(IDI_ICON_SAVE),
		IMAGE_ICON, 16, 16, LR_SHARED);
	btnSavePlayList.SetIcon(icon);
	//
	UINT uCToolTipCtrlStyle = TTS_NOPREFIX |TTS_NOFADE | TTS_NOANIMATE; // | TTS_BALLOON
	UINT uToolFlags = TTF_IDISHWND | TTF_SUBCLASS;

	ATLVERIFY(NULL != m_wndToolTip[0].Create(m_hWnd, 0, NULL, uCToolTipCtrlStyle)); // WS_POPUP|TTS_NOPREFIX|TTS_NOFADE|TTS_NOANIMATE
	CToolInfo toolInfo(uToolFlags, btnAdd.m_hWnd, 0, 0, _T("Add file(s)"));
	m_wndToolTip[0].AddTool(&toolInfo);
	m_wndToolTip[0].SetDelayTime(TTDT_INITIAL,0);
	m_wndToolTip[0].SetDelayTime(TTDT_RESHOW,0);
	m_wndToolTip[0].Activate(TRUE);
	
	ATLVERIFY(NULL != m_wndToolTip[1].Create(m_hWnd, 0, NULL, uCToolTipCtrlStyle));
	toolInfo.Init(uToolFlags, btnRemove.m_hWnd, 0, 0, _T("Remove File(s)"));
	m_wndToolTip[1].AddTool(&toolInfo);
	m_wndToolTip[1].Activate(TRUE);

	ATLVERIFY(NULL != m_wndToolTip[2].Create(m_hWnd, 0, NULL, uCToolTipCtrlStyle));
	toolInfo.Init(uToolFlags, btnAddFolder.m_hWnd, 0, 0, _T("Add Folder"));
	m_wndToolTip[2].AddTool(&toolInfo);
	m_wndToolTip[2].Activate(TRUE);
	
	// Create the listview columns
	playListView.Attach(GetDlgItem(IDC_LSV1));
	playListView.SetView(LV_VIEW_DETAILS);
	// FIXME doesnt work
	//playListView.SetExtendedListViewStyle(LVS_SHOWSELALWAYS);
	playListView.AddColumn(_T("Filename"), LV_FIELD_FILENAME);
	playListView.SetColumnWidth(LV_FIELD_FILENAME, 150);
	//
	playListView.AddColumn(_T("Path"), LV_FIELD_PATH);
	playListView.SetColumnWidth(LV_FIELD_PATH, 300);
	//
	playListView.AddColumn(_T("Status"), LV_FIELD_STATUS);
	playListView.SetColumnWidth(LV_FIELD_STATUS, 50);
	//
	playListView.AddColumn(_T("#"), LV_FIELD_INDEX);
	playListView.SetColumnWidth(LV_FIELD_INDEX, 32);
	
	//
	// register object for message filtering and idle updates    
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);
	return TRUE;
}

LRESULT CPlayList::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	HWND parent = GetParent();
	HMENU hmenu = ::GetMenu(m_hwndParent);
	ShowWindow(SW_HIDE);
	CheckMenuItem(hmenu, IDM_VIEW_PLAYLIST, MF_UNCHECKED);
	return 0;
}

BOOL CPlayList::AddFileToPlaylist(_TCHAR *fullPath)
{
	_TCHAR tmp[MAX_PATH];

	_tcscpy(tmp, fullPath);
	PathStripPath(tmp);
	PathRemoveExtension(tmp);
	UINT index = playListView.GetItemCount();
	//
	_TCHAR indexText[16];
	_stprintf(indexText, _T("%i"), index + 1);
	playListView.AddItem(index, LV_FIELD_FILENAME, tmp);
	playListView.AddItem(index, LV_FIELD_PATH, fullPath);
	playListView.AddItem(index, LV_FIELD_STATUS, _T("OK"));
	playListView.AddItem(index, LV_FIELD_INDEX, indexText);
	return TRUE;
}

LRESULT CPlayList::OnDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	HDROP hDrop = (HDROP) wParam;
	_TCHAR namebuffer[MAX_PATH];

	UINT count = ::DragQueryFile(hDrop, -1, namebuffer, MAX_PATH);
	while (count--) {
		DragQueryFile(hDrop, count, namebuffer, MAX_PATH);
		AddFileToPlaylist(namebuffer);
	} 
	::DragFinish(hDrop);
	::SetForegroundWindow(m_hWnd);
	return 0;
}

LRESULT CPlayList::OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0L;
}

LRESULT CPlayList::OnResizing(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0L;
}

LRESULT CPlayList::OnLvReturn(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& bHandled)
{
	OnNMDblclkLsv(0, 0, bHandled);
	return 0L;
}

int CPlayList::savePlaylist(_TCHAR * plName)
{
	FILE *fp = _tfopen(plName, _T("wt"));
	if (!fp)
		return 1;
	try {
		for(int i = 0; i < playListView.GetItemCount(); i++) {
			_TCHAR name[MAX_PATH];
			if (playListView.GetItemText(i, LV_FIELD_PATH, name, MAX_PATH)) {
				_fputts(name, fp);
				_ftprintf(fp, _T("\n"));
			}
		}
	} catch(_TCHAR *str) {
		MessageBox(str, STRING_EXCEPTION_EN, MB_ICONERROR);
	}
	fclose(fp);
	return 0;
}

int CPlayList::loadPlaylist(_TCHAR * plName)
{
	FILE *fp = _tfopen(plName, _T("rt"));
	if (!fp)
		return 1;
	int index = 0;
	try {
		while (!feof(fp)) {
			_TCHAR name[MAX_PATH] = { 0 };
			if (_fgetts(name, MAX_PATH, fp) && _tcslen(name)) {
				AddFileToPlaylist(name);
				playListView.AddItem(index, LV_FIELD_STATUS, ::PathFileExists(name) ? _T("OK") : _T("Error!"));
			}
			index++;
		}
	} catch(_TCHAR *str) {
		MessageBox(str, STRING_EXCEPTION_EN, MB_ICONERROR);
	}
	fclose(fp);
	return 0;
}

static int CALLBACK StrCompareFunc(LPARAM lParam1, LPARAM lParam2,
								   LPARAM lParamSort)
{
	// First two parameters are item data to be compared.
	// Third parameter is bSortDescending
	CString* pStr1 = (CString*)lParam1;
	CString* pStr2 = (CString*)lParam2;

	if (*pStr1 < *pStr2) return lParamSort ? 1 : -1;
	if (*pStr1 > *pStr2) return lParamSort ? -1 : 1;
	return 0;
}

BOOL CPlayList::SortList(int iColumn, bool bDescending)
{
	ATLASSERT((iColumn >= 0) && (iColumn < GetColumnCount()));
//	CWaitCursor waitcursor;
	int iItem;
	const int nLen = MAX_PATH;
	LVITEM lviGet;
	memset(&lviGet, 0, sizeof(LVITEM));
	lviGet.cchTextMax = nLen;
	lviGet.iSubItem = iColumn;
	LVITEM lviSet = {LVIF_PARAM};

	// ... case Sort as text
	for (iItem = 0; iItem < playListView.GetItemCount() ; iItem++) {
		CString* pStr = new CString;
		lviGet.pszText = pStr->GetBufferSetLength(nLen);
		playListView.SendMessage(LVM_GETITEMTEXT, (WPARAM)iItem, (LPARAM)&lviGet);
		pStr->ReleaseBuffer();
		lviSet.iItem = iItem;
		lviSet.lParam = (LPARAM)pStr;
		playListView.SetItem( &lviSet );
	}
	// here is the call to LVM_SORTITEMS
	playListView.SortItems((PFNLVCOMPARE) &StrCompareFunc, (LPARAM)bDescending);

	// cleanup
	for (iItem = 0; iItem < playListView.GetItemCount() ; iItem++) {
		lviSet.iItem = iItem;
		playListView.GetItem(&lviSet);
		if (lviSet.lParam)
			delete (CString*)(lviSet.lParam);
	}
	// ... end Sort as text
	// Sort as integer, double, or other type
	reIndex(0, 0);

//	UpdateSortIcons(iColumn, bDescending);
	return TRUE;
}

void CPlayList::reIndex(int fromCol, int toCol)
{
	int items = playListView.GetItemCount();

	for(int i = 0; i < playListView.GetItemCount(); i++) {
		_TCHAR indexText[16];
		_stprintf(indexText, _T("%i"), i + 1);
		playListView.SetItem(i, LV_FIELD_INDEX, LVIF_TEXT, indexText, 0, 0, 0, 0);
	}
}

LRESULT CPlayList::OnLvnColumnclickLsv(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	// no sorting for index column
	if (pNMLV->iSubItem == LV_FIELD_INDEX)
		return 0;
	// if the same columns is clicked, sort descending
	m_bSortDescending = (m_iSortColumn == pNMLV->iSubItem) ? !m_bSortDescending : false;
	SortList(pNMLV->iSubItem, m_bSortDescending);
	// remember which column was sorted last
	m_iSortColumn = pNMLV->iSubItem;
	return 0;
}

LRESULT CPlayList::OnNMDblclkLsv(int idCtrl, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	UINT index = playListView.GetSelectionMark();
	_TCHAR namebuffer[MAX_PATH];

	if (playListView.GetItemText(index, LV_FIELD_PATH, namebuffer, MAX_PATH)) {
		if (::PathFileExists(namebuffer) && !tedplayMain(namebuffer, NULL)) {
			::PostMessage(m_hwndParent, WM_USER + 1, 0, 0);
			playListView.AddItem(index, LV_FIELD_STATUS, _T("OK"));
		} else {
			playListView.AddItem(index, LV_FIELD_STATUS, _T("Error!"));
		}
	}
	return 0;
}

LRESULT CPlayList::OnPlayTune(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
	OnNMDblclkLsv(0, 0, bHandled);
	return 0;
}

LRESULT CPlayList::OnBnClickedBtnAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	const int FILENAMES_BUFFER_SIZE = 32048;
	_TCHAR szFileNamesBuffer[FILENAMES_BUFFER_SIZE] = {0};
	_TCHAR szFilter[] = _T("TED tunes (*.c8m;*.ted;*.prg)\0"
											"*.c8m;*.ted;*.prg\0"
								"All Files (*.*)\0*.*\0\0");
	WTL::CFileDialog wndFileDialog ( TRUE, NULL, NULL, 
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ALLOWMULTISELECT, 
		szFilter, m_hWnd );
	wndFileDialog.m_ofn.lpstrFile = szFileNamesBuffer;
	wndFileDialog.m_ofn.nMaxFile = FILENAMES_BUFFER_SIZE;

	if (IDOK == wndFileDialog.DoModal() ) {
		_TCHAR sDirectory[MAX_PATH];
		_TCHAR sFullPath[MAX_PATH];

		::GetCurrentDirectory(MAX_PATH, sDirectory);//wndFileDialog.m_ofn.lpstrFile);
		//PathStripToRoot(sDirectory);
		LPTSTR sFileName = wndFileDialog.m_ofn.lpstrFile + wndFileDialog.m_ofn.nFileOffset;

		while(sFileName != NULL && *sFileName != _T('\0')) {
			sFullPath[0] = _T('\0');
			::PathCombine(sFullPath, sDirectory, sFileName);
			AddFileToPlaylist(sFullPath);
			// path is the file
			sFileName += _tcslen(sFileName) + 1;
		}
	}
	return 0;
}

LRESULT CPlayList::OnBnClickedBtnRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	 int nSelRows = playListView.GetSelectedCount();
	 if (!nSelRows)
		 return 1;
	 int *pnArrayOfSelRows = new int[nSelRows];
	 if (!pnArrayOfSelRows)
		 return 2;
	 int nIndex = -1;
	 // Loop through all selected items
	 for (int i = 0; i < nSelRows; i++) {
		 // Search for the next selected list control item and get its index
		 nIndex = playListView.GetNextItem(nIndex, LVNI_SELECTED);
		 if (nIndex != LB_ERR)
			pnArrayOfSelRows[i] = nIndex;
	 }
	 for(int j = nSelRows - 1; j >= 0; j--) {
		 playListView.DeleteItem(pnArrayOfSelRows[j]);
	 }
	 // select the proper item
	 int index = playListView.GetItemCount();
	 if (index) {
		 int newsel;
		 if (index != pnArrayOfSelRows[0])
			 newsel = pnArrayOfSelRows[0];
		 else
			 newsel = index - 1;
		 playListView.SetFocus();
		 playListView.SetItemState(newsel, LVIS_SELECTED, LVIS_SELECTED);
	 }
	 delete(pnArrayOfSelRows);
	 pnArrayOfSelRows = NULL;

	 reIndex(0, 0);

	 return 0;
}

LRESULT CPlayList::OnLvnKeydownLsv1(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& bHandled)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	if (pLVKeyDow->wVKey == VK_DELETE) {
		OnBnClickedBtnRemove(0, 0, m_hWnd, bHandled);
	} else if (pLVKeyDow->wVKey == VK_ADD) {
		//if (::GetAsyncKeyState(VK_CONTROL) & 0x8000) 
		{
			OnSelectAll(0, 0, 0, bHandled);
		}
	} else if (pLVKeyDow->wVKey == VK_SPACE) {
		stepPlayList(+1);
	}
	return 0;
}

int CPlayList::stepPlayList(int direction)
{
	int items = playListView.GetItemCount();
	if (!items)
		return 1;
	int sel = playListView.GetSelectionMark();
	playListView.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
	if (sel + 1 < items) {
		playListView.SetItemState(sel + 1, LVIS_SELECTED, LVIS_SELECTED);
	} else {
		playListView.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
	}
	return 0;
}

LRESULT CPlayList::OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	MSG msg = {m_hWnd, uMsg, wParam, lParam};

	for(unsigned int i = 0; i < 3; i++) {
		if (m_wndToolTip[i].IsWindow()) {
			m_wndToolTip[0].RelayEvent(&msg);
			//m_wndToolTip[i].RelayEvent((LPMSG) GetCurrentMessage());
		}
	}
	bHandled = FALSE;
	return 1;
}

LRESULT CPlayList::OnBnClickedBtnAddfolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	_TCHAR strFolderPath[MAX_PATH];
	// TODO: Add your control notification handler code here
	CFolderDialog dlgFolder(m_hWnd, _T("Add files recoursively from folder..."),
		BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS);

	::GetCurrentDirectory(MAX_PATH, strFolderPath);
	dlgFolder.SetInitialFolder(strFolderPath);
	if ( IDOK == dlgFolder.DoModal()) {
		_tcscpy(strFolderPath, dlgFolder.GetFolderPath());
		//EnumerateFolder(strFolderPath, _T("\\*.prg"));
		//EnumerateFolder(strFolderPath, _T("\\*.c8m"));
		EnumerateFolder(strFolderPath, _T("\\*"));
	}
	return 0;
}

LRESULT CPlayList::OnSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	playListView.SetFocus();
	int index = playListView.GetItemCount();
	while (index) {
		index--;
		playListView.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
	}
	return 0;
}

LRESULT CPlayList::OnNMRclickLsv1(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	OnIdle(); //force UI Updating
	POINT pt;
	GetCursorPos(&pt);

	CMenu menu;
	menu.LoadMenu(IDR_POPUPMENU_PLIST);
	CMenuHandle menuPop = menu.GetSubMenu(0);
	
    // Display the shortcut menu. Track the right mouse button. 
	::TrackPopupMenuEx(menuPop, 
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, 
            pt.x, pt.y, m_hWnd, NULL); 

	return 0;
}

LRESULT CPlayList::OnRootOpenfilelocation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	std::string path;
	UINT index = playListView.GetSelectionMark();
	_TCHAR namebuffer[MAX_PATH] = _T("");

	if (playListView.GetItemText(index, 1, namebuffer, MAX_PATH)) {
		_TCHAR *end = _tcsrchr(namebuffer, _T('\\'));
		if (end) {
			*end = _T('\0');
		} else {
			MessageBox(_T("Could not open file location."), _T("Error!"), MB_ICONERROR);
			return 1;
		}
	}
	// TODO: Add your command handler code here
	HINSTANCE hinst = ::ShellExecute(m_hWnd, _T("open"), _T("explorer.exe"), namebuffer, NULL, SW_SHOWNORMAL);
	return 0;
}

LRESULT CPlayList::OnBnClickedBtnLoadpl(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	_TCHAR szFilter[] = _T("Tedplay playlists (*.pls)\0"
											"*.pls\0"
								"All Files (*.*)\0*.*\0\0");
	WTL::CFileDialog wndFileDialog ( TRUE, NULL, NULL, 
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER, 
		szFilter, m_hWnd );

	if (IDOK == wndFileDialog.DoModal() ) {
		LPTSTR sFileName = wndFileDialog.m_ofn.lpstrFile;
		playListView.DeleteAllItems();
		loadPlaylist(sFileName);
	}
	return 0;
}

LRESULT CPlayList::OnBnClickedBtnSavepl(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	_TCHAR szFilter[] = _T("Tedplay playlists (*.pls)\0"
											"*.pls\0"
								"All Files (*.*)\0*.*\0\0");
	WTL::CFileDialog wndFileDialog(FALSE, NULL, NULL, 
		OFN_PATHMUSTEXIST | OFN_EXPLORER, szFilter, m_hWnd);

	if (IDOK == wndFileDialog.DoModal() ) {
		LPTSTR sFileName = wndFileDialog.m_ofn.lpstrFile;
		savePlaylist(sFileName);
	}
	return 0;
}
