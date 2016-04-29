#pragma once
#include "afxcmn.h"


struct threadInfo
{
	UINT nMilliSecond;
	HWND hwnd;
	CProgressCtrl* pCtrlProgress;
};

UINT ThreadFunc1(LPVOID lpParam);



// CProgressDlg �Ի���

class CProgressDlg : public CDialog
{
	DECLARE_DYNAMIC(CProgressDlg)

public:
	CProgressDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CProgressDlg();

// �Ի�������
	enum { IDD = IDD_PROGRESS_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()

public:
	CProgressCtrl m_mainProgress;
	CWinThread* pThread1;
	int progress1, progress2, progress3;
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
