//成员函数实现在文件ImgSet.cpp中：  
#include "ImgSet.h"  
#include <windows.h>  
#include <Shlwapi.h>  
/** 
strSubDirName：子文件夹名 
*/  
int CImgSet::LoadImgsFromDir(const std::string &strSubDirName)  
{  
    WIN32_FIND_DATAA stFD = {0};  
    std::string strDirName;  
    if ("" == strSubDirName)  
    {  
        strDirName = m_strImgDirName;  
    }  
    else  
    {  
        strDirName = strSubDirName;  
    }  
    std::string strFindName = strDirName + "//*";  
    HANDLE hFile = FindFirstFileA(strFindName.c_str(), &stFD);  
    BOOL bExist = FindNextFileA(hFile, &stFD);  
      
    for (;bExist;)  
    {  
        std::string strTmpName = strDirName + stFD.cFileName;  
        if (strDirName + "." == strTmpName || strDirName + ".." == strTmpName)  
        {  
            bExist = FindNextFileA(hFile, &stFD);  
            continue;  
        }  
        if (PathIsDirectoryA(strTmpName.c_str()))  
        {  
            strTmpName += "//";  
            LoadImgsFromDir(strTmpName);  
            bExist = FindNextFileA(hFile, &stFD);  
            continue;  
        }  
		std::string strSubImg = strDirName + stFD.cFileName; // fprintf( stderr, strSubImg.c_str() );fprintf( stderr, "\n");
        /*m_szImgs.push_back(strSubImg); */ 
		m_szImgs.push_back(stFD.cFileName); 
        bExist = FindNextFileA(hFile, &stFD);  
    }  
    m_nImgNumber = m_szImgs.size();  
    return m_nImgNumber;  
}  
