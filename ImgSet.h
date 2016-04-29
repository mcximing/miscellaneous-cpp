#ifndef _IMG_SET_H_  
#define _IMG_SET_H_  
#include <vector>  
#include <string>  
#pragma comment(lib, "shlwapi.lib")  
class CImgSet  
{  
public:  
    CImgSet (const std::string &strImgDirName) : m_strImgDirName(strImgDirName+"//"), m_nImgNumber(0){}  
    int GetTotalImageNumber()  
    {  
        return m_nImgNumber;  
    }  
    std::string GetImgName(int nIndex)  
    {  
        return m_szImgs.at(nIndex);  
    }  
    int LoadImgsFromDir()  
    {  
        return LoadImgsFromDir("");  
    }
	
private:  
    int LoadImgsFromDir(const std::string &strDirName);  
private:  
    typedef std::vector <std::string> IMG_SET;  
    IMG_SET m_szImgs;  
    int m_nImgNumber;  
    const std::string m_strImgDirName;  
};  
#endif  
  
