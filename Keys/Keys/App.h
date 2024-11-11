
/** $VER: App.h (2024.09.07) P. Stuer **/

#pragma once

#include "framework.h"
#include "D2D1Timer.h"
#include "Timer.h"
#include <chrono>

class App
{
public:
    App();
    ~App();

    HRESULT Initialize();
    void    Run();                                    //运行消息循环

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void SaveWindowPosition(HWND hwnd);
    void LoadWindowPosition(HWND hwnd);

    HRESULT Render();

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceDependentResources();
    HRESULT CreateSwapChainBuffers(ID2D1DeviceContext* dc, IDXGISwapChain1* swapChain) noexcept;
    void    ResizeSwapChain(UINT width, UINT height) noexcept;
    void    DeleteDeviceDependentResources();
    void    CalculateFrameStats()const;
    HRESULT LoadResourceD2dBitmap(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, ID2D1Bitmap** ppBitmap);
    HRESULT LoadResourceD2dcolorBitmap(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, ID2D1Bitmap** ppBitmap);
    bool    EnableAlphaCompositing(HWND hWnd);
    bool    CheckFullscreen();
    void    InitTray(HWND hwnd);
    void    restricted_position(HWND hwnd, RECT rWindow);

protected:

    HWND                                 m_hwnd;

    D2D1Timer                            m_timer;

    Timer                                text_timer;   //显示文字用的

    ID2D1Factory1*                       m_pD2DFactory;

    ID2D1Device*                         m_pD2DDevice;

    ID3D11Device*                        m_pD3DDevice;

    IDWriteFactory*                      m_pDWriteFactory;

    IDXGIFactory2*                       m_pDxgiFactory;


    IDXGIDevice1*                        m_pDxgiDevice;

    IDXGISurface2*                       m_Surface;

    ID2D1Bitmap1*                        m_pD2DTargetBimtap;

    IDCompositionDevice*                 m_CompositionDevice;

    IDWriteTextFormat*                   m_pTextFormat;

    ID2D1DeviceContext*                  m_pD2DDeviceContext;
    IDXGISwapChain1*                     m_pSwapChain;

    IDCompositionTarget*                 m_CompositionTarget;
    IDCompositionVisual*                 m_CompositionVisual;

    ID2D1SolidColorBrush*                m_SolidBrush;
    ID2D1BitmapBrush*                    m_BackgroundBrush;

    ID2D1Bitmap*                         m_pBitmap[87] = {};

    ID2D1LinearGradientBrush*            plgBrush;
    ID2D1GradientStopCollection*         pgsCollection;

    const int key[86] = { 27, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 192, 49, 50, 51, 52, 53, 54,
        55, 56, 57, 48, 189, 187, 8, 9, 81, 87, 69, 82, 84, 89, 85, 73, 79, 80, 219, 221,
        220, 20, 65, 83, 68, 70, 71, 72, 74, 75, 76, 186, 222, 13, 160, 90, 88, 67, 86, 66,
        78, 77, 188, 190, 191, 161, 162, 91, 164, 32, 165, 93, 163, 44, 145, 19, 45, 36, 33, 46, 35, 34, 38, 37, 40, 39 };

    const WCHAR* ClassName = L"Compositing";
    const WCHAR* WindowTitle = L"Compositing";

    double                               m_pwidth       = 413, m_pheight = 149;             //窗口宽高
    INT                                  m_pScrWid      = GetSystemMetrics(SM_CXSCREEN); //屏幕宽
    INT                                  m_pScrHei      = GetSystemMetrics(SM_CYSCREEN); //屏幕高
    //double                               m_ScrWid      = GetSystemMetrics(SM_CXFULLSCREEN); //不包含任务栏屏幕宽,但是不准确
    //double                               m_ScrHei      = GetSystemMetrics(SM_CYFULLSCREEN); //不包含任务栏屏幕高
    //获取可用桌面大小
    RECT                                 m_prect;
    INT                                  m_pScrWidZ     = 0;
    INT                                  m_pScrHeiZ     = 0;

    //创建托盘
    NOTIFYICONDATA                       m_ptrayIcon;                                  //托盘属性  
    HMENU                                m_phMenu;                                     //托盘菜单
    int                                  m_pAlpha;                                     //时钟周期用到的淡入效果透明值
    bool                                 m_peffect     = FALSE;                        //特效开关
    bool                                 m_pDisplays_text = FALSE;                     //显示文本
    int                                  m_ptext;

public:

    IWICImagingFactory*                  m_pWICFactory;

};

