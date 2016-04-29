// ProgressDlg.cpp : 实现文件
//

#include "stdafx.h"
//#include "BinoDetect.h"
#include "ProgressDlg.h"
#include "StereoFunc.h"

threadInfo Info;

// CProgressDlg 对话框

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


// CProgressDlg 消息处理程序


BOOL CProgressDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 标定进度对话框的初始化
	loadConfiguration();

	Info.nMilliSecond = 40;
	Info.hwnd = this->m_hWnd;
	Info.pCtrlProgress = &m_mainProgress;
	pThread1 = AfxBeginThread(ThreadFunc1,&Info);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
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




// 工作者线程1，标定
UINT ThreadFunc1(LPVOID lpParam)
{
	Sleep(300);
	threadInfo* pInfo=(threadInfo*)lpParam;
	int ret = 1;

	//	AfxMessageBox(_T("The Begin."), MB_OK, 0);
	CString path = GetFolderPath(pInfo->hwnd);

	// doJudgeForCalib  判定标定样本
	SetDlgItemText(pInfo->hwnd, IDC_STATIC_TEXT, _T("正在检查标定样本有效性..."));
	SetTimer(pInfo->hwnd, 100, 50, NULL);
	ret = doJudgeForCalib(path);
	if (ret==-1){
		AfxMessageBox(_T("错误：左右向量大小不一致"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else if (ret==-2){
		AfxMessageBox(_T("错误：没有可用的图片"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else{
		CString msg;
		msg.Format(_T("标定样本判别结束，有效图片%d对"), ret);
		AfxMessageBox(msg,MB_OK,0);
	}
	KillTimer(pInfo->hwnd, 100);
	pInfo->pCtrlProgress->SetPos(0);

	// doSingleCalibLR  执行单目标定
	SetDlgItemText(pInfo->hwnd, IDC_STATIC_TEXT, _T("正在执行左右单目标定..."));
	SetTimer(pInfo->hwnd, 101, 100, NULL);
	ret = doSingleCalibLR();
	if (ret==-1){
		AfxMessageBox(_T("错误：查不到标定文件列表"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else if (ret==-2){
		AfxMessageBox(_T("错误：读取标定文件出错"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else{
		CString msg;
		msg.Format(_T("左右单目标定结束，耗时：%dms"), ret);
		AfxMessageBox(msg,MB_OK,0);
	}
	KillTimer(pInfo->hwnd, 101);
	pInfo->pCtrlProgress->SetPos(0);

	// doStereoCamCalib 执行双目标定
	SetDlgItemText(pInfo->hwnd, IDC_STATIC_TEXT, _T("正在执行双目摄像机标定..."));
	SetTimer(pInfo->hwnd, 102, 600, NULL);
	ret = doStereoCamCalib();
	if (ret==-1){
		AfxMessageBox(_T("错误：左右单目标定未完成"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else{
		CString msg;
		msg.Format(_T("双目标定结束，耗时：%dms"), ret);
		AfxMessageBox(msg,MB_OK,0);
	}
	KillTimer(pInfo->hwnd, 102);
	pInfo->pCtrlProgress->SetPos(0);

	// doStereoCamRectify 执行图像校正
	ret = doStereoCamRectify();
	if (ret==0){
		AfxMessageBox(_T("错误：未检测到双目标定数据"),MB_OK,0);
		::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	else
		AfxMessageBox(_T("校正数据已生成"),MB_OK,0);

	ResetParam();

	//	AfxMessageBox(_T("The End."), MB_OK, 0);
	//	EndDialog(pInfo->hwnd, IDCANCEL);	// 此法不可用，会导致主窗口无法正常退出，原因不明
	::SendMessage(pInfo->hwnd, WM_CLOSE, 0, 0);

	return 0;
}

