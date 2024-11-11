
/** $VER: framework.h (2024.08.01) **/

#pragma once
#include "resource.h"

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#define NOMINMAX
#include <windows.h>
#include <atlbase.h>
#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite.h>
//DComposition用到的头文件
#include <dcomp.h>
//wic用到的头文件
#include <wincodec.h>

#pragma comment(lib, "dxgi")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dcomp")
#pragma comment(lib, "windowscodecs")

#include <stdlib.h>
#include <strsafe.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>

#include <cmath>
#include <cassert>
//std用到的头文件
#include <thread>
#include <iostream>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <random>

//透明窗口需要的头文件
#include <versionhelpers.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi")

//音效用到的头文件
#include <map>
#include <Mmsystem.h>

#pragma comment(lib, "winmm.lib")
//线程用到的头文件
#include <mutex>
#include <condition_variable>
using namespace std;


//标题栏时间用到的头文件
#include <iomanip>
//窗口标题需要的头文件
#include <sstream>

#pragma hdrstop

/******************************************************************
需要用到的宏定义
******************************************************************/
//模板，，释放指针 ppT 并将其设置为等于 NULL。
template<class Interface>
inline void SafeRelease(Interface** pInterfaceToRelease)
{
    if (*pInterfaceToRelease != NULL)
    {
        (*pInterfaceToRelease)->Release();

        (*pInterfaceToRelease) = NULL;
    }
}

#ifndef THIS_HINSTANCE
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define THIS_HINSTANCE ((HINSTANCE) &__ImageBase)
#endif


inline int ToDPI(int pixels, UINT dpi)
{
    return (int) ::ceil((float)pixels * (float)dpi / USER_DEFAULT_SCREEN_DPI);
}


#define MYWM_NOTIFYICON 0x00301   //托盘菜单消息号
//托盘菜单
#define ID_SHOW       3001
#define ID_SOUND      3002
#define ID_EFFECT     3003
#define ID_TOP        3004
#define ID_PENETARTE  3005
#define ID_ABOUT      3006
#define ID_QUIT       3007


//定义数据字符0~9
#define   VK_0         0x30 
#define   VK_1         0x31 
#define   VK_2         0x32 
#define   VK_3         0x33 
#define   VK_4         0x34 
#define   VK_5         0x35 
#define   VK_6         0x36 
#define   VK_7         0x37 
#define   VK_8         0x38 
#define   VK_9         0x39

//定义数据字符A~Z
#define   VK_A	0x41 
#define   VK_B	0x42 
#define   VK_C	0x43 
#define   VK_D	0x44 
#define   VK_E	0x45 
#define   VK_F	0x46 
#define   VK_G	0x47 
#define   VK_H	0x48 
#define   VK_I	0x49 
#define   VK_J	0x4A 
#define   VK_K	0x4B 
#define   VK_L	0x4C 
#define   VK_M	0x4D 
#define   VK_N	0x4E 
#define   VK_O	0x4F 
#define   VK_P	0x50 
#define   VK_Q	0x51 
#define   VK_R	0x52 
#define   VK_S	0x53 
#define   VK_T	0x54 
#define   VK_U	0x55 
#define   VK_V	0x56 
#define   VK_W	0x57 
#define   VK_X	0x58 
#define   VK_Y	0x59 
#define   VK_Z	0x5A 






