#include "stdafx.h"
#include "ocvImgProc.h"

using namespace cv;


//////////////////////////////////////////////////////////////////////////
// 独立图像处理函数

// 绘制灰度直方图，代码参照OpenCV教程修改，要求输入单通道图像。
void drawGrayHist(Mat src, Mat &histImage)
{
	if (!src.data)
		return;
	if (src.channels()!=1)
		return;

	Mat hist;	// 存储计算出来的直方图数据
	int histSize = 256;
	float range[] = { 0, 256 } ;
	const float* histRange = { range };		// 为函数调用准备适当数据
	calcHist( &src, 1, 0, Mat(), hist, 1, &histSize, &histRange, 1, 0 );

	int hist_w = 512; int hist_h = 240;		// 画直方图的长和宽
	int bin_w = cvRound( (double) hist_w/histSize );	// 小矩形的宽

	if (!histImage.empty())
		histImage.release();
	histImage = Mat( hist_h, hist_w, CV_8UC3, Scalar(255,255,255) );// 背景为白色

	normalize(hist, hist, 0, hist_h, NORM_MINMAX, -1, Mat() );	// 归一化至0-255

	for( int i = 1; i < histSize; i++ )	// 画一个个小矩形
	{
		rectangle(histImage, Point( bin_w*(i-1), hist_h ) ,
			Point( bin_w*(i), hist_h - cvRound(hist.at<float>(i)) ),Scalar(0,0,0),CV_FILLED,8,0);
	}
}


// 绘制RGB直方图，代码来自OpenCV教程，有少许修改
void drawHist(Mat src, Mat &histImage)
{
	if( !src.data )
		return;

	// Separate the image in 3 places ( B, G and R )
	Mat bgr_planes[3];
	split( src, bgr_planes );

	// Establish the number of bins
	int histSize = 256;

	// Set the ranges ( for B,G,R) )
	float range[] = { 0, 256 } ;
	const float* histRange = { range };

	bool uniform = true; bool accumulate = false;

	Mat b_hist, g_hist, r_hist;

	// Compute the histograms:
	calcHist( &bgr_planes[0], 1, 0, Mat(), b_hist, 1, &histSize, &histRange, uniform, accumulate );
	calcHist( &bgr_planes[1], 1, 0, Mat(), g_hist, 1, &histSize, &histRange, uniform, accumulate );
	calcHist( &bgr_planes[2], 1, 0, Mat(), r_hist, 1, &histSize, &histRange, uniform, accumulate );

	// Draw the histograms for B, G and R
	int hist_w = 512; int hist_h = 240;
	int bin_w = cvRound( (double) hist_w/histSize );

	if (!histImage.empty())
		histImage.release();
	histImage = Mat( hist_h, hist_w, CV_8UC3, Scalar(255,255,255) );

	// Normalize the result to [ 0, histImage.rows ]
	normalize(b_hist, b_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
	normalize(g_hist, g_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
	normalize(r_hist, r_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );

	// Draw for each channel
	for( int i = 1; i < histSize; i++ )
	{
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(b_hist.at<float>(i-1)) ) ,
			Point( bin_w*(i), hist_h - cvRound(b_hist.at<float>(i)) ),
			Scalar( 255, 0, 0), 2, 8, 0  );
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(g_hist.at<float>(i-1)) ) ,
			Point( bin_w*(i), hist_h - cvRound(g_hist.at<float>(i)) ),
			Scalar( 0, 255, 0), 2, 8, 0  );
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(r_hist.at<float>(i-1)) ) ,
			Point( bin_w*(i), hist_h - cvRound(r_hist.at<float>(i)) ),
			Scalar( 0, 0, 255), 2, 8, 0  );
	}
}



// 由Mat类型构建CImage类
void MatToCImage(Mat &mat, CImage &cImage)
{
	//create new CImage
	int width    = mat.cols;
	int height   = mat.rows;
	int channels = mat.channels();

	cImage.Destroy(); //clear
	cImage.Create(width, 
		height, //positive: left-bottom-up   or negative: left-top-down
		8*channels ); //numbers of bits per pixel

	// 这一段if加上后才能正常显示灰度单通道图像。 
	if (channels==1)
	{
		//如果是1个通道的图像(灰度图像),CImage中内置了调色板，我们要对他进行赋值：
		RGBQUAD* ColorTable;
		int MaxColors=256;
		//这里可以通过cImage.GetMaxColorTableEntries()得到大小(如果你是cImage.Load读入图像的话)
		ColorTable = new RGBQUAD[MaxColors];
		cImage.GetColorTable(0,MaxColors,ColorTable);//这里是取得指针
		for (int i=0; i<MaxColors; i++)
		{
			ColorTable[i].rgbBlue = (BYTE)i;
			//BYTE和uchar一回事，但MFC中都用它
			ColorTable[i].rgbGreen = (BYTE)i;
			ColorTable[i].rgbRed = (BYTE)i;
		}
		cImage.SetColorTable(0,MaxColors,ColorTable);
		delete []ColorTable;
	}

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


// 灰度转三通道函数，一般用不到。最后一个参数控制是否添加伪彩色
void GrayToRGB(Mat &gray, Mat &color, bool isColor)
{
	if (gray.depth()!=CV_8U)
		return;
	if (gray.channels()!=1)
		return;
	if (!isColor)
		cvtColor(gray,color,CV_GRAY2BGR); // 直接变成三通道灰色图像
	else
	{
		color = Mat::zeros(gray.rows, gray.cols, CV_8UC3);
		// 赋予伪彩色
		for (int y=0;y<gray.rows;y++)
		{
			for (int x=0;x<gray.cols;x++)
			{
				uchar val = gray.at<uchar>(y,x);
				uchar r,g,b;

				if (val==0)		// 用常见的那个变换函数
					r = g = b = 0;
				else
				{
					r = 255-val;
					g = val < 128 ? val*2 : (uchar)((255 - val)*2);
					b = val;
				}
				color.at<cv::Vec3b>(y,x) = cv::Vec3b(r,g,b);
			}
		}
	}
}

// 灰度分割变换方法添加伪彩色-默认nslice为16，因只内置了16色的BGR表
void applyGrayMap(Mat src, Mat &dst, int gmax, int gmin, int nslice)
{
	int gap = gmax - gmin;	// 区间灰度差值
	if ((gap<nslice)||nslice<=0)	// nslice-分层数
		return;

	int n = 0;	// 颜色索引
	int step = gap/nslice;	// 每层区间跨度
	int width = src.cols;
	int height = src.rows;

	uchar *ps, *pd;
	int map[3][16] = {	// 16色BGR颜色表
		0,128,255,0,128,0,255,0,128,0,128,192,0,255,0,255,
		0,0,0,128,128,255,255,0,0,128,128,192,0,0,255,255,
		0,0,0,0,0,0,0,128,128,128,128,192,255,255,255,255
	};

	if (src.channels()==3)
		cvtColor(src,src,CV_BGR2GRAY);

	if (!dst.empty())
		dst.release();

	dst = Mat(height, width, CV_8UC3, Scalar(0,0,0));

	for (int i = 0; i < height; ++i)
	{
		ps = (src.ptr<uchar>(i));
		pd = (dst.ptr<uchar>(i));
		for ( int j = 0; j < width; ++j )
		{
			n = (ps[j]-gmin)/step;	// 区间相对值除以跨度
			if (n>=nslice)
				n = nslice-1;		// 区间顶归一化
			if (n<=0)
				n=0;				// 区间底归一为0
			pd[j*3] = map[0][n];
			pd[j*3+1] = map[1][n];
			pd[j*3+2] = map[2][n];
		}    
	}
}


//////////////////////////////////////////////////////////////////////////
// 类实现函数

// 默认构造和析构
ocvImgProc::ocvImgProc(void)
{
	inited = false;
}

ocvImgProc::~ocvImgProc(void)
{
	release();
}

// 初始化图像矩阵
void ocvImgProc::init(Mat input, bool copy)
{
	if (copy)
		input.copyTo(src);	// 深复制，连数据拷贝
	else
		src = input;	// 浅复制，仅复制文件头，快

	gray = Mat(src.rows,src.cols,CV_8UC1);	// 准备灰度化的图

	if (src.channels()==3)
		cvtColor(src,gray,CV_BGR2GRAY);
	else if (src.channels()==1)
		gray = src;

	inited = true;
}

// 释放资源
void ocvImgProc::release()
{
	src.release();
	dst.release();
	gray.release();
	inited = false;
}

// 重置灰度矩阵
void ocvImgProc::reset()
{
	if (!src.empty())
	{
		if (src.channels()==3)	// 改变过gray后最好重置
			cvtColor(src,gray,CV_BGR2GRAY);
		else if (src.channels()==1)
			gray = src;
		inited = true;
	}
	else
		gray.release();
//	if (!dst.empty())
//		dst.release();
}

//////////////////////////////////////////////////////////////////////////
// 类系列处理功能函数

/************************直方图相关***************************/

// 绘出直方图
Mat ocvImgProc::getHist(bool img)
{
	Mat image;	// 判断是对原图像还是目标图像绘直方图
	if(!img)image = src;
	else	image = dst;

	if (image.empty())
		return Mat(240,512,CV_8UC1,Scalar(255,255,255));

	// 彩色先变换为灰色，因为可能对目标图像操作故不用gray
	if (image.channels()==3)
	{
		Mat temp = Mat(image.rows,image.cols,CV_8UC1);
		cvtColor(image,temp,CV_BGR2GRAY);
		drawGrayHist(temp,hist);
		temp.release();
	}
	else if (image.channels()==1)
		drawGrayHist(image,hist);

	return hist;
}

//绘制RGB直方图
Mat ocvImgProc::getRGBHist(bool img)
{
	Mat image;	// 判断是对原图像还是目标图像绘直方图
	if(!img)image = src;
	else	image = dst;

	if (image.empty())
		return Mat(240,512,CV_8UC1,Scalar(255,255,255));

	if (image.channels()==3)
		drawHist(image,hist);
	else if (image.channels()==1)
		drawGrayHist(image,hist);

	return hist;
}

// 直方图均衡化
void ocvImgProc::doHistEqualization()
{
	Mat temp;
	if (!gray.empty())
		equalizeHist(gray,temp);	// 直接调用
	temp.copyTo(dst);
	temp.release();
	reset();
}

// 16阶灰度转伪彩色
void ocvImgProc::do16GrayToRGB()
{
	if (!gray.empty())
		applyGrayMap(gray, dst, 255, 0, 16);
}

// 灰度-彩色变换函数
void ocvImgProc::doPseudoColor(int mode)
{
	if (src.empty())
		return;
	if (mode>11 || mode<0)	// 此处用contrib库里的函数，有12种变换映射
		mode = 2;
	applyColorMap(src, dst, mode);
//	GrayToRGB(gray,dst,1);
}


/************************图像滤波***************************/

// 均值滤波
void ocvImgProc::doMeanFiltering(int i)
{
	if (src.empty())
		return;
	if (i%2==0)
		i=i+1;
	blur(src,dst,Size(i,i),Point(-1,-1));
}

// 中值滤波
void ocvImgProc::doMedianFiltering(int i)
{
	if (src.empty())
		return;
	if (i%2==0)
		i=i+1;
	medianBlur(src,dst,i);
}

// 高斯滤波
void ocvImgProc::doGaussianFiltering(int i)
{
	if (src.empty())
		return;
	if (i%2==0)
		i=i+1;
	GaussianBlur(src, dst, Size(i,i), 0, 0 );
}

// 双边滤波
void ocvImgProc::doBilateralFiltering(int i)
{
	if (src.empty())
		return;
	if (i%2==0)
		i=i+1;
	bilateralFilter(src, dst, i, i*2, i/2 );	//能够保留锐度
}

/************************边缘检测***************************/

// Canny算子边缘检测，ratio是高阈值对低阈值的比率
void ocvImgProc::doCanny(int lowTh, int ratio)
{
	if (gray.empty())
		return;
	blur(gray,gray,Size(3,3),Point(-1,-1));	// 先平滑处理
	Canny(gray, dst, lowTh, lowTh*ratio, 3);
	reset();
} // 默认可以用参数为22,3

// Sobel导数边缘检测
void ocvImgProc::doSobel()
{
	if (gray.empty())
		return;
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;

	// 高斯降噪
	GaussianBlur(gray, gray, Size(3,3), 0, 0, BORDER_DEFAULT);
	// 求 X方向梯度
	Sobel( gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
	// 求 Y方向梯度
	Sobel( gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );
	// 将中间结果转换到 CV_8U
	convertScaleAbs( grad_x, abs_grad_x );
	convertScaleAbs( grad_y, abs_grad_y );
	// 将两个方向的梯度相加来求取近似梯度
	addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, dst );
	reset();
}

// Laplacian算子边缘检测
void ocvImgProc::doLaplacian()
{
	if (gray.empty())
		return;
	int kernel_size = 3;
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	// 高斯降噪
	GaussianBlur(gray, gray, Size(3,3), 0, 0, BORDER_DEFAULT);
	Mat tp_dst;
	Laplacian( gray, tp_dst, ddepth, kernel_size, scale, delta, BORDER_DEFAULT );
	convertScaleAbs( tp_dst, dst );	// 转换数值精度，CV_16S转成CV_8U
	reset();
}

/************************形态学处理***************************/

// 腐蚀运算，参数是内核类型和大小
void ocvImgProc::doErosion(int ktype, int ksize)
{
	// 先制作卷积核，核类型分圆形矩形和交叉三种。
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		erode( src, dst, element );
}

// 膨胀运算，参数等同上
void ocvImgProc::doDilation(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		dilate( src, dst, element );
}

// 开运算，等价于先腐蚀后膨胀，用函数morphologyEx
void ocvImgProc::doOpening(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		morphologyEx( src, dst, MORPH_OPEN, element );
}

// 闭运算，等价于先膨胀后腐蚀，参数等同上
void ocvImgProc::doClosing(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		morphologyEx( src, dst, MORPH_CLOSE, element );
}

// 形态梯度，是膨胀图与腐蚀图之差，参数等同上
void ocvImgProc::doMorphGra(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		morphologyEx( src, dst, MORPH_GRADIENT, element );
}

// 顶帽运算，原图像与开运算结果图之差，参数等同上
void ocvImgProc::doTopHat(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		morphologyEx( src, dst, MORPH_TOPHAT, element );
}

// 黑帽运算，闭运算结果图与原图像之差，参数等同上
void ocvImgProc::doBlackHat(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		morphologyEx( src, dst, MORPH_BLACKHAT, element );
}


/************************阈值分割***************************/

// 二值阈值化，分为0或255，两极分化
void ocvImgProc::doThreshBin(int th)
{
	if (th>255)
		th = 255;
	if (th<0)
		th = 0;
	if (!gray.empty())
		threshold( gray, dst, th, 255, THRESH_BINARY );
}

// 反二值阈值化，同上，不过黑白颠倒
void ocvImgProc::doThreshBinInv(int th)
{
	if (th>255)
		th = 255;
	if (th<0)
		th = 0;
	if (!gray.empty())
		threshold( gray, dst, th, 255, THRESH_BINARY_INV );
}

// 截断阈值化，大于该阈值的像素点被设定为该阈值，小于该阈值的保持不变
void ocvImgProc::doThreshTrunc(int th)
{
	if (!gray.empty())
		threshold( gray, dst, th, 255, THRESH_TRUNC);
}

// 阈值化为0，灰度值大于该阈值的不进行任何改变，小于的全设为0
void ocvImgProc::doThreshZero(int th)
{
	if (!gray.empty())
		threshold( gray, dst, th, 255, THRESH_TOZERO );
}

// 反阈值化为0，灰度值小于该阈值的不进行任何改变，大于的全设为0
void ocvImgProc::doThreshZeroInv(int th)
{
	if (!gray.empty())
		threshold( gray, dst, th, 255, THRESH_TOZERO_INV );
}


/************************其他变换***************************/

// 负片处理
void ocvImgProc::doNegative()
{
	if (!src.empty())
		bitwise_not(src,dst);
}

// 垂直翻转
void ocvImgProc::doVFlip()
{
	if (!src.empty())
		flip(src,dst,0);
}

// 水平翻转
void ocvImgProc::doHFlip()
{
	if (!src.empty())
		flip(src,dst,1);
}

// 转置（90度左旋）
void ocvImgProc::doTranspose()
{
	if (!src.empty())
		transpose(src,dst);
}

// 垂直加水平，180度翻转
void ocvImgProc::doVHFlip()
{
	if (!src.empty())
		flip(src,dst,-1);
}

// 亮度对比度处理（矩阵值范围操作）
void ocvImgProc::doConvert(double a, double b)
{
	if (!src.empty())
		src.convertTo(dst, -1, a, b);
}

// 离散傅里叶变换，代码来源于OpenCV教程
void ocvImgProc::doDFT()
{
	if (gray.empty())
		return;

	//将输入图像延扩到最佳的尺寸，利于提升运算速度
	Mat padded;                            //expand input image to optimal size
	int m = getOptimalDFTSize( gray.rows );
	int n = getOptimalDFTSize( gray.cols ); // on the border add zero values
	copyMakeBorder(gray, padded, 0, m - gray.rows, 0, n - gray.cols, BORDER_CONSTANT, Scalar::all(0));

	// 频域值范围远远超过空间值范围， 因此至少要将频域储存在 float 格式中
	// 我们将输入图像转换成浮点类型，并多加一个额外通道来储存复数部分
	Mat planes[] = {Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F)};
	Mat complexI;
	merge(planes, 2, complexI);         // Add to the expanded another plane with zeros

	// 原址运算
	dft(complexI, complexI);            // this way the result may fit in the source matrix

	// 将复数转换为幅度
	// compute the magnitude and switch to logarithmic scale
	// => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
	split(complexI, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
	magnitude(planes[0], planes[1], planes[0]);// planes[0] = magnitude  
	Mat magI = planes[0];

	// 对数尺度(logarithmic scale)缩放. 傅立叶变换的幅度值范围大到不适合在屏幕上显示。
	magI += Scalar::all(1);                    // switch to logarithmic scale
	log(magI, magI);

	// 剪切和重分布幅度图四个象限，角点折到中心
	// crop the spectrum, if it has an odd number of rows or columns
	magI = magI(Rect(0, 0, magI.cols & -2, magI.rows & -2));

	// rearrange the quadrants of Fourier image  so that the origin is at the image center        
	int cx = magI.cols/2;
	int cy = magI.rows/2;

	Mat q0(magI, Rect(0, 0, cx, cy));   // Top-Left - Create a ROI per quadrant 
	Mat q1(magI, Rect(cx, 0, cx, cy));  // Top-Right
	Mat q2(magI, Rect(0, cy, cx, cy));  // Bottom-Left
	Mat q3(magI, Rect(cx, cy, cx, cy)); // Bottom-Right

	Mat tmp;                           // swap quadrants (Top-Left with Bottom-Right)
	q0.copyTo(tmp);
	q3.copyTo(q0);
	tmp.copyTo(q3);

	q1.copyTo(tmp);                    // swap quadrant (Top-Right with Bottom-Left)
	q2.copyTo(q1);
	tmp.copyTo(q2);

	normalize(magI, magI, 0, 1, CV_MINMAX); // Transform the matrix with float values into a 
	// viewable image form (float between values 0 and 1).

//	imshow("test",magI);
	magI.convertTo(dst,CV_8U,255,0);	// 加上这句是将取值范围浮点数0-1转换成灰度0-255，imshow可以显示0-1但是我的函数不行
}
