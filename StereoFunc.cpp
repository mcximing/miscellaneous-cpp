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
// ȫ�ֱ�������
/************************************************************************/

char CurrExeDirectory[MAX_PATH];// ��ǰ��������Ŀ¼
CString calibFilePath;			// �궨ͼ���ļ��б�·��

Mat currImg1;	// ��ͼ1��ǰͼ��
Mat currImg2;	// ��ͼ2��ǰͼ��
Mat currImg3;	// ��ͼ3��ǰͼ��

int m_nx;					// �����нǵ��� w
int m_ny;					// �����нǵ��� h
int boardCornerNum;			// ���̽ǵ�����= w*h
cv::Size patternSize;		// �������нǵ�=(w,h)
cv::Size imageSize;			// ͼ��ֱ���
float squareWidth;			// ���̸��ӱ߳�
int imgNum;					// ���ڱ궨��ͼ�����
int useNum;					// ��ЧͼƬ��Ŀ
vector<Point2f> corners;	// ��Žǵ����꣬ע��vectorһ��Ҫ��ʼ��
vector<vector<Point3f>>	objectPoints;	// ���̽ǵ�������������
vector<vector<Point2f>>	imagePoints1;	// ����ͼ�����̽ǵ�������������
vector<vector<Point2f>>	imagePoints2;	// ����ͼ�����̽ǵ�������������

// ���¼�����ʵ��ʹ�õı���
Mat intrinsic_matrix1;	// ����ͼ���ڲ�������3*3
Mat intrinsic_matrix2;	// ����ͼ���ڲ�������3*3
Mat distortion_coeffs1;	// ����ͼ�ľ������ϵ������1*4
Mat distortion_coeffs2;	// ����ͼ�ľ������ϵ������1*4
vector<Mat> rotation_vectors1;		// ����ͼ����ת����
vector<Mat> rotation_vectors2;		// ����ͼ����ת����
vector<Mat> translation_vectors1;	// ����ͼ��ƽ�ƾ���
vector<Mat> translation_vectors2;	// ����ͼ��ƽ�ƾ���
double err1;	// ������ͼ�궨���ֵ
double err2;
bool calib1;	// �궨ָʾ
bool calib2; 
bool calib;
bool rectify;

Mat rotation;		// ��ת����
Mat translation;	// ƽ�ƾ���
Mat essential;		// ��������
Mat foundational;	// ��������

Mat mX1;	// ����ͼ X ��������ӳ�����
Mat mY1;	// ����ͼ Y ��������ӳ�����
Mat mX2;	// ����ͼ X ��������ӳ�����
Mat	mY2;	// ����ͼ Y ��������ӳ�����
Mat	QMatrix;	// ���ڼ�����ά���Ƶ� Q ����
cv::Rect rcRoi1;// ����ͼ��Ч����ľ���
cv::Rect rcRoi2;// ����ͼ��Ч����ľ���

Mat img1r;		// У���������ͼ��
Mat img2r;

StereoBM m_BM;		// ����ƥ�� BM ����
StereoSGBM m_SGBM;	// ����ƥ�� SGBM ����
StereoVar m_VAR;	// ����ƥ�� VAR ������ʵ�飩

int MatchAlgorithm;	// ƥ���㷨ѡ�������0-BM��1-SGBM��2-VAR


/************************************************************************/
// ȫ�ֺ���ʵ��
/************************************************************************/

// ѡ���ļ���
CString GetFolderPath(HWND hParent)
{
	TCHAR szDir[MAX_PATH];//�����洢·�����ַ���
	CString strPath = _T("");
	BROWSEINFO bInfo;
	ZeroMemory(&bInfo, sizeof(bInfo));
	bInfo.hwndOwner = hParent;
	bInfo.pidlRoot = NULL;
	bInfo.pszDisplayName = szDir;
	bInfo.lpszTitle = _T("��ѡ��·��: ");
	bInfo.ulFlags = BIF_RETURNONLYFSDIRS;
	bInfo.lpfn = NULL;
	bInfo.lParam = 0;
	bInfo.iImage = 0;

	LPITEMIDLIST lpDlist; //�������淵����Ϣ��IDList
	lpDlist = SHBrowseForFolder(&bInfo) ; //��ʾѡ��Ի���
	if(lpDlist != NULL)   //�û�����ȷ����ť
	{
		SHGetPathFromIDList(lpDlist, szDir);//����Ŀ��ʶ�б�ת�����ַ���
		strPath = szDir; //��TCHAR���͵��ַ���ת��ΪCString���͵��ַ���
	}
	return strPath;
}


// ��Mat���͹���CImage��
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



// ������Ĭ�ϳ�ʼ��
void LoadDefaultParam()
{
	GetModuleFileNameA(NULL, CurrExeDirectory, MAX_PATH); //�õ�����ģ��.exeȫ·��
	strrchr( CurrExeDirectory, '\\')[1]= 0;	// �������\���ţ��ص���ߵ�.exe�ļ��������ǳ���Ŀ¼

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

	// ����ƥ����ز���
	// Ӱ������Ҫ������ SADWindowSize��numberOfDisparities �� uniquenessRatio ����
	//	m_BM.state->preFilterSize = 15;	//Ԥ�����˲������ڴ�С������Χ��[5,255]��һ��Ӧ���� 5x5..21x21 ֮�䣬��������Ϊ����ֵ, int ��
	m_BM.state->preFilterCap = 31;	//Ԥ�����˲����Ľض�ֵ��Ԥ�������ֵ������[-Cap, Cap]��Χ�ڵ�ֵ��������Χ��1-31, int
	m_BM.state->SADWindowSize = 33;	//SAD���ڴ�С������Χ��[5,255]��һ��Ӧ���� 5x5 �� 21x21 ֮�䣬����������������int ��
	m_BM.state->minDisparity = 0;	//��С�ӲĬ��ֵΪ 0, �����Ǹ�ֵ��int ��
	m_BM.state->numberOfDisparities = 64;	//�Ӳ�ڣ�������Ӳ�ֵ����С�Ӳ�ֵ֮��, ���ڴ�С������ 16 ����������int ��
	m_BM.state->textureThreshold = 10;	//������������ж���ֵ��
	m_BM.state->uniquenessRatio = 15;	//�Ӳ�Ψһ�԰ٷֱȣ��ò�������Ϊ��ֵ��һ��5-15���ҵ�ֵ�ȽϺ��ʣ�int ��
	m_BM.state->speckleWindowSize = 100;//����Ӳ���ͨ����仯�ȵĴ��ڴ�С, ֵΪ 0 ʱȡ�� speckle ��飬int ��
	m_BM.state->speckleRange = 32;		//�Ӳ�仯��ֵ�����������Ӳ�仯������ֵʱ���ô����ڵ��Ӳ����㣬int ��
	m_BM.state->disp12MaxDiff = -1;		//���Ӳ�ͼ��ֱ�Ӽ���ó��������Ӳ�ͼ��ͨ��cvValidateDisparity����ó���֮������������졣

	m_SGBM.preFilterCap = 63;	//Ԥ�����˲����Ľض�ֵ
	m_SGBM.SADWindowSize = 9;	//SAD���ڴ�С������Χ��[1,11]��һ��Ӧ���� 3x3 �� 11x11 ֮�䣬��������������
	m_SGBM.P1 = 8*m_SGBM.SADWindowSize*m_SGBM.SADWindowSize;	//�����Ӳ�仯ƽ���ԵĲ�����P1��P2��ֵԽ���Ӳ�Խƽ����
	m_SGBM.P2 = 32*m_SGBM.SADWindowSize*m_SGBM.SADWindowSize;	//P2�������P1
	m_SGBM.minDisparity = 0;	//��С�ӲĬ��ֵΪ 0, �����Ǹ�ֵ��int ��
	m_SGBM.numberOfDisparities = 64;//�Ӳ�ڣ�������Ӳ�ֵ����С�Ӳ�ֵ֮��, ���ڴ�С������ 16 ����������int ��
	m_SGBM.uniquenessRatio = 15;	//�Ӳ�Ψһ�԰ٷֱȣ��ò�������Ϊ��ֵ��һ��5-15���ҵ�ֵ�ȽϺ��ʣ�int ��	
	m_SGBM.speckleWindowSize = 100;	//����Ӳ���ͨ����仯�ȵĴ��ڴ�С, ֵΪ 0 ʱȡ�� speckle ��飬int ��
	m_SGBM.speckleRange = 32;		//�Ӳ�仯��ֵ�����������Ӳ�仯������ֵʱ���ô����ڵ��Ӳ����㣬int ��
	m_SGBM.disp12MaxDiff = -1;		//���Ӳ�ͼ��ֱ�Ӽ���ó��������Ӳ�ͼ��ͨ��cvValidateDisparity����ó���֮������������졣
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


// ���±궨��������������ͼ�������
void ResetParam()
{
	patternSize = cvSize(m_nx,m_ny);
	boardCornerNum = m_nx * m_ny;
	// ����vector����Ŀռ䣬����ʼ��Ҳ�ᱨ��
	corners.resize(boardCornerNum,Point2f(0,0));
	objectPoints.clear();	// Ҫclear����resize���ܽ������vector���·���
	imagePoints1.clear();	// ֱ��resizeֻ�����·������vector����Ŀ
	imagePoints2.clear();
	objectPoints.resize(useNum, vector<Point3f>(boardCornerNum, Point3f(0,0,0)));
	imagePoints1.resize(useNum, vector<Point2f>(boardCornerNum, Point2f(0,0)));
	imagePoints2.resize(useNum, vector<Point2f>(boardCornerNum, Point2f(0,0)));
}


// ���ͼ��ǵ�
int FindCorner(Mat &img)
{
	if (img.empty())
		return -1;

	Mat gray = Mat(img.rows,img.cols,CV_8UC1); // ����Ҷ�ͼ
	cvtColor(img,gray,CV_BGR2GRAY);

	// ���ĺ���
	bool patternFound = findChessboardCorners(gray, patternSize, corners,
		CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE
		+ CALIB_CB_FAST_CHECK);

	// ϸ��
	if(patternFound)
		cornerSubPix(gray, corners, Size(11, 11), Size(-1, -1), 
		TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.05));
	else
		return 0;

	// �����ҵ��Ľǵ�
	if(patternFound)
		drawChessboardCorners(img, patternSize, Mat(corners), patternFound);
	return 1;
}


// ��ȡ�ļ��б궨ͼ���������о�
void judgeCalibFiles(CString path, vector<bool>& ifGood, vector<string>& goodImg)
{
	Mat currImg;	// ��ǰͼ��
	CString pathWild = path + _T("\\*.jpg");	// ��Ѱ�ҵ��ļ�·��ȫ��
	string strName;	// �洢����·�����ļ���
	string temp;	// �洢·���������ļ���
	USES_CONVERSION;
	strName = T2A(path);
	strName = strName + "\\";
	temp = strName;
	char * pPath = T2A(pathWild);	// ·����

	struct _finddata_t c_file;		// �����ļ��ṹ
	long hFile;

	if( (hFile = _findfirst(pPath, &c_file)) == -1L )
	{
		fprintf(stderr, "�Ҳ���ͼ���ļ���\n");
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
			if (FindCorner(currImg)>0)// �ɹ��ҵ��ǵ�
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


// �б�궨ͼ�������Ƿ�������ڱ궨
int doJudgeForCalib(CString path) // ����������·��
{
	if (path==_T(""))	// ����·��Ϊ��
		return 0;
	
	ResetParam();

	CString pathL,pathR;		// Լ�����ļ����·�Ϊ�������ļ���
	pathL = path + _T("\\L");
	pathR = path + _T("\\R");

	calibFilePath = CurrExeDirectory;
	calibFilePath += _T("\\CalibImgName.txt"); // �����ļ�����TXT�ļ�
	ofstream fout(calibFilePath);

	vector<bool> ifGood1;
	vector<bool> ifGood2;
	vector<string> goodImg1;
	vector<string> goodImg2;

	judgeCalibFiles(pathL, ifGood1, goodImg1);
	judgeCalibFiles(pathR, ifGood2, goodImg2);

	imgNum = ifGood1.size();	// �������ȼ�ͼƬ��
	useNum = 0;
	if (imgNum!=ifGood2.size())	// ����������С��һ��
	{
		fprintf(stderr, "����������С��һ�£�\n");
		return -1;
	}
	if (imgNum==0)	// û�п��õ�ͼƬ
	{
		fprintf(stderr, "û�п��õ�ͼƬ��\n");
		return -2;
	}

	for (int i=0; i<imgNum; i++)	// ��TXT��д�����ڱ궨���ļ���
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
	//sprintf_s(msg,32,"�궨�����б��������ЧͼƬ%d��\n", useNum);
	//printf(msg);
	return useNum;
}



// �������̽ǵ����������
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


// ͨ�����ļ����е�Ŀ�궨
int doSingleCalibLR()
{
	if (calibFilePath==_T(""))	// �鲻���궨�ļ��б�
	{
		fprintf(stderr, "�鲻���궨�ļ��б�\n");
		return -1;
	}
	ResetParam();
	string fileName;
	ifstream fin(calibFilePath);

	int64 t = getTickCount();

	// ��ͼƬ��ȡ�ǵ㣬���������̽ǵ����������imagePoints
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
		fprintf(stderr, "��ȡ�궨�ļ�����\n");
		return -2;
	}

	// �������̽ǵ����������objectPoints
	getObjectPoints();

	// �궨�������
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

	// �궨�������
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
	return (int)(t/getTickFrequency()*1000);	// ��������ʱ�����ֵ

	//printf("���ҵ�Ŀ�궨����.\n");
}



// ˫Ŀ�궨���ܺ���
int doStereoCamCalib()
{
	//*******************ע�⣬��ʱû�д������Ҷ�ӦͼƬ����һ����Ŀ�궨ʧ�ܵ������
	if (!(calib1&&calib2))
	{
		fprintf(stderr, "���ҵ�Ŀ�궨δ��ɣ�\n");
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
	//sprintf_s(timeShow, 16, "˫Ŀ�궨��������ʱ: %0.2fs\n", t/getTickFrequency());
	//printf(timeShow);

	saveStereoCalibXml();
	
	return (int)(t/getTickFrequency()*1000);	// ��������ʱ�����ֵ
}



// ˫ĿУ�����ܺ���
int doStereoCamRectify()
{
	if (!calib)
	{
		if (loadStereoCalibData()==0) // û�б궨���Դ��ļ���ȡ
		{
			fprintf(stderr, "δ��⵽˫Ŀ�궨����\n");
			return 0;
		}
		calib = TRUE;
	}

	//��ʼ��
	mX1 = Mat(imageSize, CV_32FC1);
	mY1 = Mat(imageSize, CV_32FC1);
	mX2 = Mat(imageSize, CV_32FC1);
	mY2 = Mat(imageSize, CV_32FC1);

	Mat R1, R2, P1, P2, Q;
	Rect roi1, roi2;
	double alpha = -1;

	//ִ��˫ĿУ��
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

	//ʹ��HARTLEY�����Ķ��⴦��
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

	//����ͼ��У�����������ӳ�����
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

	//�������
	Q.copyTo(QMatrix);
	rcRoi1 = roi1;
	rcRoi2 = roi2;

	//printf("У������������.\n");
	saveRectifyXml();
	rectify = true;
	
	return 1;
}



// ��ͼ�����У��
int remapImage(Mat img1, Mat img2)
{
	if (!rectify)	// ���У������
	{
		if (loadRectifyData()==0) // û���������Դ��ļ���ȡ
		{
			fprintf(stderr, "�Ҳ���У�����ݣ�\n");
			return -1;
		}
		rectify = TRUE;
	}
	if (img1.empty() || img2.empty())	// û��ͼ����
	{
		fprintf(stderr, "����ͼ�����\n");
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


// ����ƥ��BM�㷨
void bmMatch()
{
	// ������ͼ����Ч��������һ����˫ĿУ���׶ε� cvStereoRectify �������ݣ�Ҳ���������趨��
	m_BM.state->roi1 = rcRoi1;
	m_BM.state->roi2 = rcRoi2;

	// ת��Ϊ�Ҷ�ͼ
	cv::Mat img1gray, img2gray;
	cvtColor(img1r, img1gray, CV_BGR2GRAY);
	cvtColor(img2r, img2gray, CV_BGR2GRAY);

	// ��������ͼ����߽��б߽����أ��Ի�ȡ��ԭʼ��ͼ��ͬ��С����Ч�Ӳ�����
	Mat img1border, img2border;
	copyMakeBorder(img1gray, img1border, 0, 0, m_BM.state->numberOfDisparities, 0, IPL_BORDER_REPLICATE);
	copyMakeBorder(img2gray, img2border, 0, 0, m_BM.state->numberOfDisparities, 0, IPL_BORDER_REPLICATE);

	// �����Ӳ�
	Mat dispBorder, disp;
	m_BM(img1border, img2border, dispBorder);

	// У�������Ч����������ģ
	Mat m_Calib_Mat_Mask_Roi = Mat::zeros(imageSize.height, imageSize.width, CV_8UC1);
	cv::rectangle(m_Calib_Mat_Mask_Roi, rcRoi1, Scalar(255), CV_FILLED);

	// ��ȡ��ԭʼ�����Ӧ���Ӳ�������ȥ�ӿ�Ĳ��֣�
	disp = dispBorder.colRange(m_BM.state->numberOfDisparities, dispBorder.cols);
	disp.copyTo(disp, m_Calib_Mat_Mask_Roi);
	disp.convertTo(currImg3, CV_8U, 255/(m_BM.state->numberOfDisparities*16.));

	//Mat disp;
	//m_BM(img1gray,img2gray,disp);
	//disp.convertTo(currImg3, CV_8U, 255/(m_BM.state->numberOfDisparities*16.));

//	ShowImage(currImg3,imgShow3,IDC_VISION3,0);
//	imwrite("disparity_bm.jpg",currImg3);
}


// ����ƥ��SGBM�㷨
void sgbmMatch()
{
	// ת��Ϊ�Ҷ�ͼ
	Mat img1gray, img2gray;
	cvtColor(img1r, img1gray, CV_BGR2GRAY);
	cvtColor(img2r, img2gray, CV_BGR2GRAY);

	// ��������ͼ����߽��б߽����أ��Ի�ȡ��ԭʼ��ͼ��ͬ��С����Ч�Ӳ�����
	Mat img1border, img2border;
	copyMakeBorder(img1gray, img1border, 0, 0, m_SGBM.numberOfDisparities, 0, IPL_BORDER_REPLICATE);
	copyMakeBorder(img2gray, img2border, 0, 0, m_SGBM.numberOfDisparities, 0, IPL_BORDER_REPLICATE);

	// �����Ӳ�
	Mat disp, dispBorder;
	m_SGBM(img1border, img2border, dispBorder);

	// У�������Ч����������ģ
	Mat m_Calib_Mat_Mask_Roi = Mat::zeros(imageSize.height, imageSize.width, CV_8UC1);
	rectangle(m_Calib_Mat_Mask_Roi, rcRoi1, Scalar(255), CV_FILLED);

	// ��ȡ��ԭʼ�����Ӧ���Ӳ�������ȥ�ӿ�Ĳ��֣�
	disp = dispBorder.colRange(m_SGBM.numberOfDisparities, img1border.cols);
	disp.copyTo(disp, m_Calib_Mat_Mask_Roi);
	disp.convertTo(currImg3, CV_8U, 255/(m_SGBM.numberOfDisparities*16.));

	//Mat disp, disp8;
	//m_SGBM(img1gray,img2gray,disp);
	//disp.convertTo(currImg3, CV_8U, 255/(m_SGBM.numberOfDisparities*16.));

//	ShowImage(currImg3,imgShow3,IDC_VISION3,0);
//	imwrite("disparity_sgbm.jpg",currImg3);
}


// ����ƥ��VAR�㷨
void varMatch()
{
	// ת��Ϊ�Ҷ�ͼ
	Mat img1gray, img2gray;
	cvtColor(img1r, img1gray, CV_BGR2GRAY);
	cvtColor(img2r, img2gray, CV_BGR2GRAY);

	Mat disp, disp8;
	m_VAR(img1gray, img2gray, disp); // ʵʩ����
	disp.convertTo(currImg3, CV_8U, 1/*255/(64*16.)*/);	// �任ֵ��

//	ShowImage(currImg3,imgShow3,IDC_VISION3,0);
//	imwrite("disparity_var.jpg",currImg3);
}






// ����˫Ŀ�궨�����ͽǵ������˫ĿУ��
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



// ����˫ĿУ��������У��ͼ�����ƥ��
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



// �������ļ�����������
int loadConfiguration()
{
	LoadDefaultParam();	// ������ȫ��Ĭ�ϲ����������ٸ���������Ĳ��ָ���

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

	// �����������������߱궨ʱ�����ⲿ����ı�Ҫ�����Ǹ���ͼƬȷ����
	fsr["useNum"] >> useNum;
	FileNodeIterator it = fsr["imageSize"].begin();
	it >> imageSize.width >> imageSize.height;

	fsr["m_nx"] >> m_nx;
	fsr["m_ny"] >> m_ny;
	boardCornerNum = m_nx * m_ny;	// ��ֵΪ�������
	patternSize = Size(m_nx, m_ny);	// ��ֵΪ�������
	fsr["squareWidth"] >> squareWidth;

	// ����ƥ����ز���

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


// ��������������ļ�
void saveConfiguration()
{
	char filename[MAX_PATH];
	strcpy_s(filename, MAX_PATH, CurrExeDirectory);
	strcat_s(filename, MAX_PATH, "\\config_params.xml");
	FileStorage fsw(filename, FileStorage::WRITE);
	if (!fsw.isOpened())
		return;

	// ���������������߱궨ʱ�����ⲿ����ı�Ҫ�����Ǹ���ͼƬȷ����
	fsw << "useNum" << useNum;
	fsw << "imageSize" << "[" << imageSize.width << imageSize.height << "]";

	// �궨���̲����趨��������
	fsw << "m_nx" << m_nx;
	fsw << "m_ny" << m_ny;
	fsw << "squareWidth" << squareWidth;

	// ����ƥ����ز���

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




// ������������궨����
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

// ������������궨����
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


// ����˫Ŀ�궨����
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


// ����˫ĿУ������
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
