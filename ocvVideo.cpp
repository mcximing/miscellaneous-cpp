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

	m_filepath = _T("");         //�ļ�·��
	m_onVideo = FALSE;			 //�ж��Ƿ񲥷���
	m_onCam = FALSE;			 //
	m_record = FALSE;
	m_framepos = 0;              //֡λ��
	m_totalframes = 0;           //֡����
	m_fps = 0;                   //֡��
	m_speed = 30;                //�����ٶ�,��ʼΪ30

}



// ��Mat���͹���CImage��
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



int ocvVideo::OpenVideo(CString FilePath, CDC* pDC, CRect rect)  //��FilePath��Ƶ�ļ��������ú���ʾ����Ȳ���
{
	if (m_cap.isOpened())
		m_cap.release();
	USES_CONVERSION;
	char * pFileName = T2A(FilePath);
	m_cap.open(pFileName);      //������Ƶ
	if(!m_cap.isOpened())
	{
		AfxMessageBox(_T("���ļ�ʧ�ܣ�"),MB_OK,0);
		return 0;
	}
	m_rect = rect;                       //���picture�ؼ���RECT    
	m_pDC = pDC;                         //���picture�ؼ��Ļ�ͼ���
	m_filepath = FilePath;               //�����Ƶ�ļ�·��
	m_totalframes = (int)m_cap.get(CV_CAP_PROP_FRAME_COUNT);  //��ȡ��Ƶ֡����
    if(m_totalframes==0)
	{
		AfxMessageBox(_T("���ܴ򿪸ø�ʽ�ļ���"),MB_OK,0);
		return 0;
	}
	m_fps=(int)m_cap.get(CV_CAP_PROP_FPS);   // ֡��
	m_onVideo = true;
	return 1;
}


void ocvVideo::PlayVideo(int frame_pos)		// ������Ƶ������ʾ��ǰ֡��ͼ�񵽿ؼ�
{
	if((frame_pos>=m_totalframes) || (!m_cap.isOpened()))
		return;

	if (frame_pos>-1)	// ����Ϊ����ʱֱ�Ӽ�������һ״̬���ţ���������λ�á�
	{
		m_framepos = frame_pos;		// ����֡��ʼλ��
		m_cap.set(CV_CAP_PROP_POS_FRAMES, (m_framepos%m_totalframes)); // ���ò���֡λ��
	}
    
	m_cap >> m_frame;	// ��ȡһ֡
	m_framepos ++;

	if(m_frame.data!=NULL)
	    DrawToRect(m_frame, m_pDC,m_rect);	//��ʾ���ؼ�
}


int ocvVideo::OpenCamera(int cam, CDC* pDC, CRect rect)  //������ͷ�������ú���ʾ����Ȳ���
{
	if (m_cap.isOpened())
		m_cap.release();
	m_cap.open(cam);			//��������ͷ

	if(!m_cap.isOpened())
	{
		AfxMessageBox(_T("������ͷʧ��!"),MB_OK,0);
		return 0;
	}

	m_rect = rect;                       //���picture�ؼ���RECT
	m_pDC = pDC;                         //���picture�ؼ��Ļ�ͼ���
	m_onCam = true;
	return 1;
}


void ocvVideo::PlayCamera()
{
	if(!m_cap.isOpened())
	{
		return;
	}
	m_cap >> m_frame;				//��ȡһ֡
	if(m_frame.data!=NULL)
	{
		DrawToRect(m_frame, m_pDC,m_rect);   //��ʾ���ؼ�
	}
}


Mat ocvVideo::GrabFrame()
{
	if(!m_cap.isOpened())
	{
		return Mat(m_frame.rows, m_frame.cols, m_frame.depth(), Scalar(255,255,255));
	}
	m_cap >> m_frame;				//��ȡһ֡
	m_framepos ++;
	return m_frame;
}


void ocvVideo::PlayFrame(Mat frame)
{
	if(frame.data!=NULL)
	{
		DrawToRect(frame, m_pDC,m_rect);   //��ʾ���ؼ�
	}
}

void ocvVideo::PlayFrame()
{
	if(m_frame.data!=NULL)
	{
		DrawToRect(m_frame, m_pDC,m_rect);   //��ʾ���ؼ�
	}
}



// ��Ƶ��֡����
void ocvVideo::JumpFrame(bool direct, int step)
{
	if (!m_onVideo)
		return;
	if (!m_cap.isOpened())
		return;

	if (direct)	// �����>>
		m_framepos += step;
	else		// ��ǰ��<<
		m_framepos -= step;

	if (m_framepos<0)
		m_framepos = 0;

	if (m_framepos>=m_totalframes)
		m_framepos = m_totalframes-1;

	m_cap.set(CV_CAP_PROP_POS_FRAMES, m_framepos);

	m_cap >> m_frame;
	DrawToRect(m_frame, m_pDC, m_rect);
}


void ocvVideo::PauseVideo()                  //��ͣ����
{

}
void ocvVideo::StopVideo()                   //ֹͣ����
{

}

int ocvVideo::GetPlaySpeed()                 //�õ���Ƶ�ļ��Ĳ����ٶ�
{
	if(m_fps>0 && m_fps<1000)
	{
		m_speed = (int)(1000/m_fps);        //ÿһ֡���ٺ���
	    return m_speed;
	}
   return 30;
}


// ������Ƶ����
void ocvVideo::SetCam(double height, double width, double fps)
{
	m_cap.set(CV_CAP_PROP_FRAME_WIDTH, width);
	m_cap.set(CV_CAP_PROP_FRAME_HEIGHT, height);
	m_cap.set(CV_CAP_PROP_FPS, fps);
}


// ��ʼ¼��
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



void ocvVideo::Close()            //�رղ����ļ�,�ͷŲ���
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
