
/** $VER: App.cpp (2024.09.07) P. Stuer **/

//#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)
#pragma warning(disable:4996)
#include "App.h"
#include "WateWaved2d.h"
#include "WateWavegdiplus.h"

//���������
int randomNumber1;
int randomNumber2;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(0, 413);
std::uniform_int_distribution<> dis2(0, 149);
//�߳�
std::unordered_map<int, bool> keymap;
std::atomic<bool> thread_closed(false);
std::atomic<bool> thread_paused(true);
std::thread soundThread;

//-----------------------------------------------------------------
//��������
void                 keySoundThreadFunc();
INT_PTR CALLBACK     About(HWND, UINT, WPARAM, LPARAM);

//-----------------------------------------------------------------
// ��ʼ����ʵ��
App::App() : m_hwnd()
{
    CreateDeviceIndependentResources();
    //��ȡ���������С
    SystemParametersInfo(SPI_GETWORKAREA, 0, &m_prect, 0);
    m_pScrWidZ = m_prect.right - m_prect.left; //���ô��ڿ��
    m_pScrHeiZ = m_prect.bottom - m_prect.top;
}

//-----------------------------------------------------------------
// ���ٴ�ʵ��
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
// ��Ӧ�ó�����Դ�ļ�����ͼ�񡣴���Դ�ļ�����D2Dλͼ
//-----------------------------------------------------------------
HRESULT App::LoadResourceD2dBitmap(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, ID2D1Bitmap** ppBitmap)
{
    IWICBitmapDecoder* pDecoder = NULL;                 //������ʾ�������ķ�����
    IWICBitmapFrameDecode* pSource = NULL;              //��֡���ӿڣ����ڷ���ʵ��ͼ��λ
    IWICStream* pStream = NULL;                         //��ʾ��������ӳ���Ԫ�������ݵ� WINDOWS ӳ����� (WIC) ��
    IWICFormatConverter* pConverter = NULL;             //��ʾһ�� IWICBitmapSource(�������ô��м������ص��޷�д�ص�Դ�ķ�����) ������ͼ�����ݴ�һ�����ظ�ʽת��Ϊ��һ�����ظ�ʽ������������ʽ�Ķ����Ͱ������ɫ��ת���� alpha ��ֵ��
    IWICBitmapScaler* pScaler = NULL;                   //��ʾʹ���ز�����ɸѡ�㷨��������λͼ�Ĵ�С�汾��

    HRSRC imageResHandle = NULL;                        //��Դ�����ͼ�������
    HGLOBAL imageResDataHandle = NULL;                  //�ڴ���,ͼ�����ݴ������
    void* pImageFile = NULL;                            //������ָ�룬ͼƬ�ļ�
    DWORD imageFileSize = 0;                            //32λ�޷�������
    UINT					PIC_WIDTH = m_pwidth;
    UINT                    PIC_HEIGHT = m_pheight;
    // ȷ����Դ
    imageResHandle = FindResourceW(THIS_HINSTANCE, resourceName, resourceType);
    HRESULT hr = imageResHandle ? S_OK : E_FAIL;
    //������Դ
    if (SUCCEEDED(hr)) {
        imageResDataHandle = LoadResource(THIS_HINSTANCE, imageResHandle);
        hr = imageResDataHandle ? S_OK : E_FAIL;
    }
    //������Դ������ͼ��Ĵ�С��
    if (SUCCEEDED(hr)) {
        pImageFile = LockResource(imageResDataHandle);
        hr = pImageFile ? S_OK : E_FAIL;
    }
    // ����ߴ�
    if (SUCCEEDED(hr)) {
        imageFileSize = SizeofResource(THIS_HINSTANCE, imageResHandle);
        hr = imageFileSize ? S_OK : E_FAIL;
    }
    // ����һ�� WIC ��ӳ�䵽�ڴ�
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateStream(&pStream);
    }
    // ʹ���ڴ�ָ��ʹ�С��ʼ����
    if (SUCCEEDED(hr)) {
        hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);
    }
    // ����������
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnLoad, &pDecoder);
    }
    // ������ʼ֡
    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }
    // ת��ͼ���ʽ
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }
    // ��ʼ����ʽת����
    if (SUCCEEDED(hr)) {
        hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
    }

    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
    }
    if (SUCCEEDED(hr)) {
        hr = pScaler->Initialize(pConverter, PIC_WIDTH, PIC_HEIGHT, WICBitmapInterpolationModeCubic);
    }
    // �� WIC λͼ���� Direct2D λͼ
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
// ����Դ�ļ�����D2Dλͼ,�������ظı�����ֵ
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

    // ȷ����Դ
    imageResHandle = FindResourceW(THIS_HINSTANCE, resourceName, resourceType);
    HRESULT hr = imageResHandle ? S_OK : E_FAIL;
    //������Դ
    if (SUCCEEDED(hr)) {
        imageResDataHandle = LoadResource(THIS_HINSTANCE, imageResHandle);
        hr = imageResDataHandle ? S_OK : E_FAIL;
    }
    //������Դ������ͼ��Ĵ�С��
    if (SUCCEEDED(hr)) {
        pImageFile = LockResource(imageResDataHandle);
        hr = pImageFile ? S_OK : E_FAIL;
    }
    // ����ߴ�
    if (SUCCEEDED(hr)) {
        imageFileSize = SizeofResource(THIS_HINSTANCE, imageResHandle);
        hr = imageFileSize ? S_OK : E_FAIL;
    }

    // ����һ�� WIC ��ӳ�䵽�ڴ�
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateStream(&pStream);
    }
    // ʹ���ڴ�ָ��ʹ�С��ʼ����
    if (SUCCEEDED(hr)) {
        hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);
    }
    // ����������
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnLoad, &pDecoder);
    }
    // ������ʼ֡
    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }
    /*���������ˣ������ı���ֵ
    if (SUCCEEDED(hr)) {
        hr = pSource->GetSize(&PIC_WIDTH, &PIC_HEIGHT);
    }
    */
    //---------------------------------------------����λͼ���Ź���
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
    }
    if (SUCCEEDED(hr)) {
        hr = pScaler->Initialize(pSource, PIC_WIDTH, PIC_HEIGHT, WICBitmapInterpolationModeHighQualityCubic);
    }
    //---------------------------------------------��ͼ��תΪ32λ
    // ת��ͼ���ʽ
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }
    // ��ʼ����ʽת����
    if (SUCCEEDED(hr)) {
        hr = pConverter->Initialize(pScaler, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
    }
    //---------------------------------------------
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapFromSource(pConverter, WICBitmapCacheOnLoad, &pWIC);
    }
    if (SUCCEEDED(hr)) {        // 2.�� IWICBitmap �����ȡ��������
        IWICBitmapLock* pILock = NULL;
        WICRect rcLock = { 0, 0, (INT)PIC_WIDTH, (INT)PIC_HEIGHT };
        hr = pWIC->Lock(&rcLock, WICBitmapLockWrite, &pILock);
        if (SUCCEEDED(hr)) {
            UINT cbBufferSize = 0;
            BYTE* pv = NULL;
            if (SUCCEEDED(hr)) {
                // ��ȡ�������������Ͻ����ص�ָ��
                hr = pILock->GetDataPointer(&cbBufferSize, &pv);
            }
            // 3.���������ϵ����ؼ���
            for (unsigned int i = 0; i < cbBufferSize; i += 4) {
                if (pv != NULL && i + 3 < cbBufferSize && pv[i + 3] != 0) {
                    pv[i + 2] = pv[i + 1];
                }
            }
            // 4.��ɫ��ϲ����������ͷ�IWICBitmapLock����
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
// ����Alphaͨ�����
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
// ���浽ע���
void App::SaveWindowPosition(HWND hwnd)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);
    m_prect = rect;

    // ���浽ע���
    HKEY hKey;
    RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\MyApp", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueEx(hKey, L"WindowPosition", 0, REG_BINARY, (BYTE*)&rect, sizeof(rect));
    RegCloseKey(hKey);
}

//-----------------------------------------------------------------
// ��ע����ȡ
void App::LoadWindowPosition(HWND hwnd)
{
    // ��ע����ȡ
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
// ��������
HRESULT App::Initialize()
{
    soundThread = std::thread(keySoundThreadFunc);
    const DWORD Style = WS_VISIBLE | WS_POPUP;
    const DWORD ExStyle = WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE | WS_EX_LAYERED; // �ֲ�͸����Ϊ�˵��뵭����
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
            MessageBox(NULL, L"ע�ᴰ����ʧ��", L"����", MB_OK | MB_ICONERROR);
            return 1;
        }

        INT          winx = m_pScrWidZ - (INT)m_pwidth;                               //���ڳ�ʼλ��
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
        SetLayeredWindowAttributes(m_hwnd, 0, 0, LWA_ALPHA);//����ʱ������һ�£�Ӧ��������Ϣѭ�����������Ⱦ���򣬰Ѵ��ڳ�ʼ͸���ȸĳ�0��������������
        SetTimer(m_hwnd, 1, 10, NULL);    //����ʱ��
        ::ShowWindow(m_hwnd, SW_SHOWNORMAL);
        ::UpdateWindow(m_hwnd);
    }
    InitTray(m_hwnd);                       //ʵ��������  
    return hr;
}

//-----------------------------------------------------------------
// �������󶨵��κ��豸����Դ�����ǵ���������ʵ�����ӳ���Ӧ�ó����ʹ��ʱ�䡣��Щ��Դ���� Direct2D��///
// DirectWrite �� WIC �������Լ�һ�� DirectWrite Text Format ����(����ʶ���ض�����������)��һ�� Direct2D ����ͼ�Ρ�
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
// �����󶨵��ض� Direct3D �豸����Դ�����Ƕ�����������Է��� Direct3D �豸��ʧ���������Ҫ���´�����Դ//
// (���磬��ʾ���ġ�Զ�̿��ơ�ɾ���Կ���)��
HRESULT App::CreateDeviceDependentResources()
{
    RECT cr = { };

    ::GetClientRect(m_hwnd, &cr);

    UINT Width = (lround)(cr.right - cr.left);
    UINT Height = (lround)(cr.bottom - cr.top);

    HRESULT hr = (Width != 0) && (Height != 0) ? S_OK : DXGI_ERROR_INVALID_CALL;

    // ������Ϊʵ�ʳ���Ŀ�겢������ͼ����� Direct2D �豸�����ġ�
    if (SUCCEEDED(hr) && (m_pD2DDeviceContext == nullptr))
    {
        hr = m_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pD2DDeviceContext);

        // ����Ŀ�괰�������豸�����ĵ� DPI��
        if (SUCCEEDED(hr))
        {
            FLOAT DPI = (FLOAT) ::GetDpiForWindow(m_hwnd);

            m_pD2DDeviceContext->SetDpi(DPI, DPI);
        }
    }

    // ������������
    if (SUCCEEDED(hr) && (m_pSwapChain == nullptr))
    {
        DXGI_SWAP_CHAIN_DESC1 scd = {};

        //���û������Ĵ�С��DXGI ��֪��������������ʲô��Ҳ�����ѯ���ڵĿͻ�����С��
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
    // �����������ĺ�̨��������
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
        // ������λͼ����Ϊ���ڳ��ֵ��豸�����ĵ�Ŀ�ꡣ
        if (SUCCEEDED(hr))
            m_pD2DDeviceContext->SetTarget(m_pD2DTargetBimtap);
    }
    // �������Ŀ�ꡣ
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
// ������������������z
HRESULT App::CreateSwapChainBuffers(ID2D1DeviceContext* ID2D1DeviceContext, IDXGISwapChain1* swapChain) noexcept
{
    // �����������ĺ�̨��������
    HRESULT hr = swapChain->GetBuffer(0, __uuidof(m_Surface), (void**)&m_Surface);

    // ����ָ�򽻻�������� Direct2D λͼ��
    if (SUCCEEDED(hr) && (m_pD2DTargetBimtap == nullptr))
    {
        D2D1_BITMAP_PROPERTIES1 Properties = {};

        Properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        Properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        Properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        hr = ID2D1DeviceContext->CreateBitmapFromDxgiSurface(m_Surface, Properties, &m_pD2DTargetBimtap);

        // ������λͼ����Ϊ���ڳ��ֵ��豸�����ĵ�Ŀ�ꡣ
        if (SUCCEEDED(hr))
            ID2D1DeviceContext->SetTarget(m_pD2DTargetBimtap);
    }
    SafeRelease(&m_Surface);
    SafeRelease(&m_pD2DTargetBimtap);

    return hr;
}

//-----------------------------------------------------------------
// �����������������Ĵ�С��
void App::ResizeSwapChain(UINT width, UINT height) noexcept
{
    // �ڵ�����������С֮ǰ�ͷŶԽ����������á�
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
// �������ڻ��Ʊ�����ͼ�����ʡ�
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
// �����ڶ�ʧ Direct3D �豸ʱ��Ҫ���´������豸�ض���Դ��
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
// ���ڹ���
LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // ��Ҫ�޸�TaskbarCreated������ϵͳ�������Զ������Ϣ
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
        //����
        static POINT    OldPos, NewPos;                        //������λ�ã�����ȡ���ƫ�������ƶ����ڣ�����ȡ���ƫ���������Ŵ���
        static RECT     rWindow;                               //���ھ��Σ��������Ŵ��ڣ������ƶ����ں���ʾ������
        static double   xOffset, yOffset;                      //����ƫ�������������Ŵ���
        double          whb = This->m_pwidth / This->m_pheight;                  //���ڵĿ�߱ȣ�����̶����ڱ���

        switch (msg) 
        {
        case WM_SIZE:                                   //���ڴ�С���ı�
        {
            if (This->m_pD2DDeviceContext == nullptr)
                return 0;

            This->ResizeSwapChain(LOWORD(lParam), HIWORD(lParam));

            return 0;
        }

        case WM_TIMER: {
            if (wParam == 1) {
                if (This->m_pAlpha > 250) {
                    KillTimer(hwnd, 1);  //�ͷ�ʱ��
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
            GetCursorPos(&OldPos);//����OldPos��������˼��һ��������Ƕ�����Ļ����λ��
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
            int cx = LOWORD(lParam);//����LOWORD(lParam)��������˼��һ���������ƶ�������ʵʱλ��
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
                if (pWinPos->x > This->m_pScrWid - (rWindow.right - rWindow.left))//��
                    pWinPos->x = This->m_pScrWid - (rWindow.right - rWindow.left);

                if (pWinPos->y > This->m_pScrHei - (rWindow.bottom - rWindow.top))//��
                    pWinPos->y = This->m_pScrHei - (rWindow.bottom - rWindow.top);

                if (pWinPos->x < 0)     //��
                    pWinPos->x = 0;

                if (pWinPos->y < 0)     //��
                    pWinPos->y = 0;
            }
            int smallx = 13, smally = 5, bigx = 16107, bigy = 5811;
            if (pWinPos->cx < smallx)  //������С�ߴ�
                pWinPos->cx = smallx;
            if (pWinPos->cy < smally)
                pWinPos->cy = smally;

            if (pWinPos->cx > bigx)  //�������ߴ�
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
                SetForegroundWindow(hwnd);//�����ý���֮��ȡ���ö�
                SetWindowPos(hwnd, HWND_NOTOPMOST, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
                This->text_timer.reset();      //���ü�ʱ
                This->text_timer.start();		//��ʼ��ʱ 
                This->m_pDisplays_text = TRUE;
                This->m_ptext = 6;
            }
            else {
                CheckMenuItem(This->m_phMenu, ID_TOP, MF_CHECKED | MF_BYCOMMAND);
                SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_NOACTIVATE ^ WS_EX_TOOLWINDOW);
                SetWindowPos(hwnd, HWND_TOPMOST, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
                This->text_timer.reset();      //���ü�ʱ
                This->text_timer.start();		//��ʼ��ʱ 
                This->m_pDisplays_text = TRUE;
                This->m_ptext = 5;
            }
            return 0;
        }
                             //˫���Ҽ����ڴ�͸
        case WM_RBUTTONDBLCLK: {
            randomNumber1 = dis(gen);
            randomNumber2 = dis2(gen);
            CheckMenuItem(This->m_phMenu, ID_PENETARTE, MF_CHECKED | MF_BYCOMMAND);
            SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
            This->text_timer.reset();      //���ü�ʱ
            This->text_timer.start();		//��ʼ��ʱ 
            This->m_pDisplays_text = TRUE;
            This->m_ptext = 7;
            return 0;
        }

                             //���̲˵���Ϣ
        case MYWM_NOTIFYICON: {
            if (lParam == WM_RBUTTONDOWN) {
                //��ȡ�������
                POINT pt; GetCursorPos(&pt);
                //�һ�����ؿ���������һ������Ĳ˵���
                SetForegroundWindow(hwnd);
                //�����˵�,�����û���ѡ�˵���ı�ʶ������
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
                    This->text_timer.reset();      //���ü�ʱ
                    This->text_timer.start();		//��ʼ��ʱ 
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
                    This->text_timer.reset();      //���ü�ʱ
                    This->text_timer.start();		//��ʼ��ʱ 
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
                        SetWindowPos(hwnd, NULL, 0, 0, (INT)This->m_pwidth, (INT)This->m_pheight, SWP_NOMOVE | SWP_NOZORDER);//��ԭ���ڴ�С
                    }
                }
                if (cmd == ID_TOP) {
                    randomNumber1 = dis(gen);
                    randomNumber2 = dis2(gen);
                    This->text_timer.reset();      //���ü�ʱ
                    This->text_timer.start();		//��ʼ��ʱ 
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
                    This->text_timer.reset();      //���ü�ʱ
                    This->text_timer.start();		//��ʼ��ʱ 
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
                    SetWindowPos(hwnd, NULL, 0, 0, (INT)This->m_pwidth, (INT)This->m_pheight, SWP_NOMOVE | SWP_NOZORDER);//��ԭ���ڴ�С
                    if (!IsWindowVisible(hwnd)) {
                        ShowWindowAsync(hwnd, true);  //��ʾ����
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
                SetForegroundWindow(hwnd);        //�����ý���
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
            KillTimer(hwnd, 2);                         //�ͷ�ʱ��
            DestroyMenu(This->m_phMenu);                         //����˵�
            Shell_NotifyIcon(NIM_DELETE, &This->m_ptrayIcon);    //��������ͼ��
            ::PostQuitMessage(0);                         //���ٴ���
            return 1;
        }
        default:
            /*
            * ��ֹ��Explorer.exe �����Ժ󣬳�����ϵͳϵͳ�����е�ͼ�����ʧ
            *
            * ԭ��Explorer.exe �����������ؽ�ϵͳ����������ϵͳ������������ʱ�����ϵͳ������
            * ע�����TaskbarCreated ��Ϣ�Ķ������ڷ���һ����Ϣ������ֻ��Ҫ��׽�����Ϣ�����ؽ�ϵ
            * ͳ���̵�ͼ�꼴�ɡ�
            */
            if (msg == WM_TASKBARCREATED)
            {
                //ϵͳExplorer��������ʱ�����¼�������  
                Shell_NotifyIcon(NIM_ADD, &This->m_ptrayIcon);
            }
            break;
        }
    }
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------
//������Ϣѭ��
void App::Run()
{
    m_timer.Reset();
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))    //���ȴ���ķ��Ŷ���Ϣ������߳���Ϣ�������Ƿ�����ѷ�������Ϣ�������� (��Ϣ����������κ�) ����PM_REMOVE.PeekMessage ����󣬽��Ӷ�����ɾ����Ϣ��
        {
            TranslateMessage(&msg);                   // �Ѱ�����Ϣ���ݸ��ַ���Ϣ
            DispatchMessage(&msg);                    // ����Ϣ���ɸ����ڳ���
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

//float g_rotation = 0;//��ת
//-----------------------------------------------------------------
// ��Ⱦһ��֡��
HRESULT App::Render()
{
    HRESULT hr = S_OK;

    //g_rotation++;//��ת
    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_F RenderTargetSize = m_pD2DDeviceContext->GetSize();

        D2D1_RECT_F Rect = D2D1::RectF(0, 0, RenderTargetSize.width, RenderTargetSize.height);//D2D1::RectF(0, 0, bitmapW, bitmapH)

        m_pD2DDeviceContext->BeginDraw();
        m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pD2DDeviceContext->Clear();        // ����
        //��ת
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
        // �����۹�ơ�
        {
            const D2D1_POINT_2F Center = D2D1::Point2F(RenderTargetSize.width / 2.f, RenderTargetSize.height / 2.f);
            const FLOAT Radius = ((std::min)(RenderTargetSize.width, RenderTargetSize.height) / 2.f) - 8.f;
            const D2D1_ELLIPSE Ellipse = D2D1::Ellipse(Center, Radius, Radius);

            m_SolidBrush->SetColor(D2D1::ColorF(.75f, .75f, 1, .25f));
            m_pD2DDeviceContext->FillEllipse(Ellipse, m_SolidBrush);
        }
        */


        FLOAT elapsed_time = (FLOAT)text_timer.getElapsedTime();//��ʱ������ʱ��
        const WCHAR* FontName = L"";

        if (elapsed_time > 1000)
        {
            m_pDisplays_text = FALSE;
            text_timer.reset();      //���ü�ʱ
        }
        if (m_pDisplays_text) {
            //���Խ��仭ˢ
            if (SUCCEEDED(hr) && (plgBrush == nullptr))
            {
                FLOAT Color_elapsed_time = 1 - (elapsed_time - 500) / 500;
                //��������ڵ�����
                D2D1_GRADIENT_STOP gradientStops[4] = {};
                gradientStops[0].color = D2D1::ColorF(D2D1::ColorF(0.0f, 0.0f, 1.0f, Color_elapsed_time));
                gradientStops[0].position = 0.0f;
                gradientStops[1].color = D2D1::ColorF(D2D1::ColorF(1.0f, 0.0f, 0.0f, Color_elapsed_time));
                gradientStops[1].position = 0.3f;
                gradientStops[2].color = D2D1::ColorF(D2D1::ColorF(0.0f, 1.0f, 0.0f, Color_elapsed_time));
                gradientStops[2].position = 0.6f;
                gradientStops[3].color = D2D1::ColorF(D2D1::ColorF(0.0f, 0.0f, 1.0f, Color_elapsed_time));
                gradientStops[3].position = 0.9f;
                //��������������һ���ֿ��������ps�е��Ǹ����䣩
                m_pD2DDeviceContext->CreateGradientStopCollection(gradientStops, 4, D2D1_GAMMA_1_0, D2D1_EXTEND_MODE_WRAP, &pgsCollection);
                //��������ˢ
                if (pgsCollection != 0)
                {
                    m_pD2DDeviceContext->CreateLinearGradientBrush(
                        D2D1::LinearGradientBrushProperties(
                            D2D1::Point2F(randomNumber1, randomNumber2),         //��������ʼ�㣨�������꣩
                            D2D1::Point2F(80 + randomNumber1, 45 + randomNumber2)),
                        pgsCollection, &plgBrush);
                }
            }
            switch (m_ptext)
            {
            case 1:
                FontName = L" ��Ч";
                break;
            case 2:
                FontName = L" ��Ч��";
                break;
            case 3:
                FontName = L" ��Ч";
                break;
            case 4:
                FontName = L" ��Ч��";
                break;
            case 5:
                FontName = L" �ö�";
                break;
            case 6:
                FontName = L" �ö���";
                break;
            case 7:
                FontName = L" ��͸";
                break;
            case 8:
                FontName = L" ��͸��";
                break;
            }
            if(plgBrush)
                m_pD2DDeviceContext->DrawTextW(FontName, (UINT) ::wcslen(FontName), m_pTextFormat, Rect, plgBrush);
            SafeRelease(&plgBrush);
            SafeRelease(&pgsCollection);
        }
        m_pD2DDeviceContext->EndDraw();
        // ���ڴ��豸�����ĵ����ݻ��Ƶ����ڵ��豸������
        hr = m_pSwapChain->Present(0, 0);
        if (!SUCCEEDED(hr) && (hr != DXGI_STATUS_OCCLUDED))
            DeleteDeviceDependentResources();
    }
    return hr;
}

//-----------------------------------------------------------------
// ����ÿ��ƽ��֡���Ĵ��룬�������˻���һ֡��ƽ��ʱ��
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
//����ʹ��d2d2timerͷ�ļ������֡��
void App::CalculateFrameStats()const
{
    // ����ÿ��ƽ��֡���Ĵ��룬�������˻���һ֡��ƽ��ʱ��
    // ��Щͳ����Ϣ����ʾ�ڴ��ڱ�������
    static int frameCnt = 0;
    static float timeElapsed = 0.0f;
    unsigned int hour = 0, minute = 0, second = 0;

    frameCnt++;
    UINT tim = m_timer.TotalTime();
    second = (tim % 60);
    minute = (tim / 60) % 60;
    hour = (tim / 60) / 60;

    // ����һ��ʱ���ڵ�ƽ��ֵ
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

        // Ϊ�˼�����һ��ƽ��ֵ����һЩֵ
        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}

//-----------------------------------------------------------------
//��������
void App::InitTray(HWND hwnd)
{
    memset(&m_ptrayIcon, 0, sizeof(NOTIFYICONDATA));
    m_ptrayIcon.cbSize = sizeof(NOTIFYICONDATA);          //�ṹ��С
    m_ptrayIcon.hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_KEYS)); //����ͼ�꣬IDI_SMALL-ͼ����Դ
    m_ptrayIcon.hWnd = hwnd;                            //�������ڵľ��
    m_ptrayIcon.uCallbackMessage = MYWM_NOTIFYICON;                 //���̲˵���Ϣ
    m_ptrayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;//��־
    m_ptrayIcon.uID = IDR_MAINFRAME;                   //��ʶ��
    lstrcpy(m_ptrayIcon.szTip, L"");                                //��ʾ��Ϣ
    //�������̲˵�
    m_phMenu = CreatePopupMenu();
    //��Ӳ˵�,�ؼ��������õ�һ����ʶ��
    AppendMenu(m_phMenu, MF_STRING, ID_SHOW, _T("��ʾ / ����"));
    AppendMenu(m_phMenu, MF_STRING, ID_SOUND, _T("��Ч"));
    AppendMenu(m_phMenu, MF_STRING, ID_EFFECT, _T("��Ч"));
    AppendMenu(m_phMenu, MF_SEPARATOR, 0, NULL);         //�ָ���
    AppendMenu(m_phMenu, MF_STRING, ID_TOP, _T("�ö�"));
    AppendMenu(m_phMenu, MF_STRING, ID_PENETARTE, _T("��͸"));
    AppendMenu(m_phMenu, MF_SEPARATOR, 0, NULL);         //�ָ���
    AppendMenu(m_phMenu, MF_STRING, ID_ABOUT, _T("����..."));
    AppendMenu(m_phMenu, MF_STRING, ID_QUIT, _T("�˳�"));

    Shell_NotifyIcon(NIM_ADD, &m_ptrayIcon);//������Ϣ
}

//-----------------------------------------------------------------
// ��������
void App::restricted_position(HWND hwnd, RECT rWindow)
{
    if (CheckFullscreen()) return;

    bool hasCaption = (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION) != 0;

    int cx = 0;  // ˮƽ����ĵ���ֵ
    int cy = 0;  // ��ֱ����ĵ���ֵ

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
//ǰ̨�����Ƿ�ȫ��
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
//����Ƿ��ڴ�����
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
// ������Ч�߳�
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
// �����ڡ������Ϣ�������
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