#pragma once

#include <atlimage.h>
#include "opencv.hpp"
using namespace cv;

/************************************************************************/
// 全局函数声明
/************************************************************************/

CString GetFolderPath(HWND hParent);	// 选择文件夹
void MatToCImage( Mat &mat, CImage &cImage);	// 由Mat类型构建CImage类

void LoadDefaultParam();	// 参数的默认初始化
void ResetParam();			// 更新标定输入参数，会清空图像点数据

int FindCorner(Mat &img);	// 检测图像角点
void judgeCalibFiles(CString path, vector<bool>& ifGood, vector<string>& goodImg);	// 读取文件夹标定图像样本并判决
int doJudgeForCalib(CString path);	// 执行文件检测,判别标定图像样本是否可以用于标定
void getObjectPoints();		// 计算棋盘角点的世界坐标
int doSingleCalibLR();		// 通过读文件, 执行左右单目标定
int doStereoCamCalib();		// 双目标定功能函数
int doStereoCamRectify();	// 双目校正功能函数
int remapImage(Mat img1, Mat img2);	// 对图像进行校正
void bmMatch();
void sgbmMatch();
void varMatch();


int loadConfiguration();	// 从配置文件载入各项参数
int loadStereoCalibData();	// 载入双目标定参数和角点参数供双目校正
int loadRectifyData();		// 载入双目校正参数供校正图像进行匹配

void saveConfiguration();	// 保存参数到配置文件
void saveLeftCalibXml();	// 保存左右摄像机标定参数
void saveRightCalibXml();
void saveStereoCalibXml();	// 保存双目标定参数
void saveRectifyXml();		// 保存双目校正参数
