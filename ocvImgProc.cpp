#include "stdafx.h"
#include "ocvImgProc.h"

using namespace cv;


//////////////////////////////////////////////////////////////////////////
// ����ͼ������

// ���ƻҶ�ֱ��ͼ���������OpenCV�̳��޸ģ�Ҫ�����뵥ͨ��ͼ��
void drawGrayHist(Mat src, Mat &histImage)
{
	if (!src.data)
		return;
	if (src.channels()!=1)
		return;

	Mat hist;	// �洢���������ֱ��ͼ����
	int histSize = 256;
	float range[] = { 0, 256 } ;
	const float* histRange = { range };		// Ϊ��������׼���ʵ�����
	calcHist( &src, 1, 0, Mat(), hist, 1, &histSize, &histRange, 1, 0 );

	int hist_w = 512; int hist_h = 240;		// ��ֱ��ͼ�ĳ��Ϳ�
	int bin_w = cvRound( (double) hist_w/histSize );	// С���εĿ�

	if (!histImage.empty())
		histImage.release();
	histImage = Mat( hist_h, hist_w, CV_8UC3, Scalar(255,255,255) );// ����Ϊ��ɫ

	normalize(hist, hist, 0, hist_h, NORM_MINMAX, -1, Mat() );	// ��һ����0-255

	for( int i = 1; i < histSize; i++ )	// ��һ����С����
	{
		rectangle(histImage, Point( bin_w*(i-1), hist_h ) ,
			Point( bin_w*(i), hist_h - cvRound(hist.at<float>(i)) ),Scalar(0,0,0),CV_FILLED,8,0);
	}
}


// ����RGBֱ��ͼ����������OpenCV�̳̣��������޸�
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



// ��Mat���͹���CImage��
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

	// ��һ��if���Ϻ����������ʾ�Ҷȵ�ͨ��ͼ�� 
	if (channels==1)
	{
		//�����1��ͨ����ͼ��(�Ҷ�ͼ��),CImage�������˵�ɫ�壬����Ҫ�������и�ֵ��
		RGBQUAD* ColorTable;
		int MaxColors=256;
		//�������ͨ��cImage.GetMaxColorTableEntries()�õ���С(�������cImage.Load����ͼ��Ļ�)
		ColorTable = new RGBQUAD[MaxColors];
		cImage.GetColorTable(0,MaxColors,ColorTable);//������ȡ��ָ��
		for (int i=0; i<MaxColors; i++)
		{
			ColorTable[i].rgbBlue = (BYTE)i;
			//BYTE��ucharһ���£���MFC�ж�����
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


// �Ҷ�ת��ͨ��������һ���ò��������һ�����������Ƿ����α��ɫ
void GrayToRGB(Mat &gray, Mat &color, bool isColor)
{
	if (gray.depth()!=CV_8U)
		return;
	if (gray.channels()!=1)
		return;
	if (!isColor)
		cvtColor(gray,color,CV_GRAY2BGR); // ֱ�ӱ����ͨ����ɫͼ��
	else
	{
		color = Mat::zeros(gray.rows, gray.cols, CV_8UC3);
		// ����α��ɫ
		for (int y=0;y<gray.rows;y++)
		{
			for (int x=0;x<gray.cols;x++)
			{
				uchar val = gray.at<uchar>(y,x);
				uchar r,g,b;

				if (val==0)		// �ó������Ǹ��任����
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

// �Ҷȷָ�任�������α��ɫ-Ĭ��nsliceΪ16����ֻ������16ɫ��BGR��
void applyGrayMap(Mat src, Mat &dst, int gmax, int gmin, int nslice)
{
	int gap = gmax - gmin;	// ����ҶȲ�ֵ
	if ((gap<nslice)||nslice<=0)	// nslice-�ֲ���
		return;

	int n = 0;	// ��ɫ����
	int step = gap/nslice;	// ÿ��������
	int width = src.cols;
	int height = src.rows;

	uchar *ps, *pd;
	int map[3][16] = {	// 16ɫBGR��ɫ��
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
			n = (ps[j]-gmin)/step;	// �������ֵ���Կ��
			if (n>=nslice)
				n = nslice-1;		// ���䶥��һ��
			if (n<=0)
				n=0;				// ����׹�һΪ0
			pd[j*3] = map[0][n];
			pd[j*3+1] = map[1][n];
			pd[j*3+2] = map[2][n];
		}    
	}
}


//////////////////////////////////////////////////////////////////////////
// ��ʵ�ֺ���

// Ĭ�Ϲ��������
ocvImgProc::ocvImgProc(void)
{
	inited = false;
}

ocvImgProc::~ocvImgProc(void)
{
	release();
}

// ��ʼ��ͼ�����
void ocvImgProc::init(Mat input, bool copy)
{
	if (copy)
		input.copyTo(src);	// ��ƣ������ݿ���
	else
		src = input;	// ǳ���ƣ��������ļ�ͷ����

	gray = Mat(src.rows,src.cols,CV_8UC1);	// ׼���ҶȻ���ͼ

	if (src.channels()==3)
		cvtColor(src,gray,CV_BGR2GRAY);
	else if (src.channels()==1)
		gray = src;

	inited = true;
}

// �ͷ���Դ
void ocvImgProc::release()
{
	src.release();
	dst.release();
	gray.release();
	inited = false;
}

// ���ûҶȾ���
void ocvImgProc::reset()
{
	if (!src.empty())
	{
		if (src.channels()==3)	// �ı��gray���������
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
// ��ϵ�д����ܺ���

/************************ֱ��ͼ���***************************/

// ���ֱ��ͼ
Mat ocvImgProc::getHist(bool img)
{
	Mat image;	// �ж��Ƕ�ԭͼ����Ŀ��ͼ���ֱ��ͼ
	if(!img)image = src;
	else	image = dst;

	if (image.empty())
		return Mat(240,512,CV_8UC1,Scalar(255,255,255));

	// ��ɫ�ȱ任Ϊ��ɫ����Ϊ���ܶ�Ŀ��ͼ������ʲ���gray
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

//����RGBֱ��ͼ
Mat ocvImgProc::getRGBHist(bool img)
{
	Mat image;	// �ж��Ƕ�ԭͼ����Ŀ��ͼ���ֱ��ͼ
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

// ֱ��ͼ���⻯
void ocvImgProc::doHistEqualization()
{
	Mat temp;
	if (!gray.empty())
		equalizeHist(gray,temp);	// ֱ�ӵ���
	temp.copyTo(dst);
	temp.release();
	reset();
}

// 16�׻Ҷ�תα��ɫ
void ocvImgProc::do16GrayToRGB()
{
	if (!gray.empty())
		applyGrayMap(gray, dst, 255, 0, 16);
}

// �Ҷ�-��ɫ�任����
void ocvImgProc::doPseudoColor(int mode)
{
	if (src.empty())
		return;
	if (mode>11 || mode<0)	// �˴���contrib����ĺ�������12�ֱ任ӳ��
		mode = 2;
	applyColorMap(src, dst, mode);
//	GrayToRGB(gray,dst,1);
}


/************************ͼ���˲�***************************/

// ��ֵ�˲�
void ocvImgProc::doMeanFiltering(int i)
{
	if (src.empty())
		return;
	if (i%2==0)
		i=i+1;
	blur(src,dst,Size(i,i),Point(-1,-1));
}

// ��ֵ�˲�
void ocvImgProc::doMedianFiltering(int i)
{
	if (src.empty())
		return;
	if (i%2==0)
		i=i+1;
	medianBlur(src,dst,i);
}

// ��˹�˲�
void ocvImgProc::doGaussianFiltering(int i)
{
	if (src.empty())
		return;
	if (i%2==0)
		i=i+1;
	GaussianBlur(src, dst, Size(i,i), 0, 0 );
}

// ˫���˲�
void ocvImgProc::doBilateralFiltering(int i)
{
	if (src.empty())
		return;
	if (i%2==0)
		i=i+1;
	bilateralFilter(src, dst, i, i*2, i/2 );	//�ܹ��������
}

/************************��Ե���***************************/

// Canny���ӱ�Ե��⣬ratio�Ǹ���ֵ�Ե���ֵ�ı���
void ocvImgProc::doCanny(int lowTh, int ratio)
{
	if (gray.empty())
		return;
	blur(gray,gray,Size(3,3),Point(-1,-1));	// ��ƽ������
	Canny(gray, dst, lowTh, lowTh*ratio, 3);
	reset();
} // Ĭ�Ͽ����ò���Ϊ22,3

// Sobel������Ե���
void ocvImgProc::doSobel()
{
	if (gray.empty())
		return;
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;

	// ��˹����
	GaussianBlur(gray, gray, Size(3,3), 0, 0, BORDER_DEFAULT);
	// �� X�����ݶ�
	Sobel( gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
	// �� Y�����ݶ�
	Sobel( gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );
	// ���м���ת���� CV_8U
	convertScaleAbs( grad_x, abs_grad_x );
	convertScaleAbs( grad_y, abs_grad_y );
	// ������������ݶ��������ȡ�����ݶ�
	addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, dst );
	reset();
}

// Laplacian���ӱ�Ե���
void ocvImgProc::doLaplacian()
{
	if (gray.empty())
		return;
	int kernel_size = 3;
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	// ��˹����
	GaussianBlur(gray, gray, Size(3,3), 0, 0, BORDER_DEFAULT);
	Mat tp_dst;
	Laplacian( gray, tp_dst, ddepth, kernel_size, scale, delta, BORDER_DEFAULT );
	convertScaleAbs( tp_dst, dst );	// ת����ֵ���ȣ�CV_16Sת��CV_8U
	reset();
}

/************************��̬ѧ����***************************/

// ��ʴ���㣬�������ں����ͺʹ�С
void ocvImgProc::doErosion(int ktype, int ksize)
{
	// ����������ˣ������ͷ�Բ�ξ��κͽ������֡�
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		erode( src, dst, element );
}

// �������㣬������ͬ��
void ocvImgProc::doDilation(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		dilate( src, dst, element );
}

// �����㣬�ȼ����ȸ�ʴ�����ͣ��ú���morphologyEx
void ocvImgProc::doOpening(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		morphologyEx( src, dst, MORPH_OPEN, element );
}

// �����㣬�ȼ��������ͺ�ʴ��������ͬ��
void ocvImgProc::doClosing(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		morphologyEx( src, dst, MORPH_CLOSE, element );
}

// ��̬�ݶȣ�������ͼ�븯ʴͼ֮�������ͬ��
void ocvImgProc::doMorphGra(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		morphologyEx( src, dst, MORPH_GRADIENT, element );
}

// ��ñ���㣬ԭͼ���뿪������ͼ֮�������ͬ��
void ocvImgProc::doTopHat(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		morphologyEx( src, dst, MORPH_TOPHAT, element );
}

// ��ñ���㣬��������ͼ��ԭͼ��֮�������ͬ��
void ocvImgProc::doBlackHat(int ktype, int ksize)
{
	Mat element = getStructuringElement( ktype,
		Size( 2*ksize + 1, 2*ksize+1 ),
		Point( ksize, ksize ) );
	if (!src.empty())
		morphologyEx( src, dst, MORPH_BLACKHAT, element );
}


/************************��ֵ�ָ�***************************/

// ��ֵ��ֵ������Ϊ0��255�������ֻ�
void ocvImgProc::doThreshBin(int th)
{
	if (th>255)
		th = 255;
	if (th<0)
		th = 0;
	if (!gray.empty())
		threshold( gray, dst, th, 255, THRESH_BINARY );
}

// ����ֵ��ֵ����ͬ�ϣ������ڰ׵ߵ�
void ocvImgProc::doThreshBinInv(int th)
{
	if (th>255)
		th = 255;
	if (th<0)
		th = 0;
	if (!gray.empty())
		threshold( gray, dst, th, 255, THRESH_BINARY_INV );
}

// �ض���ֵ�������ڸ���ֵ�����ص㱻�趨Ϊ����ֵ��С�ڸ���ֵ�ı��ֲ���
void ocvImgProc::doThreshTrunc(int th)
{
	if (!gray.empty())
		threshold( gray, dst, th, 255, THRESH_TRUNC);
}

// ��ֵ��Ϊ0���Ҷ�ֵ���ڸ���ֵ�Ĳ������κθı䣬С�ڵ�ȫ��Ϊ0
void ocvImgProc::doThreshZero(int th)
{
	if (!gray.empty())
		threshold( gray, dst, th, 255, THRESH_TOZERO );
}

// ����ֵ��Ϊ0���Ҷ�ֵС�ڸ���ֵ�Ĳ������κθı䣬���ڵ�ȫ��Ϊ0
void ocvImgProc::doThreshZeroInv(int th)
{
	if (!gray.empty())
		threshold( gray, dst, th, 255, THRESH_TOZERO_INV );
}


/************************�����任***************************/

// ��Ƭ����
void ocvImgProc::doNegative()
{
	if (!src.empty())
		bitwise_not(src,dst);
}

// ��ֱ��ת
void ocvImgProc::doVFlip()
{
	if (!src.empty())
		flip(src,dst,0);
}

// ˮƽ��ת
void ocvImgProc::doHFlip()
{
	if (!src.empty())
		flip(src,dst,1);
}

// ת�ã�90��������
void ocvImgProc::doTranspose()
{
	if (!src.empty())
		transpose(src,dst);
}

// ��ֱ��ˮƽ��180�ȷ�ת
void ocvImgProc::doVHFlip()
{
	if (!src.empty())
		flip(src,dst,-1);
}

// ���ȶԱȶȴ�������ֵ��Χ������
void ocvImgProc::doConvert(double a, double b)
{
	if (!src.empty())
		src.convertTo(dst, -1, a, b);
}

// ��ɢ����Ҷ�任��������Դ��OpenCV�̳�
void ocvImgProc::doDFT()
{
	if (gray.empty())
		return;

	//������ͼ����������ѵĳߴ磬�������������ٶ�
	Mat padded;                            //expand input image to optimal size
	int m = getOptimalDFTSize( gray.rows );
	int n = getOptimalDFTSize( gray.cols ); // on the border add zero values
	copyMakeBorder(gray, padded, 0, m - gray.rows, 0, n - gray.cols, BORDER_CONSTANT, Scalar::all(0));

	// Ƶ��ֵ��ΧԶԶ�����ռ�ֵ��Χ�� �������Ҫ��Ƶ�򴢴��� float ��ʽ��
	// ���ǽ�����ͼ��ת���ɸ������ͣ������һ������ͨ�������渴������
	Mat planes[] = {Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F)};
	Mat complexI;
	merge(planes, 2, complexI);         // Add to the expanded another plane with zeros

	// ԭַ����
	dft(complexI, complexI);            // this way the result may fit in the source matrix

	// ������ת��Ϊ����
	// compute the magnitude and switch to logarithmic scale
	// => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
	split(complexI, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
	magnitude(planes[0], planes[1], planes[0]);// planes[0] = magnitude  
	Mat magI = planes[0];

	// �����߶�(logarithmic scale)����. ����Ҷ�任�ķ���ֵ��Χ�󵽲��ʺ�����Ļ����ʾ��
	magI += Scalar::all(1);                    // switch to logarithmic scale
	log(magI, magI);

	// ���к��طֲ�����ͼ�ĸ����ޣ��ǵ��۵�����
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
	magI.convertTo(dst,CV_8U,255,0);	// ��������ǽ�ȡֵ��Χ������0-1ת���ɻҶ�0-255��imshow������ʾ0-1�����ҵĺ�������
}
