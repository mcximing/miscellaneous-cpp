#pragma once

#include <atlimage.h>
#include "opencv.hpp"
using namespace cv;

/************************************************************************/
// ȫ�ֺ�������
/************************************************************************/

CString GetFolderPath(HWND hParent);	// ѡ���ļ���
void MatToCImage( Mat &mat, CImage &cImage);	// ��Mat���͹���CImage��

void LoadDefaultParam();	// ������Ĭ�ϳ�ʼ��
void ResetParam();			// ���±궨��������������ͼ�������

int FindCorner(Mat &img);	// ���ͼ��ǵ�
void judgeCalibFiles(CString path, vector<bool>& ifGood, vector<string>& goodImg);	// ��ȡ�ļ��б궨ͼ���������о�
int doJudgeForCalib(CString path);	// ִ���ļ����,�б�궨ͼ�������Ƿ�������ڱ궨
void getObjectPoints();		// �������̽ǵ����������
int doSingleCalibLR();		// ͨ�����ļ�, ִ�����ҵ�Ŀ�궨
int doStereoCamCalib();		// ˫Ŀ�궨���ܺ���
int doStereoCamRectify();	// ˫ĿУ�����ܺ���
int remapImage(Mat img1, Mat img2);	// ��ͼ�����У��
void bmMatch();
void sgbmMatch();
void varMatch();


int loadConfiguration();	// �������ļ�����������
int loadStereoCalibData();	// ����˫Ŀ�궨�����ͽǵ������˫ĿУ��
int loadRectifyData();		// ����˫ĿУ��������У��ͼ�����ƥ��

void saveConfiguration();	// ��������������ļ�
void saveLeftCalibXml();	// ��������������궨����
void saveRightCalibXml();
void saveStereoCalibXml();	// ����˫Ŀ�궨����
void saveRectifyXml();		// ����˫ĿУ������
