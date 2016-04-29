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
	int OpenVideo(CString FilePath,CDC* pDC,CRect rect);//��FilePath��Ƶ�ļ��������ú���ʾ����Ȳ���
	void Close();				                        //�رղ���
	void PlayVideo(int frame_pos);                      //���ŵ�frame_pos֡��Ƶ
	int  OpenCamera(int cam, CDC* pDC, CRect rect);     //������ͷ�������ú���ʾ����Ȳ���
	void PlayCamera();									//��������ͷ
	cv::Mat GrabFrame();								//��ȡһ֡
	void PlayFrame(cv::Mat frame);						//�����ض�֡�����㴦���Ĳ���
	void PlayFrame();									//���أ�ֱ�Ӳ��ŵ�ǰ֡
	void PauseVideo();                                  //��ͣ����
	void StopVideo();                                   //ֹͣ����
	void JumpFrame(bool direct, int step);				// ��Ƶ��֡����
	int GetPlaySpeed();                                 //�õ���Ƶ�ļ��Ĳ����ٶ�
	void DrawToRect(cv::Mat m_frame, CDC* pDC,CRect rcPlay);	//����������
	void MatToCImage(cv::Mat &mat, CImage &cImage);			//ת��ͼƬ�洢��ʽ����ʾ
	void SetCam(double height, double width, double fps);	//��������ͷ�������
	void StartRecord(char *fileName, double fps, cv::Size frmSize);
	void RecordVideo();
	void StopRecord();
    
	virtual ~ocvVideo();

public:
    CImage m_showimage;			//CImage����������ʾ֡���ؼ�
	cv::Mat m_frame;			//��ȡ֡
	cv::VideoCapture m_cap;		//��Ƶ����ȡ����
	cv::VideoWriter m_wtr;		//��Ƶ��д�����
	CString m_filepath;			//�ļ�·��
	CRect m_rect;				//��������                       
	CDC* m_pDC;					//������ʾ֡ͼ���DCָ��
	bool m_onVideo;			//�ж��Ƿ񲥷���
	bool m_onCam;			//�ж��Ƿ񲥷���
	bool m_record;
	int m_framepos;				//֡λ��
	int m_totalframes;			//֡����
	int m_fps;					//֡��
	int m_speed;				//�����ٶ�
};

