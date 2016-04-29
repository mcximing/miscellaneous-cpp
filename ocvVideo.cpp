// ocvVideo.cpp: implementation of the ocvVideo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ocvVideo.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

using namespace cv;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ocvVideo::ocvVideo()
{

	m_filepath = _T("");         //文件路径
	m_onVideo = FALSE;			 //判断是否播放中
	m_onCam = FALSE;			 //
	m_record = FALSE;
	m_framepos = 0;              //帧位置
	m_totalframes = 0;           //帧总数
	m_fps = 0;                   //帧率
	m_speed = 30;                //播放速度,初始为30

}



// 由Mat类型构建CImage类
void ocvVideo::MatToCImage( Mat &mat, CImage &cImage)
{
	//create new CImage
	int width    = mat.cols;
	int height   = mat.rows;
	int channels = mat.channels();

	cImage.Destroy(); //clear
	cImage.Create(width, 
		height, //positive: left-bottom-up   or negative: left-top-down
		8*channels ); //numbers of bits per pixel

	//copy values
	uchar* ps;
	uchar* pimg = (uchar*)cImage.GetBits(); //A pointer to the bitmap buffer

	//The pitch is the distance, in bytes. represent the beginning of 
	// one bitmap line and the beginning of the next bitmap line
	int step = cImage.GetPitch();

	for (int i = 0; i < height; ++i)
	{
		ps = (mat.ptr<uchar>(i));
		for ( int j = 0; j < width; ++j )
		{
			if ( channels == 1 ) //gray
			{
				*(pimg + i*step + j) = ps[j];
			}
			else if ( channels == 3 ) //color
			{
				for (int k = 0 ; k < 3; ++k )
				{
					*(pimg + i*step + j*3 + k ) = ps[j*3 + k];
				}            
			}
		}    
	}
}


void ocvVideo::DrawToRect(Mat frame, CDC* pDC, CRect rcPlay)
{
	if (frame.data==NULL)
		return;
	Size dsize = Size(rcPlay.Width(),rcPlay.Height());
	Mat showImage = Mat(dsize,CV_8UC3);
	resize(frame,showImage,dsize,0,0,INTER_NEAREST);
	MatToCImage(showImage,m_showimage);
	m_showimage.Draw(pDC->m_hDC,rcPlay);
}



int ocvVideo::OpenVideo(CString FilePath, CDC* pDC, CRect rect)  //打开FilePath视频文件，并设置好显示区域等参数
{
	if (m_cap.isOpened())
		m_cap.release();
	USES_CONVERSION;
	char * pFileName = T2A(FilePath);
	m_cap.open(pFileName);      //捕获视频
	if(!m_cap.isOpened())
	{
		AfxMessageBox(_T("打开文件失败！"),MB_OK,0);
		return 0;
	}
	m_rect = rect;                       //获得picture控件的RECT    
	m_pDC = pDC;                         //获得picture控件的画图句柄
	m_filepath = FilePath;               //获得视频文件路径
	m_totalframes = (int)m_cap.get(CV_CAP_PROP_FRAME_COUNT);  //获取视频帧总数
    if(m_totalframes==0)
	{
		AfxMessageBox(_T("不能打开该格式文件！"),MB_OK,0);
		return 0;
	}
	m_fps=(int)m_cap.get(CV_CAP_PROP_FPS);   // 帧率
	m_onVideo = true;
	return 1;
}


void ocvVideo::PlayVideo(int frame_pos)		// 播放视频，即显示当前帧的图像到控件
{
	if((frame_pos>=m_totalframes) || (!m_cap.isOpened()))
		return;

	if (frame_pos>-1)	// 参数为负数时直接继续从上一状态播放，否则设置位置。
	{
		m_framepos = frame_pos;		// 播放帧开始位置
		m_cap.set(CV_CAP_PROP_POS_FRAMES, (m_framepos%m_totalframes)); // 设置播放帧位置
	}
    
	m_cap >> m_frame;	// 获取一帧
	m_framepos ++;

	if(m_frame.data!=NULL)
	    DrawToRect(m_frame, m_pDC,m_rect);	//显示到控件
}


int ocvVideo::OpenCamera(int cam, CDC* pDC, CRect rect)  //打开摄像头，并设置好显示区域等参数
{
	if (m_cap.isOpened())
		m_cap.release();
	m_cap.open(cam);			//捕获摄像头

	if(!m_cap.isOpened())
	{
		AfxMessageBox(_T("打开摄像头失败!"),MB_OK,0);
		return 0;
	}

	m_rect = rect;                       //获得picture控件的RECT
	m_pDC = pDC;                         //获得picture控件的画图句柄
	m_onCam = true;
	return 1;
}


void ocvVideo::PlayCamera()
{
	if(!m_cap.isOpened())
	{
		return;
	}
	m_cap >> m_frame;				//获取一帧
	if(m_frame.data!=NULL)
	{
		DrawToRect(m_frame, m_pDC,m_rect);   //显示到控件
	}
}


Mat ocvVideo::GrabFrame()
{
	if(!m_cap.isOpened())
	{
		return Mat(m_frame.rows, m_frame.cols, m_frame.depth(), Scalar(255,255,255));
	}
	m_cap >> m_frame;				//获取一帧
	m_framepos ++;
	return m_frame;
}


void ocvVideo::PlayFrame(Mat frame)
{
	if(frame.data!=NULL)
	{
		DrawToRect(frame, m_pDC,m_rect);   //显示到控件
	}
}

void ocvVideo::PlayFrame()
{
	if(m_frame.data!=NULL)
	{
		DrawToRect(m_frame, m_pDC,m_rect);   //显示到控件
	}
}



// 视频跳帧函数
void ocvVideo::JumpFrame(bool direct, int step)
{
	if (!m_onVideo)
		return;
	if (!m_cap.isOpened())
		return;

	if (direct)	// 向后跳>>
		m_framepos += step;
	else		// 向前跳<<
		m_framepos -= step;

	if (m_framepos<0)
		m_framepos = 0;

	if (m_framepos>=m_totalframes)
		m_framepos = m_totalframes-1;

	m_cap.set(CV_CAP_PROP_POS_FRAMES, m_framepos);

	m_cap >> m_frame;
	DrawToRect(m_frame, m_pDC, m_rect);
}


void ocvVideo::PauseVideo()                  //暂停播放
{

}
void ocvVideo::StopVideo()                   //停止播放
{

}

int ocvVideo::GetPlaySpeed()                 //得到视频文件的播放速度
{
	if(m_fps>0 && m_fps<1000)
	{
		m_speed = (int)(1000/m_fps);        //每一帧多少毫秒
	    return m_speed;
	}
   return 30;
}


// 设置视频参数
void ocvVideo::SetCam(double height, double width, double fps)
{
	m_cap.set(CV_CAP_PROP_FRAME_WIDTH, width);
	m_cap.set(CV_CAP_PROP_FRAME_HEIGHT, height);
	m_cap.set(CV_CAP_PROP_FPS, fps);
}


// 开始录像
void ocvVideo::StartRecord(char *fileName, double fps, Size frmSize)
{
	m_wtr.open(fileName, CV_FOURCC('D','I','V','3'), fps, frmSize, true);
	m_record = true;
}

void ocvVideo::RecordVideo()
{
	if (m_record && m_onCam)
		m_wtr << m_frame;
}

void ocvVideo::StopRecord()
{
	m_record = false;
	if (m_wtr.isOpened())
		m_wtr.release();
}



void ocvVideo::Close()            //关闭播放文件,释放捕获
{
	if(m_frame.data)
		m_frame.release();
	m_pDC->FillSolidRect(m_rect,RGB(0,0,0));
	if (m_cap.isOpened())
		m_cap.release();
	m_onVideo = false;
	m_onCam = false;
}



ocvVideo::~ocvVideo()
{
 	if(m_cap.isOpened())
 		Close();
}
