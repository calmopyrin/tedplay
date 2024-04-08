#pragma once

#include "tedplay.h"

class CScreenDlg : public CDialogImpl<CScreenDlg>
{
public:
	enum { IDD = IDD_DIALOG_PROPERTIES };

	BEGIN_MSG_MAP(CScreenDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CLOSE, OnWmClose)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		properties.Attach(GetDlgItem(IDC_EDIT_PROPERTIES));
		properties.SetFont((HFONT)GetStockObject(SYSTEM_FIXED_FONT), FALSE);
		CenterWindow(GetParent());
		SetWindowText("Video matrix");
		updateContent();
		return TRUE;
	}
	LRESULT OnWmClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		EndDialog(IDD_DIALOG_SCREEN);
		return 0;
	}
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
	void updateContent() {
		char screenBuf[42 * 25 + 1];
		tedShowScreenData(screenBuf);
		properties.AppendText(screenBuf, TRUE);
	}
protected:
	CEdit properties;
};