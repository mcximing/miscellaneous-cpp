// ocvVideo.h: interface for the ocvVideo class.
//
//////////////////////////////////////////////////////////////////////



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlimage.h>
#include <opencv.hpp>

class ocvVideo 
{
public:
	ocvVideo();
	int OpenVideo(CString FilePath,CDC* pDC,CRect rect);//打开FilePath视频文件，并设置好显示区域等参数
	void Close();				                        //关闭播放
	void PlayVideo(int frame_pos);                      //播放第frame_pos帧视频
	int  OpenCamera(int cam, CDC* pDC, CRect rect);     //打开摄像头，并设置好显示区域等参数
	void PlayCamera();									//播放摄像头
	cv::Mat GrabFrame();								//获取一帧
	void PlayFrame(cv::Mat frame);						//播放特定帧，方便处理后的播放
	void PlayFrame();									//重载，直接播放当前帧
	void PauseVideo();                                  //暂停播放
	void StopVideo();                                   //停止播放
	void JumpFrame(bool direct, int step);				// 视频跳帧函数
	int GetPlaySpeed();                                 //得到视频文件的播放速度
	void DrawToRect(cv::Mat m_frame, CDC* pDC,CRect rcPlay);	//画到矩形中
	void MatToCImage(cv::Mat &mat, CImage &cImage);			//转换图片存储格式供显示
	void SetCam(double height, double width, double fps);	//设置摄像头读入参数
	void StartRecord(char *fileName, double fps, cv::Size frmSize);
	void RecordVideo();
	void StopRecord();
    
	virtual ~ocvVideo();

public:
    CImage m_showimage;			//CImage对象，用于显示帧到控件
	cv::Mat m_frame;			//获取帧
	cv::VideoCapture m_cap;		//视频流读取对象
	cv::VideoWriter m_wtr;		//视频流写入对象
	CString m_filepath;			//文件路径
	CRect m_rect;				//播放区域                       
	CDC* m_pDC;					//用于显示帧图像的DC指针
	bool m_onVideo;			//判断是否播放中
	bool m_onCam;			//判断是否播放中
	bool m_record;
	int m_framepos;				//帧位置
	int m_totalframes;			//帧总数
	int m_fps;					//帧率
	int m_speed;				//播放速度
};

