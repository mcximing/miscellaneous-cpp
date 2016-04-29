#pragma once

#include "opencv.hpp"

using namespace cv;

extern CString calibFilePath;	// �궨ͼ���ļ��б�·��

extern Mat currImg1;	// ��ͼ1��ǰͼ��
extern Mat currImg2;	// ��ͼ2��ǰͼ��
extern Mat currImg3;	// ��ͼ3��ǰͼ��

extern int m_nx;				// �����нǵ��� w
extern int m_ny;				// �����нǵ��� h
extern int boardCornerNum;		// ���̽ǵ�����= w*h
extern cv::Size patternSize;	// �������нǵ�=(w,h)
extern cv::Size imageSize;		// ͼ��ֱ���
extern float squareWidth;		// ���̸��ӱ߳�
extern int imgNum;				// ���ڱ궨��ͼ��������������
extern int useNum;				// ��ЧͼƬ��Ŀ
extern vector<Point2f> corners;	// ��Žǵ����꣬ע��vectorһ��Ҫ��ʼ��
extern vector<vector<Point3f>>	objectPoints;	// ���̽ǵ�������������
extern vector<vector<Point2f>>	imagePoints1;	// ����ͼ�����̽ǵ�������������
extern vector<vector<Point2f>>	imagePoints2;	// ����ͼ�����̽ǵ�������������


extern Mat img1r;		// У���������ͼ��
extern Mat img2r;

extern int MatchAlgorithm;