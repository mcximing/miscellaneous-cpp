#include "StdAfx.h"
#include <windows.h>
#include <shlobj.h>
#include <time.h>
#include <fstream>
#include <io.h>
#include "StereoFunc.h"

using namespace std;
using namespace cv;


/************************************************************************/
// 全局变量定义
/************************************************************************/

char CurrExeDirectory[MAX_PATH];// 当前程序所在目录
CString calibFilePath;			// 标定图像文件列表路径

Mat currImg1;	// 视图1当前图像
Mat currImg2;	// 视图2当前图像
Mat currImg3;	// 视图3当前图像

int m_nx;					// 棋盘行角点数 w
int m_ny;					// 棋盘列角点数 h
int boardCornerNum;			// 棋盘角点总数= w*h
cv::Size patternSize;		// 棋盘行列角点=(w,h)
cv::Size imageSize;			// 图像分辨率
float squareWidth;			// 棋盘格子边长
int imgNum;					// 用于标定的图像对数
int useNum;					// 有效图片数目
vector<Point2f> corners;	// 存放角点坐标，注意vector一定要初始化
vector<vector<Point3f>>	objectPoints;	// 棋盘角点世界坐标序列
vector<vector<Point2f>>	imagePoints1;	// 左视图的棋盘角点像素坐标序列
vector<vector<Point2f>>	imagePoints2;	// 右视图的棋盘角点像素坐标序列

// 以下几段是实际使用的变量
Mat intrinsic_matrix1;	// 左视图的内参数矩阵3*3
Mat intrinsic_matrix2;	// 右视图的内参数矩阵3*3
Mat distortion_coeffs1;	// 左视图的径向畸变系数矩阵1*4
Mat distortion_coeffs2;	// 右视图的径向畸变系数矩阵1*4
vector<Mat> rotation_vectors1;		// 左视图的旋转矩阵
vector<Mat> rotation_vectors2;		// 右视图的旋转矩阵
vector<Mat> translation_vectors1;	// 左视图的平移矩阵
vector<Mat> translation_vectors2;	// 右视图的平移矩阵
double err1;	// 左右视图标定误差值
double err2;
bool calib1;	// 标定指示
bool calib2; 
bool calib;
bool rectify;

Mat rotation;		// 旋转矩阵
Mat translation;	// 平移矩阵
Mat essential;		// 本征矩阵
Mat foundational;	// 基础矩阵

Mat mX1;	// 左视图 X 方向像素映射矩阵
Mat mY1;	// 左视图 Y 方向像素映射矩阵
Mat mX2;	// 右视图 X 方向像素映射矩阵
Mat	mY2;	// 右视图 Y 方向像素映射矩阵
Mat	QMatrix;	// 用于计算三维点云的 Q 矩阵
cv::Rect rcRoi1;// 左视图有效区域的矩形
cv::Rect rcRoi2;// 右视图有效区域的矩形

Mat img1r;		// 校正后的左右图像
Mat img2r;

StereoBM m_BM;		// 立体匹配 BM 方法
StereoSGBM m_SGBM;	// 立体匹配 SGBM 方法
StereoVar m_VAR;	// 立体匹配 VAR 方法（实验）

int MatchAlgorithm;	// 匹配算法选择参数，0-BM，1-SGBM，2-VAR


/************************************************************************/
// 全局函数实现
/************************************************************************/

// 选择文件夹
CString GetFolderPath(HWND hParent)
{
	TCHAR szDir[MAX_PATH];//用来存储路径的字符串
	CString strPath = _T("");
	BROWSEINFO bInfo;
	ZeroMemory(&bInfo, sizeof(bInfo));
	bInfo.hwndOwner = hParent;
	bInfo.pidlRoot = NULL;
	bInfo.pszDisplayName = szDir;
	bInfo.lpszTitle = _T("请选择路径: ");
	bInfo.ulFlags = BIF_RETURNONLYFSDIRS;
	bInfo.lpfn = NULL;
	bInfo.lParam = 0;
	bInfo.iImage = 0;

	LPITEMIDLIST lpDlist; //用来保存返回信息的IDList
	lpDlist = SHBrowseForFolder(&bInfo) ; //显示选择对话框
	if(lpDlist != NULL)   //用户按了确定按钮
	{
		SHGetPathFromIDList(lpDlist, szDir);//把项目标识列表转化成字符串
		strPath = szDir; //将TCHAR类型的字符串转换为CString类型的字符串
	}
	return strPath;
}


// 由Mat类型构建CImage类
void MatToCImage( Mat &mat, CImage &cImage)
{
	//create new CImage
	int width    = mat.cols;
	int height   = mat.rows;
	int channels = mat.channels();

	cImage.Destroy(); //clear
	cImage.Create(width, 
		height, //positive: left-bottom-up   or negative: left-top-down
		8*channels ); //numbers of bits per pixel

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



// 参数的默认初始化
void LoadDefaultParam()
{
	GetModuleFileNameA(NULL, CurrExeDirectory, MAX_PATH); //得到程序模块.exe全路径
	strrchr( CurrExeDirectory, '\\')[1]= 0;	// 逆序查找\符号，截掉后边的.exe文件名，即是程序目录

	calibFilePath = _T("");
	useNum = 0;
	imgNum = 4;
	m_nx = 8;
	m_ny = 6;
	boardCornerNum = m_nx * m_ny;
	patternSize = Size(m_nx, m_ny);
	imageSize = Size(320, 240);
	squareWidth = 30;
	err1 = err2 = 0;
	calib1 = FALSE;
	calib2 = FALSE;
	calib = FALSE;
	rectify = FALSE;
	MatchAlgorithm = 0;
	ResetParam();

	// 立体匹配相关参数
	// 影响大的主要参数是 SADWindowSize、numberOfDisparities 和 uniquenessRatio 三个
	//	m_BM.state->preFilterSize = 15;	//预处理滤波器窗口大小，容许范围是[5,255]，一般应该在 5x5..21x21 之间，参数必须为奇数值, int 型
	m_BM.state->preFilterCap = 31;	//预处理滤波器的截断值，预处理输出值仅保留[-Cap, Cap]范围内的值，参数范围：1-31, int
	m_BM.state->SADWindowSize = 33;	//SAD窗口大小，容许范围是[5,255]，一般应该在 5x5 至 21x21 之间，参数必须是奇数，int 型
	m_BM.state->minDisparity = 0;	//最小视差，默认值为 0, 可以是负值，int 型
	m_BM.state->numberOfDisparities = 64;	//视差窗口，即最大视差值与最小视差值之差, 窗口大小必须是 16 的整数倍，int 型
	m_BM.state->textureThreshold = 10;	//低纹理区域的判断阈值。
	m_BM.state->uniquenessRatio = 15;	//视差唯一性百分比，该参数不能为负值，一般5-15左右的值比较合适，int 型
	m_BM.state->speckleWindowSize = 100;//检查视差连通区域变化度的窗口大小, 值为 0 时取消 speckle 检查，int 型
	m_BM.state->speckleRange = 32;		//视差变化阈值，当窗口内视差变化大于阈值时，该窗口内的视差清零，int 型
	m_BM.state->disp12MaxDiff = -1;		//左视差图（直接计算得出）和右视差图（通过cvValidateDisparity计算得出）之间的最大容许差异。

	m_SGBM.preFilterCap = 63;	//预处理滤波器的截断值
	m_SGBM.SADWindowSize = 9;	//SAD窗口大小，容许范围是[1,11]，一般应该在 3x3 至 11x11 之间，参数必须是奇数
	m_SGBM.P1 = 8*m_SGBM.SADWindowSize*m_SGBM.SADWindowSize;	//控制视差变化平滑性的参数。P1、P2的值越大，视差越平滑。
	m_SGBM.P2 = 32*m_SGBM.SADWindowSize*m_SGBM.SADWindowSize;	//P2必须大于P1
	m_SGBM.minDisparity = 0;	//最小视差，默认值为 0, 可以是负值，int 型
	m_SGBM.numberOfDisparities = 64;//视差窗口，即最大视差值与最小视差值之差, 窗口大小必须是 16 的整数倍，int 型
	m_SGBM.uniquenessRatio = 15;	//视差唯一性百分比，该参数不能为负值，一般5-15左右的值比较合适，int 型	
	m_SGBM.speckleWindowSize = 100;	//检查视差连通区域变化度的窗口大小, 值为 0 时取消 speckle 检查，int 型
	m_SGBM.speckleRange = 32;		//视差变化阈值，当窗口内视差变化大于阈值时，该窗口内的视差清零，int 型
	m_SGBM.disp12MaxDiff = -1;		//左视差图（直接计算得出）和右视差图（通过cvValidateDisparity计算得出）之间的最大容许差异。
	m_SGBM.fullDP = false;

	m_VAR.levels = 3;			// ignored with USE_AUTO_PARAMS
	m_VAR.pyrScale = 0.5;		// ignored with USE_AUTO_PARAMS
	m_VAR.nIt = 25;
	m_VAR.minDisp = -32;
	m_VAR.maxDisp = 32;
	m_VAR.poly_n = 7;
	m_VAR.poly_sigma = 1.5;
	m_VAR.fi = 15.0f;
	m_VAR.lambda = 0.03f;
	m_VAR.penalization = m_VAR.PENALIZATION_TICHONOV;   // ignored with USE_AUTO_PARAMS
	m_VAR.cycle = m_VAR.CYCLE_V;                        // ignored with USE_AUTO_PARAMS
	m_VAR.flags = m_VAR.USE_SMART_ID | m_VAR.USE_AUTO_PARAMS | m_VAR.USE_INITIAL_DISPARITY | m_VAR.USE_MEDIAN_FILTERING ;

}


// 更新标定输入参数，会清空图像点数据
void ResetParam()
{
	patternSize = cvSize(m_nx,m_ny);
	boardCornerNum = m_nx * m_ny;
	// 调整vector分配的空间，不初始化也会报错
	corners.resize(boardCornerNum,Point2f(0,0));
	objectPoints.clear();	// 要clear后再resize才能将两层的vector重新分配
	imagePoints1.clear();	// 直接resize只能重新分配外层vector的数目
	imagePoints2.clear();
	objectPoints.resize(useNum, vector<Point3f>(boardCornerNum, Point3f(0,0,0)));
	imagePoints1.resize(useNum, vector<Point2f>(boardCornerNum, Point2f(0,0)));
	imagePoints2.resize(useNum, vector<Point2f>(boardCornerNum, Point2f(0,0)));
}


// 检测图像角点
int FindCorner(Mat &img)
{
	if (img.empty())
		return -1;

	Mat gray = Mat(img.rows,img.cols,CV_8UC1); // 构造灰度图
	cvtColor(img,gray,CV_BGR2GRAY);

	// 核心函数
	bool patternFound = findChessboardCorners(gray, patternSize, corners,
		CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE
		+ CALIB_CB_FAST_CHECK);

	// 细化
	if(patternFound)
		cornerSubPix(gray, corners, Size(11, 11), Size(-1, -1), 
		TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.05));
	else
		return 0;

	// 绘制找到的角点
	if(patternFound)
		drawChessboardCorners(img, patternSize, Mat(corners), patternFound);
	return 1;
}


// 读取文件夹标定图像样本并判决
void judgeCalibFiles(CString path, vector<bool>& ifGood, vector<string>& goodImg)
{
	Mat currImg;	// 当前图像
	CString pathWild = path + _T("\\*.jpg");	// 欲寻找的文件路径全名
	string strName;	// 存储完整路径带文件名
	string temp;	// 存储路径名不带文件名
	USES_CONVERSION;
	strName = T2A(path);
	strName = strName + "\\";
	temp = strName;
	char * pPath = T2A(pathWild);	// 路径名

	struct _finddata_t c_file;		// 查找文件结构
	long hFile;

	if( (hFile = _findfirst(pPath, &c_file)) == -1L )
	{
		fprintf(stderr, "找不到图像文件！\n");
		return;
	}
	else
	{
		do {
			strName = temp;
			strName = strName + c_file.name;
			currImg =  imread(strName,1);
			if (currImg.empty())
			{
				ifGood.push_back(FALSE);
				goodImg.push_back("");
				continue;
			}
			if (FindCorner(currImg)>0)// 成功找到角点
			{
				ifGood.push_back(TRUE);
				goodImg.push_back(strName);
			}
			else
			{
				ifGood.push_back(FALSE);
				goodImg.push_back("");
			}
		} while (_findnext(hFile, &c_file) == 0);
	}
	_findclose(hFile);
}


// 判别标定图像样本是否可以用于标定
int doJudgeForCalib(CString path) // 参数：样本路径
{
	if (path==_T(""))	// 样本路径为空
		return 0;
	
	ResetParam();

	CString pathL,pathR;		// 约定该文件夹下分为左右两文件夹
	pathL = path + _T("\\L");
	pathR = path + _T("\\R");

	calibFilePath = CurrExeDirectory;
	calibFilePath += _T("\\CalibImgName.txt"); // 保存文件名的TXT文件
	ofstream fout(calibFilePath);

	vector<bool> ifGood1;
	vector<bool> ifGood2;
	vector<string> goodImg1;
	vector<string> goodImg2;

	judgeCalibFiles(pathL, ifGood1, goodImg1);
	judgeCalibFiles(pathR, ifGood2, goodImg2);

	imgNum = ifGood1.size();	// 向量长度即图片数
	useNum = 0;
	if (imgNum!=ifGood2.size())	// 左右向量大小不一致
	{
		fprintf(stderr, "左右向量大小不一致！\n");
		return -1;
	}
	if (imgNum==0)	// 没有可用的图片
	{
		fprintf(stderr, "没有可用的图片！\n");
		return -2;
	}

	for (int i=0; i<imgNum; i++)	// 往TXT里写可用于标定的文件名
	{
		if (ifGood1[i]&&ifGood2[i])
		{
			fout<<goodImg1[i]<<"\n";
			useNum++;
		}
	}
	for (int i=0; i<imgNum; i++)
	{
		if (ifGood1[i]&&ifGood2[i])
		{
			fout<<goodImg2[i]<<"\n";
		}
	}
	imgNum = useNum;
	//char msg[32];
	//memset(msg,0,32);
	//sprintf_s(msg,32,"标定样本判别结束，有效图片%d对\n", useNum);
	//printf(msg);
	return useNum;
}



// 计算棋盘角点的世界坐标
void getObjectPoints()
{
	boardCornerNum = m_nx * m_ny;
	objectPoints.clear();
	objectPoints.resize(useNum, vector<Point3f>(boardCornerNum, Point3f(0,0,0)));
	int n = 0;
	for(int i=0; i < useNum; i++ )
	{
		n = 0;
		for( int j = 0; j < patternSize.height; j++ )
			for( int k = 0; k < patternSize.width; k++ )
				objectPoints[i][n++] = Point3f(j*squareWidth, k*squareWidth, 0);
	}
}


// 通过读文件进行单目标定
int doSingleCalibLR()
{
	if (calibFilePath==_T(""))	// 查不到标定文件列表
	{
		fprintf(stderr, "查不到标定文件列表！\n");
		return -1;
	}
	ResetParam();
	string fileName;
	ifstream fin(calibFilePath);

	int64 t = getTickCount();

	// 打开图片获取角点，并计算棋盘角点的像素坐标imagePoints
	for (int i=0; i<useNum; i++)
	{
		getline(fin,fileName);
		currImg1 = imread(fileName,1);
		FindCorner(currImg1);
		for (int j=0; j<boardCornerNum; j++)
			imagePoints1[i][j] = Point2f(corners[j].x,corners[j].y);
	}
	for (int i=0; i<useNum; i++)
	{
		getline(fin,fileName);
		currImg2 = imread(fileName,1);
		FindCorner(currImg2);
		for (int j=0; j<boardCornerNum; j++)
			imagePoints2[i][j] = Point2f(corners[j].x,corners[j].y);
	}

	imageSize = currImg2.size();
	if (imageSize == Size(0,0))
	{
		fprintf(stderr, "读取标定文件出错！\n");
		return -2;
	}

	// 计算棋盘角点的世界坐标objectPoints
	getObjectPoints();

	// 标定左摄像机
	calibrateCamera(
		objectPoints,
		imagePoints1,
		imageSize,
		intrinsic_matrix1,
		distortion_coeffs1,
		rotation_vectors1,
		translation_vectors1,
		CV_CALIB_FIX_ASPECT_RATIO || CV_CALIB_FIX_K3 || CV_CALIB_FIX_PRINCIPAL_POINT 
		);

	//err1 = singleCalibEvaluate(imagePoints1);
	saveLeftCalibXml();
	calib1 = true;

	// 标定右摄像机
	calibrateCamera(
		objectPoints,
		imagePoints2,
		imageSize,
		intrinsic_matrix2,
		distortion_coeffs2,
		rotation_vectors2,
		translation_vectors2,
		CV_CALIB_FIX_ASPECT_RATIO || CV_CALIB_FIX_K3 || CV_CALIB_FIX_PRINCIPAL_POINT 
		);

	//err2 = singleCalibEvaluate(imagePoints2);
	saveRightCalibXml();
	calib2 = true;
	
	t = getTickCount() - t;
	return (int)(t/getTickFrequency()*1000);	// 返回运行时间毫秒值

	//printf("左右单目标定结束.\n");
}



// 双目标定功能函数
int doStereoCamCalib()
{
	//*******************注意，暂时没有处理左右对应图片中有一方单目标定失败的情况。
	if (!(calib1&&calib2))
	{
		fprintf(stderr, "左右单目标定未完成！\n");
		return -1;
	}
	int64 t = getTickCount();
	
	stereoCalibrate(
		objectPoints,
		imagePoints1,
		imagePoints2,
		intrinsic_matrix1,
		distortion_coeffs1,
		intrinsic_matrix2,
		distortion_coeffs2,
		imageSize,
		rotation,
		translation,
		essential,
		foundational,
		cv::TermCriteria(cv::TermCriteria::COUNT+cv::TermCriteria::EPS, 30, 1e-6),
		CV_CALIB_FIX_K3 + CV_CALIB_FIX_K4 + CV_CALIB_FIX_K5
		);

	calib = true;

	t = getTickCount() - t;

	//char timeShow[16];
	//memset(timeShow,0,16);
	//sprintf_s(timeShow, 16, "双目标定结束，耗时: %0.2fs\n", t/getTickFrequency());
	//printf(timeShow);

	saveStereoCalibXml();
	
	return (int)(t/getTickFrequency()*1000);	// 返回运行时间毫秒值
}



// 双目校正功能函数
int doStereoCamRectify()
{
	if (!calib)
	{
		if (loadStereoCalibData()==0) // 没有标定则尝试从文件读取
		{
			fprintf(stderr, "未检测到双目标定数据\n");
			return 0;
		}
		calib = TRUE;
	}

	//初始化
	mX1 = Mat(imageSize, CV_32FC1);
	mY1 = Mat(imageSize, CV_32FC1);
	mX2 = Mat(imageSize, CV_32FC1);
	mY2 = Mat(imageSize, CV_32FC1);

	Mat R1, R2, P1, P2, Q;
	Rect roi1, roi2;
	double alpha = -1;

	//执行双目校正
	stereoRectify(
		intrinsic_matrix1,
		distortion_coeffs1,
		intrinsic_matrix2,
		distortion_coeffs2,
		imageSize,
		rotation,
		translation,
		R1, R2, P1, P2, Q, 
		CV_CALIB_ZERO_DISPARITY,
		alpha, 
		imageSize,
		&roi1, &roi2);

	//使用HARTLEY方法的额外处理
	if (0)
	{
		Mat F, H1, H2;
		F = findFundamentalMat(
			imagePoints1[0],
			imagePoints2[0],
			CV_FM_8POINT, 0, 0);
		stereoRectifyUncalibrated(
			imagePoints1[0],
			imagePoints2[0],
			F, imageSize, H1, H2, 3);

		R1 = intrinsic_matrix1.inv() * H1 * intrinsic_matrix1;
		R2 = intrinsic_matrix2.inv() * H2 * intrinsic_matrix2;
		P1 = intrinsic_matrix1;
		P2 = intrinsic_matrix2;
	}

	//生成图像校正所需的像素映射矩阵
	initUndistortRectifyMap(
		intrinsic_matrix1,
		distortion_coeffs1,
		R1, P1, 
		imageSize,
		CV_16SC2,
		mX1, mY1);

	initUndistortRectifyMap(
		intrinsic_matrix2,
		distortion_coeffs2,
		R2, P2, 
		imageSize,
		CV_16SC2,
		mX2, mY2);

	//输出数据
	Q.copyTo(QMatrix);
	rcRoi1 = roi1;
	rcRoi2 = roi2;

	//printf("校正数据已生成.\n");
	saveRectifyXml();
	rectify = true;
	
	return 1;
}



// 对图像进行校正
int remapImage(Mat img1, Mat img2)
{
	if (!rectify)	// 检查校正数据
	{
		if (loadRectifyData()==0) // 没有数据则尝试从文件读取
		{
			fprintf(stderr, "找不到校正数据！\n");
			return -1;
		}
		rectify = TRUE;
	}
	if (img1.empty() || img2.empty())	// 没有图像传入
	{
		fprintf(stderr, "传入图像错误！\n");
		return 0;
	}

//	ResetParam();

	if ( !mX1.empty() && !mY1.empty() )
	{
		remap(img1, img1r, mX1, mY1, INTER_LINEAR);
	}
	if ( !mX2.empty() && !mY2.empty() )
	{
		remap(img2, img2r, mX2, mY2, INTER_LINEAR);
	}
	return 1;
}


// 立体匹配BM算法
void bmMatch()
{
	// 左右视图的有效像素区域，一般由双目校正阶段的 cvStereoRectify 函数传递，也可以自行设定。
	m_BM.state->roi1 = rcRoi1;
	m_BM.state->roi2 = rcRoi2;

	// 转换为灰度图
	cv::Mat img1gray, img2gray;
	cvtColor(img1r, img1gray, CV_BGR2GRAY);
	cvtColor(img2r, img2gray, CV_BGR2GRAY);

	// 对左右视图的左边进行边界延拓，以获取与原始视图相同大小的有效视差区域
	Mat img1border, img2border;
	copyMakeBorder(img1gray, img1border, 0, 0, m_BM.state->numberOfDisparities, 0, IPL_BORDER_REPLICATE);
	copyMakeBorder(img2gray, img2border, 0, 0, m_BM.state->numberOfDisparities, 0, IPL_BORDER_REPLICATE);

	// 计算视差
	Mat dispBorder, disp;
	m_BM(img1border, img2border, dispBorder);

	// 校正后的有效像素区域掩模
	Mat m_Calib_Mat_Mask_Roi = Mat::zeros(imageSize.height, imageSize.width, CV_8UC1);
	cv::rectangle(m_Calib_Mat_Mask_Roi, rcRoi1, Scalar(255), CV_FILLED);

	// 截取与原始画面对应的视差区域（舍去加宽的部分）
	disp = dispBorder.colRange(m_BM.state->numberOfDisparities, dispBorder.cols);
	disp.copyTo(disp, m_Calib_Mat_Mask_Roi);
	disp.convertTo(currImg3, CV_8U, 255/(m_BM.state->numberOfDisparities*16.));

	//Mat disp;
	//m_BM(img1gray,img2gray,disp);
	//disp.convertTo(currImg3, CV_8U, 255/(m_BM.state->numberOfDisparities*16.));

//	ShowImage(currImg3,imgShow3,IDC_VISION3,0);
//	imwrite("disparity_bm.jpg",currImg3);
}


// 立体匹配SGBM算法
void sgbmMatch()
{
	// 转换为灰度图
	Mat img1gray, img2gray;
	cvtColor(img1r, img1gray, CV_BGR2GRAY);
	cvtColor(img2r, img2gray, CV_BGR2GRAY);

	// 对左右视图的左边进行边界延拓，以获取与原始视图相同大小的有效视差区域
	Mat img1border, img2border;
	copyMakeBorder(img1gray, img1border, 0, 0, m_SGBM.numberOfDisparities, 0, IPL_BORDER_REPLICATE);
	copyMakeBorder(img2gray, img2border, 0, 0, m_SGBM.numberOfDisparities, 0, IPL_BORDER_REPLICATE);

	// 计算视差
	Mat disp, dispBorder;
	m_SGBM(img1border, img2border, dispBorder);

	// 校正后的有效像素区域掩模
	Mat m_Calib_Mat_Mask_Roi = Mat::zeros(imageSize.height, imageSize.width, CV_8UC1);
	rectangle(m_Calib_Mat_Mask_Roi, rcRoi1, Scalar(255), CV_FILLED);

	// 截取与原始画面对应的视差区域（舍去加宽的部分）
	disp = dispBorder.colRange(m_SGBM.numberOfDisparities, img1border.cols);
	disp.copyTo(disp, m_Calib_Mat_Mask_Roi);
	disp.convertTo(currImg3, CV_8U, 255/(m_SGBM.numberOfDisparities*16.));

	//Mat disp, disp8;
	//m_SGBM(img1gray,img2gray,disp);
	//disp.convertTo(currImg3, CV_8U, 255/(m_SGBM.numberOfDisparities*16.));

//	ShowImage(currImg3,imgShow3,IDC_VISION3,0);
//	imwrite("disparity_sgbm.jpg",currImg3);
}


// 立体匹配VAR算法
void varMatch()
{
	// 转换为灰度图
	Mat img1gray, img2gray;
	cvtColor(img1r, img1gray, CV_BGR2GRAY);
	cvtColor(img2r, img2gray, CV_BGR2GRAY);

	Mat disp, disp8;
	m_VAR(img1gray, img2gray, disp); // 实施计算
	disp.convertTo(currImg3, CV_8U, 1/*255/(64*16.)*/);	// 变换值域

//	ShowImage(currImg3,imgShow3,IDC_VISION3,0);
//	imwrite("disparity_var.jpg",currImg3);
}






// 载入双目标定参数和角点参数供双目校正
int loadStereoCalibData()
{
	char filename[MAX_PATH];
	strcpy_s(filename, MAX_PATH, CurrExeDirectory);
	strcat_s(filename, MAX_PATH, "\\stereo_calib_params.xml");
	FileStorage fs(filename, FileStorage::READ);
	if ( !fs.isOpened() )
	{
		return 0;
	}

	FileNodeIterator it = fs["imageSize"].begin(); 
	it >> imageSize.width >> imageSize.height;

	fs ["used_boards_num"]>> useNum;

	fs ["intrinsic_matrix1"] >> intrinsic_matrix1;
	fs ["distortion_coeffs1"] >> distortion_coeffs1;
	fs ["intrinsic_matrix2"] >> intrinsic_matrix2;
	fs ["distortion_coeffs2"] >> distortion_coeffs2;

	fs ["rotation"] >> rotation;
	fs ["translation"] >> translation;
	fs ["essential"] >> essential;
	fs ["foundational"] >> foundational;

	fs.release();
	return 1;
}



// 载入双目校正参数供校正图像进行匹配
int loadRectifyData()
{
	char filename[MAX_PATH];
	strcpy_s(filename, MAX_PATH, CurrExeDirectory);
	strcat_s(filename, MAX_PATH, "\\stereo_rectify_params.xml");
	FileStorage fs(filename, FileStorage::READ);
	if ( !fs.isOpened() )
	{
		return 0;
	}

	FileNodeIterator it = fs["imageSize"].begin();
	it >> imageSize.width >> imageSize.height;

	vector<int> roiVal1;
	vector<int> roiVal2;

	fs["leftValidArea"] >> roiVal1;
	rcRoi1.x = roiVal1[0];
	rcRoi1.y = roiVal1[1];
	rcRoi1.width = roiVal1[2];
	rcRoi1.height = roiVal1[3];

	fs["rightValidArea"] >> roiVal2;
	rcRoi2.x = roiVal2[0];
	rcRoi2.y = roiVal2[1];
	rcRoi2.width = roiVal2[2];
	rcRoi2.height = roiVal2[3];

	fs["QMatrix"] >> QMatrix;
	fs["remapX1"] >> mX1;
	fs["remapY1"] >> mY1;
	fs["remapX2"] >> mX2;
	fs["remapY2"] >> mY2;

	fs.release();
	return 1;
}



// 从配置文件载入各项参数
int loadConfiguration()
{
	LoadDefaultParam();	// 先载入全部默认参数，而后再根据配置里的部分覆盖

	char filename[MAX_PATH];
	strcpy_s(filename, MAX_PATH, CurrExeDirectory);
	strcat_s(filename, MAX_PATH, "\\config_params.xml");
	FileStorage fsr(filename, FileStorage::READ);
	if ( !fsr.isOpened() )
	{
		LoadDefaultParam();
		saveConfiguration();
		return 0;
	}

	// 以下两个参数在离线标定时并无外部输入的必要，而是根据图片确定。
	fsr["useNum"] >> useNum;
	FileNodeIterator it = fsr["imageSize"].begin();
	it >> imageSize.width >> imageSize.height;

	fsr["m_nx"] >> m_nx;
	fsr["m_ny"] >> m_ny;
	boardCornerNum = m_nx * m_ny;	// 该值为计算得来
	patternSize = Size(m_nx, m_ny);	// 该值为计算得来
	fsr["squareWidth"] >> squareWidth;

	// 立体匹配相关参数

	fsr["MatchAlgorithm"] >> MatchAlgorithm;

	fsr["BM_minDisp"] >> m_BM.state->minDisparity;
	fsr["BM_maxDisp"] >> m_BM.state->numberOfDisparities;
	m_BM.state->numberOfDisparities -= m_BM.state->minDisparity;
	fsr["BM_preFilterCap"] >> m_BM.state->preFilterCap;
	fsr["BM_uniqRatio"] >> m_BM.state->uniquenessRatio;
	fsr["BM_SADWinSize"] >> m_BM.state->SADWindowSize;
	fsr["BM_specRange"] >> m_BM.state->speckleRange;
	fsr["BM_specWinSize"] >> m_BM.state->speckleWindowSize;
	fsr["BM_disp12MaxDiff"] >> m_BM.state->disp12MaxDiff;
	fsr["BM_texThres"] >> m_BM.state->textureThreshold;

	fsr["SGBM_minDisp"] >> m_SGBM.minDisparity;
	fsr["SGBM_maxDisp"] >> m_SGBM.numberOfDisparities;
	m_SGBM.numberOfDisparities -= m_SGBM.minDisparity;
	fsr["SGBM_preFilterCap"] >> m_SGBM.preFilterCap;
	fsr["SGBM_uniqRatio"] >> m_SGBM.uniquenessRatio;
	fsr["SGBM_SADWinSize"] >> m_SGBM.SADWindowSize;
	fsr["SGBM_specRange"] >> m_SGBM.speckleRange;
	fsr["SGBM_specWinSize"] >> m_SGBM.speckleWindowSize;
	fsr["SGBM_disp12MaxDiff"] >> m_SGBM.disp12MaxDiff;
	fsr["SGBM_fullDP"] >> m_SGBM.fullDP;

	fsr["VAR_levels"] >> m_VAR.levels;
	fsr["VAR_pyrScale"] >> m_VAR.pyrScale;
	fsr["VAR_nIt"] >> m_VAR.nIt;
	fsr["VAR_minDisp"] >> m_VAR.minDisp;
	fsr["VAR_maxDisp"] >> m_VAR.maxDisp;
	fsr["VAR_poly_n"] >> m_VAR.poly_n;
	fsr["VAR_poly_sigma"] >> m_VAR.poly_sigma;
	fsr["VAR_fi"] >> m_VAR.fi;
	fsr["VAR_lambda"] >> m_VAR.lambda;
	fsr["VAR_penalization"] >> m_VAR.penalization;
	fsr["VAR_cycle"] >> m_VAR.cycle;
	fsr["VAR_flags"] >> m_VAR.flags;

	return 1;
}


// 保存参数到配置文件
void saveConfiguration()
{
	char filename[MAX_PATH];
	strcpy_s(filename, MAX_PATH, CurrExeDirectory);
	strcat_s(filename, MAX_PATH, "\\config_params.xml");
	FileStorage fsw(filename, FileStorage::WRITE);
	if (!fsw.isOpened())
		return;

	// 此两个参数在离线标定时并无外部输入的必要，而是根据图片确定。
	fsw << "useNum" << useNum;
	fsw << "imageSize" << "[" << imageSize.width << imageSize.height << "]";

	// 标定棋盘参数设定，需输入
	fsw << "m_nx" << m_nx;
	fsw << "m_ny" << m_ny;
	fsw << "squareWidth" << squareWidth;

	// 立体匹配相关参数

	fsw << "MatchAlgorithm" << MatchAlgorithm;

	fsw << "BM_minDisp" << m_BM.state->minDisparity;
	fsw << "BM_maxDisp" << m_BM.state->numberOfDisparities + m_BM.state->minDisparity;
	fsw << "BM_preFilterCap" << m_BM.state->preFilterCap;
	fsw << "BM_uniqRatio" << m_BM.state->uniquenessRatio;
	fsw << "BM_SADWinSize" << m_BM.state->SADWindowSize;
	fsw << "BM_specRange" << m_BM.state->speckleRange;
	fsw << "BM_specWinSize" << m_BM.state->speckleWindowSize;
	fsw << "BM_disp12MaxDiff" << m_BM.state->disp12MaxDiff;
	fsw << "BM_texThres" << m_BM.state->textureThreshold;

	fsw << "SGBM_minDisp" << m_SGBM.minDisparity;
	fsw << "SGBM_maxDisp" << m_SGBM.numberOfDisparities + m_SGBM.minDisparity;
	fsw << "SGBM_preFilterCap" << m_SGBM.preFilterCap;
	fsw << "SGBM_uniqRatio" << m_SGBM.uniquenessRatio;
	fsw << "SGBM_SADWinSize" << m_SGBM.SADWindowSize;
	fsw << "SGBM_specRange" << m_SGBM.speckleRange;
	fsw << "SGBM_specWinSize" << m_SGBM.speckleWindowSize;
	fsw << "SGBM_disp12MaxDiff" << m_SGBM.disp12MaxDiff;
	fsw << "SGBM_fullDP" << m_SGBM.fullDP;

	fsw << "VAR_levels" << m_VAR.levels;
	fsw << "VAR_pyrScale" << m_VAR.pyrScale;
	fsw << "VAR_nIt" << m_VAR.nIt;
	fsw << "VAR_minDisp" << m_VAR.minDisp;
	fsw << "VAR_maxDisp" << m_VAR.maxDisp;
	fsw << "VAR_poly_n" << m_VAR.poly_n;
	fsw << "VAR_poly_sigma" << m_VAR.poly_sigma;
	fsw << "VAR_fi" << m_VAR.fi;
	fsw << "VAR_lambda" << m_VAR.lambda;
	fsw << "VAR_penalization" << m_VAR.penalization;
	fsw << "VAR_cycle" << m_VAR.cycle;
	fsw << "VAR_flags" << m_VAR.flags;

	fsw.release();
}




// 保存左摄像机标定参数
void saveLeftCalibXml()
{
	char filename[MAX_PATH];
	strcpy_s(filename, MAX_PATH, CurrExeDirectory);
	strcat_s(filename, MAX_PATH, "\\calib_result_left.xml");
	FileStorage fs(filename, FileStorage::WRITE);
	if (fs.isOpened())
	{
		struct tm newtime;
		__time32_t aclock; 
		char timebuf[32];
		_time32(&aclock); // Get time in seconds.
		_localtime32_s(&newtime, &aclock);	// Convert time to struct tm form.
		asctime_s(timebuf, 32, &newtime);	// Print local time as a string.
		fs << "calibrationDate" << timebuf;

		fs << "used_boards_num"	<< useNum;
		fs << "imageSize" << "[" << imageSize.width << imageSize.height << "]";
		fs << "intrinsic_matrix1"	<< intrinsic_matrix1;
		fs << "distortion_coeffs1"	<< distortion_coeffs1;

		for (int i=0; i<useNum; i++)
		{
			char rmatName[50];
			sprintf_s(rmatName, "rotationMatrix_of_img%d", i);
			fs << rmatName << rotation_vectors1[i];
		}
		for (int j=0; j<useNum; j++)
		{
			char tmatName[50];
			sprintf_s(tmatName, "translationMatrix_of_img%d", j);
			fs << tmatName << translation_vectors1[j];
		}

		fs << "err1" << err1;

		fs.release();
	}

}

// 保存右摄像机标定参数
void saveRightCalibXml()
{
	char filename[MAX_PATH];
	strcpy_s(filename, MAX_PATH, CurrExeDirectory);
	strcat_s(filename, MAX_PATH, "\\calib_result_right.xml");
	FileStorage fs(filename, FileStorage::WRITE);
	if (fs.isOpened())
	{
		struct tm newtime;
		__time32_t aclock; 
		char timebuf[32];
		_time32(&aclock); // Get time in seconds.
		_localtime32_s(&newtime, &aclock);	// Convert time to struct tm form.
		asctime_s(timebuf, 32, &newtime);	// Print local time as a string.
		fs << "calibrationDate" << timebuf;

		fs << "used_boards_num"	<< useNum;
		fs << "imageSize" << "[" << imageSize.width << imageSize.height << "]";
		fs << "intrinsic_matrix2"	<< intrinsic_matrix2;
		fs << "distortion_coeffs2"	<< distortion_coeffs2;

		for (int i=0; i<useNum; i++)
		{
			char rmatName[50];
			sprintf_s(rmatName, "rotationMatrix_of_img%d", i);
			fs << rmatName << rotation_vectors2[i];
		}
		for (int j=0; j<useNum; j++)
		{
			char tmatName[50];
			sprintf_s(tmatName, "translationMatrix_of_img%d", j);
			fs << tmatName << translation_vectors2[j];
		}

		fs << "err2" << err2;

		fs.release();
	}
}


// 保存双目标定参数
void saveStereoCalibXml()
{
	char filename[MAX_PATH];
	strcpy_s(filename, MAX_PATH, CurrExeDirectory);
	strcat_s(filename, MAX_PATH, "\\stereo_calib_params.xml");
	FileStorage fs(filename, FileStorage::WRITE);
	if (fs.isOpened())
	{
		struct tm newtime;
		__time32_t aclock; 
		char timebuf[32];
		_time32(&aclock); // Get time in seconds.
		_localtime32_s(&newtime, &aclock);	// Convert time to struct tm form.
		asctime_s(timebuf, 32, &newtime);	// Print local time as a string.
		fs << "calibrationDate" << timebuf;

		fs << "used_boards_num"	<< useNum;
		fs << "imageSize" << "[" << imageSize.width << imageSize.height << "]";

		fs << "intrinsic_matrix1"	<< intrinsic_matrix1;
		fs << "distortion_coeffs1"	<< distortion_coeffs1;
		fs << "intrinsic_matrix2"	<< intrinsic_matrix2;
		fs << "distortion_coeffs2"	<< distortion_coeffs2;

		fs << "rotation"	<< rotation;
		fs << "translation"	<< translation;
		fs << "essential"	<< essential;
		fs << "foundational"<< foundational;

		fs.release();
	}
}


// 保存双目校正参数
void saveRectifyXml()
{
	char filename[MAX_PATH];
	strcpy_s(filename, MAX_PATH, CurrExeDirectory);
	strcat_s(filename, MAX_PATH, "\\stereo_rectify_params.xml");
	FileStorage fs(filename, FileStorage::WRITE);
	if (fs.isOpened())
	{
		fs << "imageSize" << "[" << imageSize.width << imageSize.height << "]";

		fs << "leftValidArea" << "[:"
			<< rcRoi1.x << rcRoi1.y
			<< rcRoi1.width << rcRoi1.height << "]";
		fs << "rightValidArea" << "[:"
			<< rcRoi2.x << rcRoi2.y
			<< rcRoi2.width << rcRoi2.height << "]";

		fs << "QMatrix" << QMatrix;
		fs << "remapX1" << mX1;
		fs << "remapY1" << mY1;
		fs << "remapX2" << mX2;
		fs << "remapY2" << mY2;

		fs.release();
	}
}
