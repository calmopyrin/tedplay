#include "resource.h"
#include "stdafx.h"
#include <prsht.h>

#include "PropPageAudio.h"

CPropPageAudio::CPropPageAudio(void)
{
	m_psp.dwFlags |= PSP_USEICONID;
	m_psp.pszIcon = MAKEINTRESOURCE(IDI_PROPPAGEAUDIO);
	m_psp.hInstance = _Module.GetResourceInstance();
}

LPARAM CPropPageAudio::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	int r;

	if (!ebLatency.m_hWnd) {
		ebLatency.Attach(GetDlgItem(IDC_EDIT_BUFLEN));
	}
	if (!sbLatency.m_hWnd)
		sbLatency.Attach(GetDlgItem(IDC_SPIN_BUFLEN));
	sbLatency.SetRange(40, 500);

	// only when DDX is out for trackbars
	sbLatency.SetPos(vLatency);

	DoDataExchange();

	return 0;
}

LRESULT CPropPageAudio::OnDefaultClick(WORD wNotifyCode, WORD wID, HWND hwndCtl, BOOL& bHandled)
{
	sbLatency.SetPos(vLatency = 100);
	return 0;
}

int CPropPageAudio::OnApply()
{
	BOOL retval = DoDataExchange(true);
	return retval;// ? PSNRET_INVALID : PSNRET_NOERROR;
}

LRESULT CPropPageAudio::OnTrackBar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	int value;

	bHandled = FALSE;
	return bHandled;
}
