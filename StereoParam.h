#pragma once

#include "opencv.hpp"

using namespace cv;

extern CString calibFilePath;	// 标定图像文件列表路径

extern Mat currImg1;	// 视图1当前图像
extern Mat currImg2;	// 视图2当前图像
extern Mat currImg3;	// 视图3当前图像

extern int m_nx;				// 棋盘行角点数 w
extern int m_ny;				// 棋盘列角点数 h
extern int boardCornerNum;		// 棋盘角点总数= w*h
extern cv::Size patternSize;	// 棋盘行列角点=(w,h)
extern cv::Size imageSize;		// 图像分辨率
extern float squareWidth;		// 棋盘格子边长
extern int imgNum;				// 用于标定的图像对数，绑定输入框
extern int useNum;				// 有效图片数目
extern vector<Point2f> corners;	// 存放角点坐标，注意vector一定要初始化
extern vector<vector<Point3f>>	objectPoints;	// 棋盘角点世界坐标序列
extern vector<vector<Point2f>>	imagePoints1;	// 左视图的棋盘角点像素坐标序列
extern vector<vector<Point2f>>	imagePoints2;	// 右视图的棋盘角点像素坐标序列


extern Mat img1r;		// 校正后的左右图像
extern Mat img2r;

extern int MatchAlgorithm;