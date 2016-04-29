// ProgressDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
//#include "BinoDetect.h"
#include "ProgressDlg.h"
#include "StereoFunc.h"

threadInfo Info;

// CProgressDlg �Ի���

IMPLEMENT_DYNAMIC(CProgressDlg, CDialog)

CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProgressDlg::IDD, pParent)
{
	progress1 = 0;
	progress2 = 0;
	progress3 = 0;
}

CProgressDlg::~CProgressDlg()
{
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MAIN_PROGRESS, m_mainProgress);
}


BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CProgressDlg ��Ϣ�������


BOOL CProgressDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// �궨���ȶԻ���ĳ�ʼ��
	loadConfiguration();

	Info.nMilliSecond = 40;
	Info.hwnd = this->m_hWnd;
	Info.pCtrlProgress = &m_mainProgress;
	pThread1 = AfxBeginThread(ThreadFunc1,&Info);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}



void CProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case 100:
		m_mainProgress.SetPos(++progress1);
		if (progress1==99)
			progress1 = 1;
		break;

	case 101:
		m_mainProgress.SetPos(++progress2);
		if (progress2==99)
			progress2 = 1;
		break;

	case 102:
		m_mainProgress.SetPos(++progress3);
		if (progress3==99)
			progress3 = 1;
		break;
	default :
		break;
	}

	CDialog::OnTimer(nIDEvent);
}




// �������߳�1���궨
UINT ThreadFunc1(LPVOID lpParam)
{
	Sleep(300);
	threadInfo* pInfo=(threadInfo*)lpParam;
	int ret = 1;

	//	AfxMessageBox(_T("The Begin."), MB_OK, 0);
	CString path = GetFolderPath(pInfo->hwnd);

	// doJudgeForCalib  �ж��궨����
	SetDlgItemText(pInfo->hwnd, IDC_STATIC_TEXT, _T("���ڼ��궨������Ч��..."));
	SetTimer(pInfo->hwnd, 100, 50, NULL);
	ret = doJudgeForCalib(path);
	if (ret==-1){
		AfxMessageBox(_T("��������������С��һ��"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else if (ret==-2){
		AfxMessageBox(_T("����û�п��õ�ͼƬ"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else{
		CString msg;
		msg.Format(_T("�궨�����б��������ЧͼƬ%d��"), ret);
		AfxMessageBox(msg,MB_OK,0);
	}
	KillTimer(pInfo->hwnd, 100);
	pInfo->pCtrlProgress->SetPos(0);

	// doSingleCalibLR  ִ�е�Ŀ�궨
	SetDlgItemText(pInfo->hwnd, IDC_STATIC_TEXT, _T("����ִ�����ҵ�Ŀ�궨..."));
	SetTimer(pInfo->hwnd, 101, 100, NULL);
	ret = doSingleCalibLR();
	if (ret==-1){
		AfxMessageBox(_T("���󣺲鲻���궨�ļ��б�"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else if (ret==-2){
		AfxMessageBox(_T("���󣺶�ȡ�궨�ļ�����"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else{
		CString msg;
		msg.Format(_T("���ҵ�Ŀ�궨��������ʱ��%dms"), ret);
		AfxMessageBox(msg,MB_OK,0);
	}
	KillTimer(pInfo->hwnd, 101);
	pInfo->pCtrlProgress->SetPos(0);

	// doStereoCamCalib ִ��˫Ŀ�궨
	SetDlgItemText(pInfo->hwnd, IDC_STATIC_TEXT, _T("����ִ��˫Ŀ������궨..."));
	SetTimer(pInfo->hwnd, 102, 600, NULL);
	ret = doStereoCamCalib();
	if (ret==-1){
		AfxMessageBox(_T("�������ҵ�Ŀ�궨δ���"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else{
		CString msg;
		msg.Format(_T("˫Ŀ�궨��������ʱ��%dms"), ret);
		AfxMessageBox(msg,MB_OK,0);
	}
	KillTimer(pInfo->hwnd, 102);
	pInfo->pCtrlProgress->SetPos(0);

	// doStereoCamRectify ִ��ͼ��У��
	ret = doStereoCamRectify();
	if (ret==0){
		AfxMessageBox(_T("����δ��⵽˫Ŀ�궨����"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else
		AfxMessageBox(_T("У������������"),MB_OK,0);

	ResetParam();

	//	AfxMessageBox(_T("The End."), MB_OK, 0);
	//	EndDialog(pInfo->hwnd, IDCANCEL);	// �˷������ã��ᵼ���������޷������˳���ԭ����
	::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);

	return 0;
}

