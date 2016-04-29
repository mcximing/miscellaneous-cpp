#pragma once

#include "atlimage.h"
#include "opencv.hpp"

// ���������������
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
	void init(cv::Mat input, bool copy);	// ��ʼ��ͼ��copyָʾ�Ƿ�������
	void release();	// �ͷ���Դ
	void reset();	// ͼ������Ϊ�գ�������Ч

	cv::Mat getHist(bool img);		// ���ֱ��ͼ��img��0��Դͼ��1��Ŀ��ͼ��
	cv::Mat getRGBHist(bool img);	// ��ɫRGBֱ��ͼ
	void doHistEqualization();	// ֱ��ͼ���⻯
	void do16GrayToRGB();		// 16�׻Ҷ�תα��ɫ
	void doPseudoColor(int mode);	// �Ҷ�-��ɫ�任

	void doMeanFiltering(int i);	// ��ֵ�˲�
	void doMedianFiltering(int i);	// ��ֵ�˲�
	void doGaussianFiltering(int i);// ��˹�˲�
	void doBilateralFiltering(int i);	// ˫���˲�
	void doCanny(int lowTh, int ratio);	// Canny��Ե���
	void doSobel();		// Sobel�������ӱ�Ե���
	void doLaplacian();	// Laplacian���ӱ�Ե���

	void doErosion(int ktype, int ksize);	// ��ʴ����
	void doDilation(int ktype, int ksize);	// ��������
	void doOpening(int ktype, int ksize);	// ������
	void doClosing(int ktype, int ksize);	// ������
	void doMorphGra(int ktype, int ksize);	// ��̬�ݶ�
	void doTopHat(int ktype, int ksize);	// ��ñ����
	void doBlackHat(int ktype, int ksize);	// ��ñ����
	void doThreshBin(int th);		// ��ֵ��ֵ��
	void doThreshBinInv(int th);	// ����ֵ��ֵ��
	void doThreshTrunc(int th);		// �ض���ֵ��
	void doThreshZero(int th);		// ��ֵ��Ϊ0
	void doThreshZeroInv(int th);	// ����ֵ��Ϊ0

	void doNegative();	// ��ƬЧ��
	void doVFlip();		// ��ֱ��ת
	void doHFlip();		// ˮƽ��ת
	void doTranspose();	// ת�ã�90��������
	void doVHFlip();	// ��ֱ��ˮƽ��180�ȷ�ת
	void doDFT();		// ��ɢ����Ҷ�任
	void doConvert(double a, double b);	// ���ȶԱȶȴ�������ֵ��Χ������
	
public:
	bool inited;	// ָʾͼ���Ƿ���׼����
	cv::Mat src;	// Դͼ��
	cv::Mat dst;	// Ŀ��ͼ��
	cv::Mat gray;	// Դͼ��ĻҶ�ͼ��
	cv::Mat hist;	// ֱ��ͼ�洢����
	CImage m_showimage;	// ������ʾ��CImage��
};


