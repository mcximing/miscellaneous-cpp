#pragma once

#include "atlimage.h"
#include "opencv.hpp"

// 类外独立函数声明
void drawHist(cv::Mat src, cv::Mat &histImage);
void drawGrayHist(cv::Mat src, cv::Mat &histImage);
void MatToCImage( cv::Mat &mat, CImage &cImage);
void GrayToRGB(cv::Mat &gray, cv::Mat &color, bool isColor);
void applyGrayMap(cv::Mat src, cv::Mat &dst, int gmax, int gmin, int nslice);

class ocvImgProc
{
public:
	ocvImgProc(void);
	~ocvImgProc(void);
	void init(cv::Mat input, bool copy);	// 初始化图像，copy指示是否进行深复制
	void release();	// 释放资源
	void reset();	// 图像重置为空，依旧有效

	cv::Mat getHist(bool img);		// 获得直方图，img：0是源图像，1是目标图像
	cv::Mat getRGBHist(bool img);	// 彩色RGB直方图
	void doHistEqualization();	// 直方图均衡化
	void do16GrayToRGB();		// 16阶灰度转伪彩色
	void doPseudoColor(int mode);	// 灰度-彩色变换

	void doMeanFiltering(int i);	// 均值滤波
	void doMedianFiltering(int i);	// 中值滤波
	void doGaussianFiltering(int i);// 高斯滤波
	void doBilateralFiltering(int i);	// 双边滤波
	void doCanny(int lowTh, int ratio);	// Canny边缘检测
	void doSobel();		// Sobel导数算子边缘检测
	void doLaplacian();	// Laplacian算子边缘检测

	void doErosion(int ktype, int ksize);	// 腐蚀运算
	void doDilation(int ktype, int ksize);	// 膨胀运算
	void doOpening(int ktype, int ksize);	// 开运算
	void doClosing(int ktype, int ksize);	// 闭运算
	void doMorphGra(int ktype, int ksize);	// 形态梯度
	void doTopHat(int ktype, int ksize);	// 顶帽运算
	void doBlackHat(int ktype, int ksize);	// 黑帽运算
	void doThreshBin(int th);		// 二值阈值化
	void doThreshBinInv(int th);	// 反二值阈值化
	void doThreshTrunc(int th);		// 截断阈值化
	void doThreshZero(int th);		// 阈值化为0
	void doThreshZeroInv(int th);	// 反阈值化为0

	void doNegative();	// 负片效果
	void doVFlip();		// 垂直翻转
	void doHFlip();		// 水平翻转
	void doTranspose();	// 转置（90度左旋）
	void doVHFlip();	// 垂直加水平，180度翻转
	void doDFT();		// 离散傅里叶变换
	void doConvert(double a, double b);	// 亮度对比度处理（矩阵值范围操作）
	
public:
	bool inited;	// 指示图像是否已准备好
	cv::Mat src;	// 源图像
	cv::Mat dst;	// 目标图像
	cv::Mat gray;	// 源图像的灰度图像
	cv::Mat hist;	// 直方图存储矩阵
	CImage m_showimage;	// 用于显示的CImage类
};


