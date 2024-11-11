
/** $VER: App.cpp (2024.09.07) P. Stuer **/

//#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)
#pragma warning(disable:4996)
#include "App.h"
#include "WateWaved2d.h"
#include "WateWavegdiplus.h"

//生成随机数
int randomNumber1;
int randomNumber2;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(0, 413);
std::uniform_int_distribution<> dis2(0, 149);
//线程
std::unordered_map<int, bool> keymap;
std::atomic<bool> thread_closed(false);
std::atomic<bool> thread_paused(true);
std::thread soundThread;

//-----------------------------------------------------------------
//声明函数
void                 keySoundThreadFunc();
INT_PTR CALLBACK     About(HWND, UINT, WPARAM, LPARAM);

//-----------------------------------------------------------------
// 初始化新实例
App::App() : m_hwnd()
{
    CreateDeviceIndependentResources();
    //获取可用桌面大小
    SystemParametersInfo(SPI_GETWORKAREA, 0, &m_prect, 0);
    m_pScrWidZ = m_prect.right - m_prect.left; //可用窗口宽高
    m_pScrHeiZ = m_prect.bottom - m_prect.top;
}

//-----------------------------------------------------------------
// 销毁此实例
App::~App()
{
    if (soundThread.joinable())
    {
        thread_closed = true;
        if (soundThread.joinable())
        {
            soundThread.join();
        }
    }
    SafeRelease(&m_pWICFactory);
    SafeRelease(&m_pD2DFactory);
    SafeRelease(&m_pDWriteFactory);

    DeleteDeviceDependentResources();
}

//-----------------------------------------------------------------
// 从应用程序资源文件加载图像。从资源文件加载D2D位图
//-----------------------------------------------------------------
HRESULT App::LoadResourceD2dBitmap(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, ID2D1Bitmap** ppBitmap)
{
    IWICBitmapDecoder* pDecoder = NULL;                 //公开表示解码器的方法。
    IWICBitmapFrameDecode* pSource = NULL;              //是帧级接口，用于访问实际图像位
    IWICStream* pStream = NULL;                         //表示用于引用映像和元数据内容的 WINDOWS 映像组件 (WIC) 流
    IWICFormatConverter* pConverter = NULL;             //表示一个 IWICBitmapSource(公开引用从中检索像素但无法写回的源的方法。) ，它将图像数据从一种像素格式转换为另一种像素格式，处理到索引格式的抖动和半调、调色板转换和 alpha 阈值。
    IWICBitmapScaler* pScaler = NULL;                   //表示使用重采样或筛选算法调整输入位图的大小版本。

    HRSRC imageResHandle = NULL;                        //资源句柄，图像处理程序
    HGLOBAL imageResDataHandle = NULL;                  //内存句柄,图像数据处理程序
    void* pImageFile = NULL;                            //无类型指针，图片文件
    DWORD imageFileSize = 0;                            //32位无符号整数
    UINT					PIC_WIDTH = m_pwidth;
    UINT                    PIC_HEIGHT = m_pheight;
    // 确定资源
    imageResHandle = FindResourceW(THIS_HINSTANCE, resourceName, resourceType);
    HRESULT hr = imageResHandle ? S_OK : E_FAIL;
    //加载资源
    if (SUCCEEDED(hr)) {
        imageResDataHandle = LoadResource(THIS_HINSTANCE, imageResHandle);
        hr = imageResDataHandle ? S_OK : E_FAIL;
    }
    //锁定资源并计算图像的大小。
    if (SUCCEEDED(hr)) {
        pImageFile = LockResource(imageResDataHandle);
        hr = pImageFile ? S_OK : E_FAIL;
    }
    // 计算尺寸
    if (SUCCEEDED(hr)) {
        imageFileSize = SizeofResource(THIS_HINSTANCE, imageResHandle);
        hr = imageFileSize ? S_OK : E_FAIL;
    }
    // 创建一个 WIC 流映射到内存
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateStream(&pStream);
    }
    // 使用内存指针和大小初始化流
    if (SUCCEEDED(hr)) {
        hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);
    }
    // 创建解码器
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnLoad, &pDecoder);
    }
    // 创建初始帧
    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }
    // 转换图像格式
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }
    // 初始化格式转换器
    if (SUCCEEDED(hr)) {
        hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
    }

    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
    }
    if (SUCCEEDED(hr)) {
        hr = pScaler->Initialize(pConverter, PIC_WIDTH, PIC_HEIGHT, WICBitmapInterpolationModeCubic);
    }
    // 从 WIC 位图创建 Direct2D 位图
    if (SUCCEEDED(hr)) {
        hr = pRenderTarget->CreateBitmapFromWicBitmap(pScaler, NULL, ppBitmap);
    }
    SafeRelease(&pDecoder);
    SafeRelease(&pSource);
    SafeRelease(&pStream);
    SafeRelease(&pConverter);
    SafeRelease(&pScaler);
    return hr;
}
// 从资源文件加载D2D位图,遍历像素改变像素值
//-----------------------------------------------------------------
HRESULT App::LoadResourceD2dcolorBitmap(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, ID2D1Bitmap** ppBitmap)
{
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pSource = NULL;
    IWICStream* pStream = NULL;
    IWICFormatConverter* pConverter = NULL;
    IWICBitmapScaler* pScaler = NULL;
    HRSRC                   imageResHandle = NULL;
    HGLOBAL                 imageResDataHandle = NULL;
    void* pImageFile = NULL;
    DWORD                   imageFileSize = 0;
    UINT					PIC_WIDTH = m_pwidth;
    UINT                    PIC_HEIGHT = m_pheight;
    IWICBitmap* pWIC = NULL;

    // 确定资源
    imageResHandle = FindResourceW(THIS_HINSTANCE, resourceName, resourceType);
    HRESULT hr = imageResHandle ? S_OK : E_FAIL;
    //加载资源
    if (SUCCEEDED(hr)) {
        imageResDataHandle = LoadResource(THIS_HINSTANCE, imageResHandle);
        hr = imageResDataHandle ? S_OK : E_FAIL;
    }
    //锁定资源并计算图像的大小。
    if (SUCCEEDED(hr)) {
        pImageFile = LockResource(imageResDataHandle);
        hr = pImageFile ? S_OK : E_FAIL;
    }
    // 计算尺寸
    if (SUCCEEDED(hr)) {
        imageFileSize = SizeofResource(THIS_HINSTANCE, imageResHandle);
        hr = imageFileSize ? S_OK : E_FAIL;
    }

    // 创建一个 WIC 流映射到内存
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateStream(&pStream);
    }
    // 使用内存指针和大小初始化流
    if (SUCCEEDED(hr)) {
        hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);
    }
    // 创建解码器
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnLoad, &pDecoder);
    }
    // 创建初始帧
    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }
    /*这里屏蔽了，否则会改变宽高值
    if (SUCCEEDED(hr)) {
        hr = pSource->GetSize(&PIC_WIDTH, &PIC_HEIGHT);
    }
    */
    //---------------------------------------------增加位图缩放功能
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
    }
    if (SUCCEEDED(hr)) {
        hr = pScaler->Initialize(pSource, PIC_WIDTH, PIC_HEIGHT, WICBitmapInterpolationModeHighQualityCubic);
    }
    //---------------------------------------------把图像转为32位
    // 转换图像格式
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }
    // 初始化格式转换器
    if (SUCCEEDED(hr)) {
        hr = pConverter->Initialize(pScaler, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
    }
    //---------------------------------------------
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapFromSource(pConverter, WICBitmapCacheOnLoad, &pWIC);
    }
    if (SUCCEEDED(hr)) {        // 2.从 IWICBitmap 对象读取像素数据
        IWICBitmapLock* pILock = NULL;
        WICRect rcLock = { 0, 0, (INT)PIC_WIDTH, (INT)PIC_HEIGHT };
        hr = pWIC->Lock(&rcLock, WICBitmapLockWrite, &pILock);
        if (SUCCEEDED(hr)) {
            UINT cbBufferSize = 0;
            BYTE* pv = NULL;
            if (SUCCEEDED(hr)) {
                // 获取锁定矩形中左上角像素的指针
                hr = pILock->GetDataPointer(&cbBufferSize, &pv);
            }
            // 3.进行纹理混合的像素计算
            for (unsigned int i = 0; i < cbBufferSize; i += 4) {
                if (pv != NULL && i + 3 < cbBufferSize && pv[i + 3] != 0) {
                    pv[i + 2] = pv[i + 1];
                }
            }
            // 4.颜色混合操作结束，释放IWICBitmapLock对象
            pILock->Release();
        }
    }

    if (SUCCEEDED(hr)) {
        hr = pRenderTarget->CreateBitmapFromWicBitmap(pWIC, NULL, ppBitmap);
    }
    SafeRelease(&pDecoder);
    SafeRelease(&pSource);
    SafeRelease(&pStream);
    SafeRelease(&pConverter);
    SafeRelease(&pScaler);

    return hr;
}

//-----------------------------------------------------------------
// 开启Alpha通道混合
// https://github.com/glfw/glfw/blob/master/src/win32_window.c#L368C27-L368C27
bool App::EnableAlphaCompositing(HWND hWnd)
{
    if (!::IsWindowsVistaOrGreater()) { return false; }

    BOOL is_composition_enable = false;
    ::DwmIsCompositionEnabled(&is_composition_enable);
    if (!is_composition_enable) { return true; }

    DWORD current_color = 0;
    BOOL is_opaque = false;
    ::DwmGetColorizationColor(&current_color, &is_opaque);

    if (!is_opaque || IsWindows8OrGreater())
    {
        HRGN region = ::CreateRectRgn(0, 0, -1, -1);
        DWM_BLURBEHIND bb = { 0 };
        bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
        bb.hRgnBlur = region;
        bb.fEnable = true;
        ::DwmEnableBlurBehindWindow(hWnd, &bb);
        ::DeleteObject(region);
        return true;
    }
    else // For Window7
    {
        DWM_BLURBEHIND bb = { 0 };
        bb.dwFlags = DWM_BB_ENABLE;
        ::DwmEnableBlurBehindWindow(hWnd, &bb);
        return false;
    }
}

//-----------------------------------------------------------------
// 保存到注册表
void App::SaveWindowPosition(HWND hwnd)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);
    m_prect = rect;

    // 保存到注册表
    HKEY hKey;
    RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\MyApp", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueEx(hKey, L"WindowPosition", 0, REG_BINARY, (BYTE*)&rect, sizeof(rect));
    RegCloseKey(hKey);
}

//-----------------------------------------------------------------
// 从注册表读取
void App::LoadWindowPosition(HWND hwnd)
{
    // 从注册表读取
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\MyApp", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        RECT rect;
        DWORD dwType = REG_BINARY;
        DWORD dwSize = sizeof(rect);
        if (RegQueryValueEx(hKey, L"WindowPosition", NULL, &dwType, (BYTE*)&rect, &dwSize) == ERROR_SUCCESS)
        {
            m_prect = rect;
        }
        RegCloseKey(hKey);
    }

    SetWindowPos(hwnd, NULL, m_prect.left, m_prect.top,
        m_prect.right - m_prect.left, m_prect.bottom - m_prect.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
}

//-----------------------------------------------------------------
// 创建窗口
HRESULT App::Initialize()
{
    soundThread = std::thread(keySoundThreadFunc);
    const DWORD Style = WS_VISIBLE | WS_POPUP;
    const DWORD ExStyle = WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE | WS_EX_LAYERED; // 分层透明是为了淡入淡出。
    HRESULT hr = S_OK;
    if (SUCCEEDED(hr))
    {
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
        wcex.lpszClassName = ClassName;
        wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wcex.lpfnWndProc = App::WndProc;
        wcex.hInstance = THIS_HINSTANCE;
        //wcex.hbrBackground = (HBRUSH)CreateSolidBrush(0x000000);
        wcex.lpszMenuName = NULL;
        wcex.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(LONG_PTR);
        if (!RegisterClassEx(&wcex))
        {
            MessageBox(NULL, L"注册窗口类失败", L"错误", MB_OK | MB_ICONERROR);
            return 1;
        }

        INT          winx = m_pScrWidZ - (INT)m_pwidth;                               //窗口初始位置
        INT          winy = m_pScrHeiZ - (INT)m_pheight;
        m_hwnd = ::CreateWindowExW(ExStyle, ClassName, WindowTitle, Style, winx, winy, (INT)m_pwidth, (INT)m_pheight, NULL, NULL, THIS_HINSTANCE, this);
        hr = m_hwnd ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        LoadWindowPosition(m_hwnd);
        CreateDeviceDependentResources();
        EnableAlphaCompositing(m_hwnd);
        SetForegroundWindow(m_hwnd);
        SetLayeredWindowAttributes(m_hwnd, 0, 0, LWA_ALPHA);//启动时会先闪一下，应该是在消息循环里加入了渲染程序，把窗口初始透明度改成0，将就着算解决了
        SetTimer(m_hwnd, 1, 10, NULL);    //创建时钟
        ::ShowWindow(m_hwnd, SW_SHOWNORMAL);
        ::UpdateWindow(m_hwnd);
    }
    InitTray(m_hwnd);                       //实例化托盘  
    return hr;
}

//-----------------------------------------------------------------
// 创建不绑定到任何设备的资源。它们的生命周期实际上延长了应用程序的使用时间。这些资源包括 Direct2D、///
// DirectWrite 和 WIC 工厂，以及一个 DirectWrite Text Format 对象(用于识别特定的字体特征)和一个 Direct2D 几何图形。
HRESULT App::CreateDeviceIndependentResources()
{
#ifdef _DEBUG
    D2D1_FACTORY_OPTIONS const Options = { D2D1_DEBUG_LEVEL_INFORMATION };
    const DWORD DXGIFlags = DXGI_CREATE_FACTORY_DEBUG;
    const DWORD D3DFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
#else
    D2D1_FACTORY_OPTIONS const Options = { D2D1_DEBUG_LEVEL_NONE };
    const DWORD DXGIFlags = 0;
    const DWORD D3DFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif

    HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, Options, &m_pD2DFactory);
    
    if (SUCCEEDED(hr) && !m_pDxgiFactory) 
    {
        hr = ::CreateDXGIFactory2(DXGIFlags, __uuidof(m_pDxgiFactory), (void**)&m_pDxgiFactory);
    }

    if (SUCCEEDED(hr) && !m_pD3DDevice) 
    {
        hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3DFlags, nullptr, 0, D3D11_SDK_VERSION, &m_pD3DDevice, nullptr, nullptr);
    }

    if (SUCCEEDED(hr) && !m_pDWriteFactory) 
    {
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
    }

    if (SUCCEEDED(hr) && !m_pWICFactory) 
    {
        hr = ::CoInitialize(nullptr);
        hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pWICFactory));
    }

    if (SUCCEEDED(hr) && !m_pDxgiDevice) 
    {
        hr = m_pD3DDevice->QueryInterface(&m_pDxgiDevice);
    }

    if (SUCCEEDED(hr) && !m_pD2DDevice) 
    {
        hr = m_pD2DFactory->CreateDevice(m_pDxgiDevice, &m_pD2DDevice);
    }

                                        
    if (SUCCEEDED(hr) && !m_CompositionDevice) 
    {
        hr = ::DCompositionCreateDevice(m_pDxgiDevice, __uuidof(m_CompositionDevice), (void**)&m_CompositionDevice);
    }

    if (SUCCEEDED(hr) && !m_pTextFormat) 
    {
        static const WCHAR FontName[] = L"Segoe UI";
        static const FLOAT FontSize = 35.f;
        hr = m_pDWriteFactory->CreateTextFormat(FontName, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &m_pTextFormat);
    }

    if (SUCCEEDED(hr)) 
    {
        m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }

    return hr;
}

//-----------------------------------------------------------------
// 创建绑定到特定 Direct3D 设备的资源。它们都集中在这里，以防在 Direct3D 设备丢失的情况下需要重新创建资源//
// (例如，显示更改、远程控制、删除显卡等)。
HRESULT App::CreateDeviceDependentResources()
{
    RECT cr = { };

    ::GetClientRect(m_hwnd, &cr);

    UINT Width = (lround)(cr.right - cr.left);
    UINT Height = (lround)(cr.bottom - cr.top);

    HRESULT hr = (Width != 0) && (Height != 0) ? S_OK : DXGI_ERROR_INVALID_CALL;

    // 创建作为实际呈现目标并公开绘图命令的 Direct2D 设备上下文。
    if (SUCCEEDED(hr) && (m_pD2DDeviceContext == nullptr))
    {
        hr = m_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pD2DDeviceContext);

        // 根据目标窗口设置设备上下文的 DPI。
        if (SUCCEEDED(hr))
        {
            FLOAT DPI = (FLOAT) ::GetDpiForWindow(m_hwnd);

            m_pD2DDeviceContext->SetDpi(DPI, DPI);
        }
    }

    // 创建交换链。
    if (SUCCEEDED(hr) && (m_pSwapChain == nullptr))
    {
        DXGI_SWAP_CHAIN_DESC1 scd = {};

        //设置缓冲区的大小。DXGI 不知道交换链将用于什么，也不会查询窗口的客户区大小。
        scd.Width = Width;
        scd.Height = Height;
        scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;      // Best performance and compatibility
        scd.SampleDesc.Count = 1;                               // Number of multisamples per pixel (Multisampling disabled).
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.BufferCount = 2;
        scd.Scaling = DXGI_SCALING_STRETCH;
        scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;   // Flip Model
        scd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;   // Enable transparency

        hr = m_pDxgiFactory->CreateSwapChainForComposition(m_pDxgiDevice, &scd, nullptr, &m_pSwapChain);
    }
    // 检索交换链的后台缓冲区。
    if (SUCCEEDED(hr))
    {
        hr = m_pSwapChain->GetBuffer(0, __uuidof(m_Surface), (void**)&m_Surface);
    }
    if (SUCCEEDED(hr) && (m_pD2DTargetBimtap == nullptr)) {
        D2D1_BITMAP_PROPERTIES1 Properties = {};
        Properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        Properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        Properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        hr = m_pD2DDeviceContext->CreateBitmapFromDxgiSurface(m_Surface, Properties, &m_pD2DTargetBimtap);
        // 将表面位图设置为用于呈现的设备上下文的目标。
        if (SUCCEEDED(hr))
            m_pD2DDeviceContext->SetTarget(m_pD2DTargetBimtap);
    }
    // 创建组合目标。
    if (SUCCEEDED(hr) && (m_CompositionTarget == nullptr))
    {
        hr = m_CompositionDevice->CreateTargetForHwnd(m_hwnd, true, &m_CompositionTarget);
        if (SUCCEEDED(hr) && (m_CompositionVisual == nullptr))
            hr = m_CompositionDevice->CreateVisual(&m_CompositionVisual);
        if (SUCCEEDED(hr))
            hr = m_CompositionTarget->SetRoot(m_CompositionVisual);
    }
    if (SUCCEEDED(hr))
        hr = m_CompositionVisual->SetContent(m_pSwapChain);
    if (SUCCEEDED(hr))
        hr = m_CompositionDevice->Commit();

    /*
    if (SUCCEEDED(hr) && (m_BackgroundBrush == nullptr))
        hr = CreatePatternBrush(m_pD2DDeviceContext, &m_BackgroundBrush);

    if (SUCCEEDED(hr) && (m_SolidBrush == nullptr))
        hr = m_pD2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_SolidBrush);
       
    if (SUCCEEDED(hr) && (m_pBitmap[0] == nullptr))
        hr = LoadResourceD2dcolorBitmap(m_pD2DDeviceContext, m_pWICFactory, MAKEINTRESOURCE(2000), L"PNG", &m_pBitmap[0]);
   
    */

    for (int i = 0; i <= 86; i++)
    {
        if (SUCCEEDED(hr) && (m_pBitmap[i] == nullptr))
            hr = LoadResourceD2dBitmap(m_pD2DDeviceContext, m_pWICFactory, MAKEINTRESOURCE(2000 + i), L"PNG", &m_pBitmap[i]);
    }
    SafeRelease(&m_Surface);
    SafeRelease(&m_pD2DTargetBimtap);
    SafeRelease(&m_pDxgiDevice);
    SafeRelease(&m_pD2DDevice);
    SafeRelease(&m_pDxgiFactory);

    return hr;
}

//-----------------------------------------------------------------
// 创建交换链缓冲区。z
HRESULT App::CreateSwapChainBuffers(ID2D1DeviceContext* ID2D1DeviceContext, IDXGISwapChain1* swapChain) noexcept
{
    // 检索交换链的后台缓冲区。
    HRESULT hr = swapChain->GetBuffer(0, __uuidof(m_Surface), (void**)&m_Surface);

    // 创建指向交换链表面的 Direct2D 位图。
    if (SUCCEEDED(hr) && (m_pD2DTargetBimtap == nullptr))
    {
        D2D1_BITMAP_PROPERTIES1 Properties = {};

        Properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        Properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        Properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        hr = ID2D1DeviceContext->CreateBitmapFromDxgiSurface(m_Surface, Properties, &m_pD2DTargetBimtap);

        // 将表面位图设置为用于呈现的设备上下文的目标。
        if (SUCCEEDED(hr))
            ID2D1DeviceContext->SetTarget(m_pD2DTargetBimtap);
    }
    SafeRelease(&m_Surface);
    SafeRelease(&m_pD2DTargetBimtap);

    return hr;
}

//-----------------------------------------------------------------
// 调整交换链缓冲区的大小。
void App::ResizeSwapChain(UINT width, UINT height) noexcept
{
    // 在调整缓冲区大小之前释放对交换链的引用。
    m_pD2DDeviceContext->SetTarget(nullptr);

    HRESULT hr = (width != 0) && (height != 0) ? S_OK : DXGI_ERROR_INVALID_CALL;

    if (SUCCEEDED(hr))
        hr = m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    if (SUCCEEDED(hr))
        CreateSwapChainBuffers(m_pD2DDeviceContext, m_pSwapChain);
    else
        DeleteDeviceDependentResources();
}

/*
//-----------------------------------------------------------------
// 创建用于绘制背景的图案画笔。
HRESULT App::CreatePatternBrush(ID2D1RenderTarget* renderTarget, ID2D1BitmapBrush** bitmapBrush) const noexcept
{
    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(10.0f, 10.0f), &rt);

    CComPtr<ID2D1SolidColorBrush> GridBrush;

    if (SUCCEEDED(hr))
        hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF(0.93f, 0.94f, 0.96f, 1.0f)), &GridBrush);

    if (SUCCEEDED(hr))
    {
        rt->BeginDraw();

        rt->FillRectangle(D2D1::RectF(0.0f, 0.0f, 10.0f, 1.0f), GridBrush);
        rt->FillRectangle(D2D1::RectF(0.0f, 0.1f, 1.0f, 10.0f), GridBrush);

        hr = rt->EndDraw();
    }

    CComPtr<ID2D1Bitmap> GridBitmap;

    if (SUCCEEDED(hr))
        hr = rt->GetBitmap(&GridBitmap);

    if (SUCCEEDED(hr))
    {
        D2D1_BITMAP_BRUSH_PROPERTIES BrushProperties = D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP);

        hr = rt->CreateBitmapBrush(GridBitmap, BrushProperties, bitmapBrush);
    }

    return hr;
}
*/

//-----------------------------------------------------------------
// 丢弃在丢失 Direct3D 设备时需要重新创建的设备特定资源。
void App::DeleteDeviceDependentResources()
{
    SafeRelease(&m_pD2DDevice);
    SafeRelease(&m_CompositionDevice);
    SafeRelease(&m_pD3DDevice);
    for (int i = 0; i <= 86; i++) {
        SafeRelease(&m_pBitmap[i]);
    }

    SafeRelease(&plgBrush);
    SafeRelease(&pgsCollection);

    SafeRelease(&m_pSwapChain);
    SafeRelease(&m_pD2DDeviceContext);

    /*
    SafeRelease(&m_SolidBrush);
    SafeRelease(&m_BackgroundBrush);
    */
}


//-----------------------------------------------------------------
// 窗口过程
LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 不要修改TaskbarCreated，这是系统任务栏自定义的消息
    UINT WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));

    if (msg == WM_CREATE) {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        App* This = (App*)pcs->lpCreateParams;
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(This));
        return 1;
    }

    App* This = (App*)(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (This != nullptr) 
    {
        //变量
        static POINT    OldPos, NewPos;                        //鼠标光标的位置，用于取鼠标偏移量以移动窗口，用于取鼠标偏移量以缩放窗口
        static RECT     rWindow;                               //窗口矩形，用于缩放窗口，用于移动窗口和显示标题栏
        static double   xOffset, yOffset;                      //鼠标的偏移量，用于缩放窗口
        double          whb = This->m_pwidth / This->m_pheight;                  //窗口的宽高比，用雨固定窗口比例

        switch (msg) 
        {
        case WM_SIZE:                                   //窗口大小被改变
        {
            if (This->m_pD2DDeviceContext == nullptr)
                return 0;

            This->ResizeSwapChain(LOWORD(lParam), HIWORD(lParam));

            return 0;
        }

        case WM_TIMER: {
            if (wParam == 1) {
                if (This->m_pAlpha > 250) {
                    KillTimer(hwnd, 1);  //释放时钟
                }
                else {
                    This->m_pAlpha = This->m_pAlpha + 15;
                }
                SetLayeredWindowAttributes(hwnd, 0, This->m_pAlpha, LWA_ALPHA);
            }
            if (wParam == 2) {
                if (This->m_pAlpha <= 10) {
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                }
                else {
                    SetLayeredWindowAttributes(hwnd, 0, This->m_pAlpha, LWA_ALPHA);
                }
                This->m_pAlpha = This->m_pAlpha - 15;
            }
            return 0;
        }
        case WM_LBUTTONDOWN: {
            SetCapture(hwnd);
            OldPos.x = LOWORD(lParam);
            OldPos.y = HIWORD(lParam);
            return 0;
        }
        case WM_RBUTTONDOWN: {
            SetCapture(hwnd);
            GetCursorPos(&OldPos);//都叫OldPos，但是意思不一样，这个是对于屏幕鼠标的位置
            return 0;
        }
        case WM_LBUTTONUP: {
            ReleaseCapture();
            This->restricted_position(hwnd, rWindow);
            return 0;
        }

        case WM_RBUTTONUP: {
            ReleaseCapture();
            This->restricted_position(hwnd, rWindow);
            return 0;
        }

        case WM_MOUSEMOVE: {
            int cx = LOWORD(lParam);//都叫LOWORD(lParam)，但是意思不一样，这是移动后的鼠标实时位置
            int cy = HIWORD(lParam);
            if (This->m_peffect) {

                waterd2d.putStone(cx, cy, 3, 256);
            }

            GetWindowRect(hwnd, &rWindow);
            if (GetCapture() == hwnd && wParam == MK_LBUTTON) {
                SetWindowPos(hwnd, NULL, rWindow.left + (short)LOWORD(lParam) - OldPos.x, rWindow.top + (short)HIWORD(lParam) - OldPos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            if (GetCapture() == hwnd && wParam == MK_RBUTTON && !This->m_peffect) {
                if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION) {
                    return 0;
                }
                GetCursorPos(&NewPos);
                xOffset = NewPos.x - OldPos.x;
                yOffset = NewPos.y - OldPos.y;

                if (xOffset * xOffset > yOffset * yOffset) {
                    MoveWindow(hwnd, rWindow.left, rWindow.top, (rWindow.right + xOffset - rWindow.left), (rWindow.right + xOffset - rWindow.left) / whb, false);
                }
                else {
                    MoveWindow(hwnd, rWindow.left, rWindow.top, (rWindow.bottom + yOffset - rWindow.top) * whb, rWindow.bottom + yOffset - rWindow.top, false);
                }
                OldPos.x = NewPos.x;
                OldPos.y = NewPos.y;

            }
            return 0;
        }
        case WM_WINDOWPOSCHANGING: {
            WINDOWPOS* pWinPos = (WINDOWPOS*)lParam;
            if (This->CheckFullscreen()) {
                if (pWinPos->x > This->m_pScrWid - (rWindow.right - rWindow.left))//右
                    pWinPos->x = This->m_pScrWid - (rWindow.right - rWindow.left);

                if (pWinPos->y > This->m_pScrHei - (rWindow.bottom - rWindow.top))//下
                    pWinPos->y = This->m_pScrHei - (rWindow.bottom - rWindow.top);

                if (pWinPos->x < 0)     //左
                    pWinPos->x = 0;

                if (pWinPos->y < 0)     //上
                    pWinPos->y = 0;
            }
            int smallx = 13, smally = 5, bigx = 16107, bigy = 5811;
            if (pWinPos->cx < smallx)  //限制最小尺寸
                pWinPos->cx = smallx;
            if (pWinPos->cy < smally)
                pWinPos->cy = smally;

            if (pWinPos->cx > bigx)  //限制最大尺寸
                pWinPos->cx = bigx;
            if (pWinPos->cy > bigy)
                pWinPos->cy = bigy;
            return 0;
        }
        case WM_LBUTTONDBLCLK: {
            randomNumber1 = dis(gen);
            randomNumber2 = dis2(gen);
            if (GetMenuState(This->m_phMenu, ID_TOP, MF_BYCOMMAND) & MF_CHECKED) {
                CheckMenuItem(This->m_phMenu, ID_TOP, MF_UNCHECKED | MF_BYCOMMAND);
                if (This->CheckFullscreen())
                {
                    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) ^ WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);
                }
                SetForegroundWindow(hwnd);//窗口置焦点之后取消置顶
                SetWindowPos(hwnd, HWND_NOTOPMOST, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
                This->text_timer.reset();      //重置计时
                This->text_timer.start();		//开始计时 
                This->m_pDisplays_text = TRUE;
                This->m_ptext = 6;
            }
            else {
                CheckMenuItem(This->m_phMenu, ID_TOP, MF_CHECKED | MF_BYCOMMAND);
                SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_NOACTIVATE ^ WS_EX_TOOLWINDOW);
                SetWindowPos(hwnd, HWND_TOPMOST, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
                This->text_timer.reset();      //重置计时
                This->text_timer.start();		//开始计时 
                This->m_pDisplays_text = TRUE;
                This->m_ptext = 5;
            }
            return 0;
        }
                             //双击右键窗口穿透
        case WM_RBUTTONDBLCLK: {
            randomNumber1 = dis(gen);
            randomNumber2 = dis2(gen);
            CheckMenuItem(This->m_phMenu, ID_PENETARTE, MF_CHECKED | MF_BYCOMMAND);
            SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
            This->text_timer.reset();      //重置计时
            This->text_timer.start();		//开始计时 
            This->m_pDisplays_text = TRUE;
            This->m_ptext = 7;
            return 0;
        }

                             //托盘菜单消息
        case MYWM_NOTIFYICON: {
            if (lParam == WM_RBUTTONDOWN) {
                //获取鼠标坐标
                POINT pt; GetCursorPos(&pt);
                //右击后点别地可以清除“右击出来的菜单”
                SetForegroundWindow(hwnd);
                //弹出菜单,并把用户所选菜单项的标识符返回
                int cmd = TrackPopupMenu(This->m_phMenu, TPM_RETURNCMD, pt.x, pt.y, NULL, hwnd, NULL);

                if (cmd == ID_SHOW) {
                    if (IsWindowVisible(hwnd)) {
                        ShowWindowAsync(hwnd, false);
                    }
                    else {
                        ShowWindowAsync(hwnd, true);
                    }
                }
                if (cmd == ID_SOUND) {
                    randomNumber1 = dis(gen);
                    randomNumber2 = dis2(gen);
                    This->text_timer.reset();      //重置计时
                    This->text_timer.start();		//开始计时 
                    This->m_pDisplays_text = TRUE;
                    if (GetMenuState(This->m_phMenu, ID_SOUND, MF_BYCOMMAND) & MF_CHECKED) {
                        CheckMenuItem(This->m_phMenu, ID_SOUND, MF_UNCHECKED | MF_BYCOMMAND);
                        thread_paused = true;
                        This->m_ptext = 2;
                    }
                    else {
                        CheckMenuRadioItem(This->m_phMenu, ID_SOUND, ID_SOUND, ID_SOUND, MF_BYCOMMAND);
                        thread_paused = false;
                        This->m_ptext = 1;
                    }
                }
                if (cmd == ID_EFFECT) {
                    randomNumber1 = dis(gen);
                    randomNumber2 = dis2(gen);
                    This->text_timer.reset();      //重置计时
                    This->text_timer.start();		//开始计时 
                    This->m_pDisplays_text = TRUE;
                    if (GetMenuState(This->m_phMenu, ID_EFFECT, MF_BYCOMMAND) & MF_CHECKED) {
                        CheckMenuItem(This->m_phMenu, ID_EFFECT, MF_UNCHECKED | MF_BYCOMMAND);
                        This->m_peffect = FALSE;
                        This->m_ptext = 4;
                    }
                    else {
                        if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION) {
                            GetClientRect(hwnd, &rWindow);
                            ClientToScreen(hwnd, (POINT*)&rWindow);
                            ClientToScreen(hwnd, (POINT*)&rWindow.right);
                            SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) ^ WS_CAPTION);
                            SetWindowPos(hwnd, NULL, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                            MoveWindow(hwnd, rWindow.left, rWindow.top, rWindow.right - rWindow.left, rWindow.bottom - rWindow.top, TRUE);
                        }
                        CheckMenuRadioItem(This->m_phMenu, ID_EFFECT, ID_EFFECT, ID_EFFECT, MF_BYCOMMAND);
                        This->m_peffect = TRUE;
                        This->m_ptext = 3;
                        SetWindowPos(hwnd, NULL, 0, 0, (INT)This->m_pwidth, (INT)This->m_pheight, SWP_NOMOVE | SWP_NOZORDER);//还原窗口大小
                    }
                }
                if (cmd == ID_TOP) {
                    randomNumber1 = dis(gen);
                    randomNumber2 = dis2(gen);
                    This->text_timer.reset();      //重置计时
                    This->text_timer.start();		//开始计时 
                    This->m_pDisplays_text = TRUE;
                    if (GetMenuState(This->m_phMenu, ID_TOP, MF_BYCOMMAND) & MF_CHECKED) {
                        CheckMenuItem(This->m_phMenu, ID_TOP, MF_UNCHECKED | MF_BYCOMMAND);
                        if (This->CheckFullscreen())
                            SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) ^ WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);
                        SetWindowPos(hwnd, HWND_NOTOPMOST, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
                        This->m_ptext = 6;
                    }
                    else {
                        CheckMenuItem(This->m_phMenu, ID_TOP, MF_CHECKED | MF_BYCOMMAND);
                        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_NOACTIVATE ^ WS_EX_TOOLWINDOW);
                        SetWindowPos(hwnd, HWND_TOPMOST, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
                        This->m_ptext = 5;
                    }
                }
                if (cmd == ID_PENETARTE) {
                    randomNumber1 = dis(gen);
                    randomNumber2 = dis2(gen);
                    This->text_timer.reset();      //重置计时
                    This->text_timer.start();		//开始计时 
                    This->m_pDisplays_text = TRUE;
                    if (GetMenuState(This->m_phMenu, ID_PENETARTE, MF_CHECKED) & MF_CHECKED) {
                        CheckMenuItem(This->m_phMenu, ID_PENETARTE, MF_UNCHECKED | MF_BYCOMMAND);
                        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & (~WS_EX_TRANSPARENT));
                        This->m_ptext = 8;
                    }
                    else {
                        CheckMenuItem(This->m_phMenu, ID_PENETARTE, MF_CHECKED | MF_BYCOMMAND);
                        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
                        This->m_ptext = 7;
                    }
                }
                if (cmd == ID_ABOUT) {
                    DialogBox(THIS_HINSTANCE, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
                    SetWindowPos(hwnd, NULL, 0, 0, (INT)This->m_pwidth, (INT)This->m_pheight, SWP_NOMOVE | SWP_NOZORDER);//还原窗口大小
                    if (!IsWindowVisible(hwnd)) {
                        ShowWindowAsync(hwnd, true);  //显示窗口
                    }
                }
                if (cmd == ID_QUIT) {
                    SetTimer(hwnd, 2, 10, NULL);
                }
            }
            if (lParam == WM_LBUTTONDBLCLK) {
                if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION) {
                    GetClientRect(hwnd, &rWindow);
                    ClientToScreen(hwnd, (POINT*)&rWindow);
                    ClientToScreen(hwnd, (POINT*)&rWindow.right);
                }
                else {
                    GetWindowRect(hwnd, &rWindow);
                    AdjustWindowRectEx(&rWindow, GetWindowLongPtr(hwnd, GWL_STYLE) ^ WS_CAPTION, FALSE, GetWindowLongPtr(hwnd, GWL_EXSTYLE));
                }
                SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) ^ WS_CAPTION);
                SetWindowPos(hwnd, NULL, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                MoveWindow(hwnd, rWindow.left, rWindow.top, rWindow.right - rWindow.left, rWindow.bottom - rWindow.top, TRUE);
            }

            if (lParam == WM_LBUTTONDOWN) {
                SetForegroundWindow(hwnd);        //窗口置焦点
            }
            return 0;
        }
         case WM_CLOSE:
         {
             This->SaveWindowPosition(hwnd);
             DestroyWindow(hwnd);
             return 0;
         }

        case WM_DESTROY: {
            KillTimer(hwnd, 2);                         //释放时钟
            DestroyMenu(This->m_phMenu);                         //清除菜单
            Shell_NotifyIcon(NIM_DELETE, &This->m_ptrayIcon);    //销毁托盘图标
            ::PostQuitMessage(0);                         //销毁窗口
            return 1;
        }
        default:
            /*
            * 防止当Explorer.exe 崩溃以后，程序在系统系统托盘中的图标就消失
            *
            * 原理：Explorer.exe 重新载入后会重建系统任务栏。当系统任务栏建立的时候会向系统内所有
            * 注册接收TaskbarCreated 消息的顶级窗口发送一条消息，我们只需要捕捉这个消息，并重建系
            * 统托盘的图标即可。
            */
            if (msg == WM_TASKBARCREATED)
            {
                //系统Explorer崩溃重启时，重新加载托盘  
                Shell_NotifyIcon(NIM_ADD, &This->m_ptrayIcon);
            }
            break;
        }
    }
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------
//运行消息循环
void App::Run()
{
    m_timer.Reset();
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))    //调度传入的非排队消息，检查线程消息队列中是否存在已发布的消息，并检索 (消息（如果存在任何) ）。PM_REMOVE.PeekMessage 处理后，将从队列中删除消息。
        {
            TranslateMessage(&msg);                   // 把按键消息传递给字符消息
            DispatchMessage(&msg);                    // 将消息分派给窗口程序
        }
        else
        {
            m_timer.Tick();
            CalculateFrameStats();
            Render();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

//float g_rotation = 0;//旋转
//-----------------------------------------------------------------
// 渲染一个帧。
HRESULT App::Render()
{
    HRESULT hr = S_OK;

    //g_rotation++;//旋转
    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_F RenderTargetSize = m_pD2DDeviceContext->GetSize();

        D2D1_RECT_F Rect = D2D1::RectF(0, 0, RenderTargetSize.width, RenderTargetSize.height);//D2D1::RectF(0, 0, bitmapW, bitmapH)

        m_pD2DDeviceContext->BeginDraw();
        m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pD2DDeviceContext->Clear();        // 清屏
        //旋转
        //m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Rotation(g_rotation, D2D1::Point2F(m_pD2DDeviceContext->GetSize().width / 2, m_pD2DDeviceContext->GetSize().height / 2)));

        int KeyCode, subscript;
        if (m_pBitmap) {
            if (m_peffect) {
                waterd2d.WaveRender(m_pD2DDeviceContext, waterd2d.w_pbitmap[0]);
                for (subscript = 0; subscript <= 85; subscript++) {
                    KeyCode = key[subscript];
                    if (GetAsyncKeyState(KeyCode) & 0x8000) {
                        waterd2d.WaveRender(m_pD2DDeviceContext, waterd2d.w_pbitmap[subscript + 1]);
                    }
                }
                waterd2d.calcNextFrameWave();
            }
            else {
                m_pD2DDeviceContext->DrawBitmap(m_pBitmap[0], Rect);
                for (subscript = 0; subscript <= 85; subscript++) {
                    KeyCode = key[subscript];
                    if (GetAsyncKeyState(KeyCode) & 0x8000) {
                        m_pD2DDeviceContext->DrawBitmap(m_pBitmap[subscript + 1], Rect);
                    }
                }
            }
        }

        /*
        // 吸引聚光灯。
        {
            const D2D1_POINT_2F Center = D2D1::Point2F(RenderTargetSize.width / 2.f, RenderTargetSize.height / 2.f);
            const FLOAT Radius = ((std::min)(RenderTargetSize.width, RenderTargetSize.height) / 2.f) - 8.f;
            const D2D1_ELLIPSE Ellipse = D2D1::Ellipse(Center, Radius, Radius);

            m_SolidBrush->SetColor(D2D1::ColorF(.75f, .75f, 1, .25f));
            m_pD2DDeviceContext->FillEllipse(Ellipse, m_SolidBrush);
        }
        */


        FLOAT elapsed_time = (FLOAT)text_timer.getElapsedTime();//计时器运行时间
        const WCHAR* FontName = L"";

        if (elapsed_time > 1000)
        {
            m_pDisplays_text = FALSE;
            text_timer.reset();      //重置计时
        }
        if (m_pDisplays_text) {
            //线性渐变画刷
            if (SUCCEEDED(hr) && (plgBrush == nullptr))
            {
                FLOAT Color_elapsed_time = 1 - (elapsed_time - 500) / 500;
                //创建渐变节点数组
                D2D1_GRADIENT_STOP gradientStops[4] = {};
                gradientStops[0].color = D2D1::ColorF(D2D1::ColorF(0.0f, 0.0f, 1.0f, Color_elapsed_time));
                gradientStops[0].position = 0.0f;
                gradientStops[1].color = D2D1::ColorF(D2D1::ColorF(1.0f, 0.0f, 0.0f, Color_elapsed_time));
                gradientStops[1].position = 0.3f;
                gradientStops[2].color = D2D1::ColorF(D2D1::ColorF(0.0f, 1.0f, 0.0f, Color_elapsed_time));
                gradientStops[2].position = 0.6f;
                gradientStops[3].color = D2D1::ColorF(D2D1::ColorF(0.0f, 0.0f, 1.0f, Color_elapsed_time));
                gradientStops[3].position = 0.9f;
                //创建渐变条（这一部分可以想象成ps中的那个渐变）
                m_pD2DDeviceContext->CreateGradientStopCollection(gradientStops, 4, D2D1_GAMMA_1_0, D2D1_EXTEND_MODE_WRAP, &pgsCollection);
                //创建渐变刷
                if (pgsCollection != 0)
                {
                    m_pD2DDeviceContext->CreateLinearGradientBrush(
                        D2D1::LinearGradientBrushProperties(
                            D2D1::Point2F(randomNumber1, randomNumber2),         //渐变线起始点（窗口坐标）
                            D2D1::Point2F(80 + randomNumber1, 45 + randomNumber2)),
                        pgsCollection, &plgBrush);
                }
            }
            switch (m_ptext)
            {
            case 1:
                FontName = L" 音效";
                break;
            case 2:
                FontName = L" 音效关";
                break;
            case 3:
                FontName = L" 特效";
                break;
            case 4:
                FontName = L" 特效关";
                break;
            case 5:
                FontName = L" 置顶";
                break;
            case 6:
                FontName = L" 置顶关";
                break;
            case 7:
                FontName = L" 穿透";
                break;
            case 8:
                FontName = L" 穿透关";
                break;
            }
            if(plgBrush)
                m_pD2DDeviceContext->DrawTextW(FontName, (UINT) ::wcslen(FontName), m_pTextFormat, Rect, plgBrush);
            SafeRelease(&plgBrush);
            SafeRelease(&pgsCollection);
        }
        m_pD2DDeviceContext->EndDraw();
        // 将内存设备上下文的内容绘制到窗口的设备上下文
        hr = m_pSwapChain->Present(0, 0);
        if (!SUCCEEDED(hr) && (hr != DXGI_STATUS_OCCLUDED))
            DeleteDeviceDependentResources();
    }
    return hr;
}

//-----------------------------------------------------------------
// 计算每秒平均帧数的代码，还计算了绘制一帧的平均时间
/*
void App::CalculateFrameStats()const
{
    static int frameCount = 0;
    static double prevTime = GetTickCount64() / 1000.0;

    double currentTime = GetTickCount64() / 1000.0;
    double elapsedTime = currentTime - prevTime;

    unsigned int hour = 0, minute = 0, second = 0;

    UINT tim = m_timer.getElapsedTime() / 1000;
    second = (tim % 60);
    minute = (tim / 60) % 60;
    hour = (tim / 60) / 60;

    if (elapsedTime >= 1.0f)
    {
        double fps = frameCount / elapsedTime;
        float mspf = 1000.0f / fps;

        std::ostringstream outs;
        outs.precision(6);

        outs << "Time: " << std::setw(2) << std::setfill('0') << hour << ":" << std::setw(2) << std::setfill('0') << minute << ":" << std::setw(2) << std::setfill('0')
            << second << " | " <<
            "FPS: " << fps << " | " << mspf << " (ms)";
        SetWindowTextA(m_hwnd, outs.str().c_str());

        frameCount = 0;
        prevTime = currentTime;
    }
    else
    {
        frameCount++;
    }
}
*/
//这是使用d2d2timer头文件计算的帧数
void App::CalculateFrameStats()const
{
    // 计算每秒平均帧数的代码，还计算了绘制一帧的平均时间
    // 这些统计信息会显示在窗口标题栏中
    static int frameCnt = 0;
    static float timeElapsed = 0.0f;
    unsigned int hour = 0, minute = 0, second = 0;

    frameCnt++;
    UINT tim = m_timer.TotalTime();
    second = (tim % 60);
    minute = (tim / 60) % 60;
    hour = (tim / 60) / 60;

    // 计算一秒时间内的平均值
    if ((tim - timeElapsed) >= 1.0f)
    {
        float fps = (float)frameCnt; // fps = frameCnt / 1
        float mspf = 1000.0f / fps;

        std::ostringstream outs;
        outs.precision(6);

        outs << "Time: " << setw(2) << setfill('0') << hour << ":" << setw(2) << setfill('0') << minute << ":" << setw(2) << setfill('0')
            << second << " | " <<
            "FPS: " << fps << " | " << mspf << " (ms)";
        SetWindowTextA(m_hwnd, outs.str().c_str());

        // 为了计算下一个平均值重置一些值
        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}

//-----------------------------------------------------------------
//创建托盘
void App::InitTray(HWND hwnd)
{
    memset(&m_ptrayIcon, 0, sizeof(NOTIFYICONDATA));
    m_ptrayIcon.cbSize = sizeof(NOTIFYICONDATA);          //结构大小
    m_ptrayIcon.hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_KEYS)); //定义图标，IDI_SMALL-图标资源
    m_ptrayIcon.hWnd = hwnd;                            //关联窗口的句柄
    m_ptrayIcon.uCallbackMessage = MYWM_NOTIFYICON;                 //托盘菜单消息
    m_ptrayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;//标志
    m_ptrayIcon.uID = IDR_MAINFRAME;                   //标识符
    lstrcpy(m_ptrayIcon.szTip, L"");                                //提示信息
    //生成托盘菜单
    m_phMenu = CreatePopupMenu();
    //添加菜单,关键在于设置的一个标识符
    AppendMenu(m_phMenu, MF_STRING, ID_SHOW, _T("显示 / 隐藏"));
    AppendMenu(m_phMenu, MF_STRING, ID_SOUND, _T("音效"));
    AppendMenu(m_phMenu, MF_STRING, ID_EFFECT, _T("特效"));
    AppendMenu(m_phMenu, MF_SEPARATOR, 0, NULL);         //分割条
    AppendMenu(m_phMenu, MF_STRING, ID_TOP, _T("置顶"));
    AppendMenu(m_phMenu, MF_STRING, ID_PENETARTE, _T("穿透"));
    AppendMenu(m_phMenu, MF_SEPARATOR, 0, NULL);         //分割条
    AppendMenu(m_phMenu, MF_STRING, ID_ABOUT, _T("关于..."));
    AppendMenu(m_phMenu, MF_STRING, ID_QUIT, _T("退出"));

    Shell_NotifyIcon(NIM_ADD, &m_ptrayIcon);//发送消息
}

//-----------------------------------------------------------------
// 窗口限制
void App::restricted_position(HWND hwnd, RECT rWindow)
{
    if (CheckFullscreen()) return;

    bool hasCaption = (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION) != 0;

    int cx = 0;  // 水平方向的调整值
    int cy = 0;  // 垂直方向的调整值

    if ((rWindow.bottom > m_pScrHeiZ) || (rWindow.left < 0) || (rWindow.top < 0) || (rWindow.right > m_pScrWidZ))
    {
        if (hasCaption)
        {
            cx = (rWindow.left < 0) ? -rWindow.left - 8 : (rWindow.right > m_pScrWidZ) ? m_pScrWidZ - rWindow.right + 8 : 0;
            cy = (rWindow.top < 0) ? -rWindow.top : (rWindow.bottom > m_pScrHeiZ) ? m_pScrHeiZ - rWindow.bottom + 8 : 0;
        }
        else
        {
            cx = (rWindow.right - rWindow.left > m_pScrWidZ) ?
                -rWindow.left : (rWindow.left < 0) ?
                -rWindow.left : (rWindow.right > m_pScrWidZ) ?
                m_pScrWidZ - rWindow.right : 0;

            cy = (rWindow.bottom - rWindow.top > m_pScrHeiZ) ?
                -rWindow.top : (rWindow.top < 0) ?
                -rWindow.top : (rWindow.bottom > m_pScrHeiZ) ?
                m_pScrHeiZ - rWindow.bottom : 0;
        }
        SetWindowPos(hwnd, NULL, rWindow.left + cx, rWindow.top + cy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

//-----------------------------------------------------------------
//前台窗口是否全屏
bool App::CheckFullscreen()
{
    bool bFullScreen = false;
    HWND hWnd = GetForegroundWindow();
    RECT rcApp, rcDesk;
    GetWindowRect(GetDesktopWindow(), &rcDesk);
    if (hWnd != GetDesktopWindow() && hWnd != GetShellWindow()) {
        GetWindowRect(hWnd, &rcApp);
        if (rcApp.left <= rcDesk.left
            && rcApp.top <= rcDesk.top
            && rcApp.right >= rcDesk.right
            && rcApp.bottom >= rcDesk.bottom)
        {
            bFullScreen = true;
        }
    }
    return bFullScreen;
}

/*
//-----------------------------------------------------------------
//鼠标是否在窗口内
bool IsMouseInWindow(HWND hwnd)
{
    POINT mousePos;
    GetCursorPos(&mousePos);

    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    return mousePos.x >= windowRect.left && mousePos.x < windowRect.right &&
        mousePos.y >= windowRect.top && mousePos.y < windowRect.bottom;
}
*/

//-----------------------------------------------------------------
// 按键音效线程
// https://github.com/Git1Sunny/TapSound
void keySoundThreadFunc()
{
    for (int i = 8; i < 254; i++)
    {
        keymap[i] = false;
    }
    while (!thread_closed)
    {
        if (thread_paused)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        for (auto it = keymap.begin(); it != keymap.end(); it++)
        {
            if (GetAsyncKeyState(it->first) && !it->second)
            {
                keymap[it->first] = true;
                switch (it->first)
                {
                case VK_C:
                    if (GetAsyncKeyState(VK_LCONTROL))
                    {
                        PlaySound(MAKEINTRESOURCE(IDR_WAVE3), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                        break;
                    }
                case VK_V:
                    if (GetAsyncKeyState(VK_LCONTROL))
                    {
                        PlaySound(MAKEINTRESOURCE(IDR_WAVE4), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                        break;
                    }
                case VK_A:
                case VK_B:
                case VK_D:
                case VK_E:
                case VK_F:
                case VK_G:
                case VK_H:
                case VK_I:
                case VK_J:
                case VK_K:
                case VK_L:
                case VK_M:
                case VK_N:
                case VK_O:
                case VK_P:
                case VK_Q:
                case VK_R:
                case VK_S:
                case VK_T:
                case VK_U:
                case VK_W:
                case VK_X:
                case VK_Y:
                case VK_Z:
                case VK_LEFT:
                case VK_UP:
                case VK_RIGHT:
                case VK_DOWN:
                    PlaySound(MAKEINTRESOURCE(IDR_WAVE2), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                    break;
                case VK_F1:
                case VK_F2:
                case VK_F3:
                case VK_F4:
                case VK_F5:
                case VK_F6:
                case VK_F7:
                case VK_F8:
                case VK_F9:
                case VK_F10:
                case VK_F11:
                case VK_F12:
                    PlaySound(MAKEINTRESOURCE(IDR_WAVE3), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                    break;
                case VK_RETURN:
                    PlaySound(MAKEINTRESOURCE(IDR_WAVE4), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                    break;
                case VK_SPACE:
                    PlaySound(MAKEINTRESOURCE(IDR_WAVE5), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                    break;
                default:
                    PlaySound(MAKEINTRESOURCE(IDR_WAVE1), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                    break;
                }
            }
            if (!GetAsyncKeyState(it->first) && it->second)
            {
                keymap[it->first] = false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}



//-----------------------------------------------------------------
// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}