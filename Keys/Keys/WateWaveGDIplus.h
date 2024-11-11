/***********************************************************************
功能：GDI+水波纹效果
作者：林豪横
***********************************************************************/
#include "framework.h"
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
/**/
class WaterWaveEffectgdiplus
{
public:
    WaterWaveEffectgdiplus();
    ~WaterWaveEffectgdiplus();

    // 功能：根据水波波幅渲染水波位图数据(转为d2d位图渲染)(数据类型是DWORD）
    void WaveRender(ID2D1RenderTarget* pRenderTarget, Bitmap* originalBitmap);
    // 功能：根据水波波幅渲染水波位图数据(转为d2d位图渲染)(数据类型是BYTE）
    void WaveRender2(ID2D1RenderTarget* pRenderTarget, Bitmap* originalBitmap);
    // 功能：根据水波波幅渲染水波位图数据(转为d2d位图渲染)(数据类型原始指针是BYTE，修改指针是DWORD）
    void WaveRender3(ID2D1RenderTarget* pRenderTarget, Bitmap* originalBitmap);
    // 功能：使用gdi+位图渲染（数据类型是BYTE）（可以使用WaveRender1的前半部分数据类型DWORD）
    void WaveRenderGdiBYTE(Graphics* graphics, Bitmap* originalBitmap);
    // 功能：水波扩散
    void calcNextFrameWave();
    // 功能：扔石子（设定波源）
    void putStone(int x, int y, int stoneSize, int stoneWeight);


private:
    // 功能：初始化
    void init();
    // 功能：释放水波对象资源
    void FreeRipple();
    void CreategdiplusBitmap();

    HRESULT LoadResourcegdiplusBitmap(PCWSTR resourceName, PCWSTR resourceType, Bitmap** ppBitmap);
    HRESULT LoadResourcegdipluscolorBitmap(PCWSTR resourceName, PCWSTR resourceType, Bitmap** ppBitmap);
    std::vector<BYTE*> bufferPool;  // 缓冲区池

public:
    Bitmap* w_pbitmap[87];

private:

    ULONG_PTR gdiplusToken;  // 用于存储 GDI+ 启动令牌
    std::vector<std::vector<int>> wave1;  // 当前时刻的振幅数据
    std::vector<std::vector<int>> wave2;  // 上一时刻的振幅数据
    const short PIC_WIDTH = 413; //窗口的宽度
    const short PIC_HEIGHT = 149;  //窗口的高度

}watergdiplus;

WaterWaveEffectgdiplus::WaterWaveEffectgdiplus()
{
    init();
}

WaterWaveEffectgdiplus::~WaterWaveEffectgdiplus()
{
    FreeRipple();
}

void WaterWaveEffectgdiplus::init()
{    // 初始化 GDI+
    GdiplusStartupInput gdiplusStartupInput;  // GDI+ 启动输入参数
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);  // 启动 GDI+
    wave1.resize(PIC_HEIGHT, std::vector<int>(PIC_WIDTH, 0));
    wave2.resize(PIC_HEIGHT, std::vector<int>(PIC_WIDTH, 0));
    for (int i = 0; i < 87; i++)
    {
        BYTE* newBuffer = new BYTE[PIC_WIDTH * PIC_HEIGHT * sizeof(DWORD)];
        bufferPool.push_back(newBuffer);
    }
    CreategdiplusBitmap();
}

void WaterWaveEffectgdiplus::WaveRender(ID2D1RenderTarget* pRenderTarget, Bitmap* originalBitmap)
{
    BitmapData bitmapData;
    originalBitmap->LockBits(NULL, ImageLockModeRead, PixelFormat32bppPARGB, &bitmapData);

    DWORD* originalData = (DWORD*)bitmapData.Scan0;
    DWORD* newData = nullptr;
    if (!bufferPool.empty())
    {
        BYTE* byteBuffer = bufferPool.back();
        bufferPool.pop_back();
        newData = (DWORD*)byteBuffer;
        memset(newData, 0, sizeof(DWORD) * PIC_WIDTH * PIC_HEIGHT);  // 初始化 newData
    }
    else
    {
        newData = new DWORD[PIC_WIDTH * PIC_HEIGHT * sizeof(DWORD)];
        memset(newData, 0, sizeof(DWORD) * PIC_WIDTH * PIC_HEIGHT);  // 初始化 newData
    }

    for (int y = 1; y < PIC_HEIGHT - 1; y++) {
        for (int x = 1; x < PIC_WIDTH - 1; x++) {
            //第一种偏移方式
            //int waveData = (1024 - wave1[y][x]);
            //int a = ((x - PIC_WIDTH / 2) * waveData / 1024) + PIC_WIDTH / 2;
            //int b = ((y - PIC_HEIGHT / 2) * waveData / 1024) + PIC_HEIGHT / 2;
            //第二种偏移方式
            int a = x + (wave1[y][x - 1] - wave1[y][x + 1]);
            int b = y + (wave1[y - 1][x] - wave1[y + 1][x]);

            if (a >= 1 && a < PIC_WIDTH - 1 && b >= 1 && b < PIC_HEIGHT - 1) {

                if (newData)
                    newData[x + y * PIC_WIDTH] = originalData[a + b * PIC_WIDTH];
            }
        }
    }

    D2D1_BITMAP_PROPERTIES bitmapProperties = {
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.0f,96.0f };

    ID2D1Bitmap* pD2DBitmap = NULL;
    pRenderTarget->CreateBitmap(D2D1::SizeU(bitmapData.Width, bitmapData.Height),
        reinterpret_cast<const uint8_t*>(newData),
        bitmapData.Stride,
        &bitmapProperties,
        &pD2DBitmap);

    // 使用 pD2DBitmap 进行 Direct2D 的绘制操作
    if (pD2DBitmap)
        pRenderTarget->DrawBitmap(pD2DBitmap);

    // 释放 D2D 位图资源
    if (pD2DBitmap != NULL) {
        pD2DBitmap->Release();
        pD2DBitmap = NULL;
    }
    originalBitmap->UnlockBits(&bitmapData);
    // 在函数结束时将缓冲区放回缓冲区池
    if (newData)
    {
        bufferPool.push_back((BYTE*)newData);
    }
    else
    {
        delete[] newData;
    }
}
void WaterWaveEffectgdiplus::WaveRender2(ID2D1RenderTarget* pRenderTarget, Bitmap* originalBitmap)
{
    BitmapData bitmapData;
    originalBitmap->LockBits(NULL, ImageLockModeRead, PixelFormat32bppPARGB, &bitmapData);

    BYTE* originalData = (BYTE*)bitmapData.Scan0;
    BYTE* newData = nullptr;
    if (!bufferPool.empty())
    {
        newData = bufferPool.back();
        bufferPool.pop_back();
        memset(newData, 0, sizeof(BYTE) * PIC_WIDTH * PIC_HEIGHT * sizeof(DWORD));  // 初始化 newData
    }
    else
    {
        newData = new BYTE[PIC_WIDTH * PIC_HEIGHT * sizeof(DWORD)];
        memset(newData, 0, sizeof(DWORD) * PIC_WIDTH * PIC_HEIGHT);  // 初始化 newData
    }
    for (int y = 1; y < PIC_HEIGHT - 1; y++) {
        for (int x = 1; x < PIC_WIDTH - 1; x++) {
            //第一种偏移方式
            //int waveData = (1024 - wave1[y][x]);
            //int a = ((x - PIC_WIDTH / 2) * waveData / 1024) + PIC_WIDTH / 2;
            //int b = ((y - PIC_HEIGHT / 2) * waveData / 1024) + PIC_HEIGHT / 2;
            //第二种偏移方式
            int a = x + (wave1[y][x - 1] - wave1[y][x + 1]);
            int b = y + (wave1[y - 1][x] - wave1[y + 1][x]);

            if (a >= 1 && a < PIC_WIDTH - 1 && b >= 1 && b < PIC_HEIGHT - 1) {
                DWORD originalPixel = ((DWORD*)originalData)[a + b * PIC_WIDTH];


                int newIndex = x * 4 + y * bitmapData.Stride;
                if (newData)
                {
                    newData[newIndex + 0] = (originalPixel) & 0xFF;
                    newData[newIndex + 1] = (originalPixel >> 8) & 0xFF;
                    newData[newIndex + 2] = (originalPixel >> 16) & 0xFF;
                    newData[newIndex + 3] = (originalPixel >> 24) & 0xFF;
                }
            }
        }
    }

    D2D1_BITMAP_PROPERTIES bitmapProperties = {
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.0f,96.0f };

    ID2D1Bitmap* pD2DBitmap = NULL;
    pRenderTarget->CreateBitmap(D2D1::SizeU(bitmapData.Width, bitmapData.Height),
        reinterpret_cast<const uint8_t*>(newData),
        bitmapData.Stride,
        &bitmapProperties,
        &pD2DBitmap);

    // 使用 pD2DBitmap 进行 Direct2D 的绘制操作
    if (pD2DBitmap)
        pRenderTarget->DrawBitmap(pD2DBitmap);

    // 释放 D2D 位图资源
    if (pD2DBitmap != NULL) {
        pD2DBitmap->Release();
        pD2DBitmap = NULL;
    }
    originalBitmap->UnlockBits(&bitmapData);
    if (newData)
    {
        bufferPool.push_back((BYTE*)newData);
    }
    else
    {
        delete[] newData;
    }

}
void WaterWaveEffectgdiplus::WaveRender3(ID2D1RenderTarget* pRenderTarget, Bitmap* originalBitmap)
{
    BitmapData bitmapData;
    originalBitmap->LockBits(NULL, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    BYTE* originalData = (BYTE*)bitmapData.Scan0;
    DWORD* newData = nullptr;
    if (!bufferPool.empty())
    {
        BYTE* byteBuffer = bufferPool.back();
        bufferPool.pop_back();
        newData = (DWORD*)byteBuffer;
        memset(newData, 0, sizeof(DWORD) * PIC_WIDTH * PIC_HEIGHT);  // 初始化 newData
    }
    else
    {
        newData = new DWORD[PIC_WIDTH * PIC_HEIGHT * sizeof(DWORD)];
        memset(newData, 0, sizeof(DWORD) * PIC_WIDTH * PIC_HEIGHT);  // 初始化 newData
    }

    for (int y = 1; y < PIC_HEIGHT - 1; y++) {
        for (int x = 1; x < PIC_WIDTH - 1; x++) {
            //第一种偏移方式
            // int waveData = (1024 - wave1[y][x]);
            // int a = ((x - PIC_WIDTH / 2) * waveData / 1024) + PIC_WIDTH / 2;
            // int b = ((y - PIC_HEIGHT / 2) * waveData / 1024) + PIC_HEIGHT / 2;
            //第二种偏移方式
            int a = x + (wave1[y][x - 1] - wave1[y][x + 1]);
            int b = y + (wave1[y - 1][x] - wave1[y + 1][x]);

            if (a >= 1 && a < PIC_WIDTH - 1 && b >= 1 && b < PIC_HEIGHT - 1) {
                DWORD originalPixel = ((DWORD*)originalData)[a + b * PIC_WIDTH];

                // 分离颜色和透明度通道
                BYTE alpha = (originalPixel >> 24) & 0xFF;
                BYTE r = (originalPixel >> 16) & 0xFF;
                BYTE g = (originalPixel >> 8) & 0xFF;
                BYTE b = originalPixel & 0xFF;

                // 根据透明度调整颜色值
                r = (r * alpha) / 255;
                g = (g * alpha) / 255;
                b = (b * alpha) / 255;
                if (newData)
                    newData[x + y * PIC_WIDTH] = (alpha << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }

    D2D1_BITMAP_PROPERTIES bitmapProperties = {
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.0f,96.0f };

    ID2D1Bitmap* pD2DBitmap = NULL;
    pRenderTarget->CreateBitmap(D2D1::SizeU(bitmapData.Width, bitmapData.Height),
        reinterpret_cast<const uint8_t*>(newData),
        bitmapData.Stride,
        &bitmapProperties,
        &pD2DBitmap);

    // 使用 pD2DBitmap 进行 Direct2D 的绘制操作
    if (pD2DBitmap)
        pRenderTarget->DrawBitmap(pD2DBitmap);

    // 释放 D2D 位图资源
    if (pD2DBitmap != NULL) {
        pD2DBitmap->Release();
        pD2DBitmap = NULL;
    }
    originalBitmap->UnlockBits(&bitmapData);
    // 在函数结束时将缓冲区放回缓冲区池
        // 在函数结束时将缓冲区放回缓冲区池
    if (newData)
    {
        bufferPool.push_back((BYTE*)newData);
    }
    else
    {
        delete[] newData;
    }

}

void WaterWaveEffectgdiplus::WaveRenderGdiBYTE(Graphics* graphics, Bitmap* originalBitmap)
{
    BitmapData bitmapData;
    originalBitmap->LockBits(NULL, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    BYTE* originalData = (BYTE*)bitmapData.Scan0;
    BYTE* newData = new BYTE[bitmapData.Stride * PIC_HEIGHT];
    memset(newData, 0, sizeof(DWORD) * PIC_WIDTH * PIC_HEIGHT);  // 初始化 newData
    int lineIndex = 0;
    for (int y = 1; y < PIC_HEIGHT - 1; y++) {
        for (int x = 1; x < PIC_WIDTH - 1; x++)
        {
            int waveData = (1024 - wave1[y][x]);
            int a = (x - PIC_WIDTH / 2) * waveData / 1024 + PIC_WIDTH / 2;
            int b = (y - PIC_HEIGHT / 2) * waveData / 1024 + PIC_HEIGHT / 2;

            if (0 <= a && a < PIC_WIDTH && 0 <= b && b < PIC_HEIGHT)
            {
                int sourceIndex = b * bitmapData.Stride + a * 4;

                for (int c = 0; c < 4; c++)
                {
                    if (lineIndex + c >= 0 && lineIndex + c < bitmapData.Stride * PIC_HEIGHT && sourceIndex + c >= 0 && sourceIndex + c < bitmapData.Stride * PIC_HEIGHT)
                        newData[lineIndex + c] = originalData[sourceIndex + c];
                }
            }

            lineIndex += 4;
        }
    }

    Bitmap newBitmap(PIC_WIDTH, PIC_HEIGHT, bitmapData.Stride, PixelFormat32bppARGB, newData);
    graphics->DrawImage(&newBitmap, 0, 0);

    originalBitmap->UnlockBits(&bitmapData);
    delete[] newData;
}

void WaterWaveEffectgdiplus::calcNextFrameWave()
{
    for (int y = 1; y < PIC_HEIGHT - 1; y++) {
        for (int x = 1; x < PIC_WIDTH - 1; x++) {
            // 公式：X0'= (X1+X2+X3+X4) / 2 - X0
            wave2[y][x] = ((wave1[y - 1][x] + wave1[y + 1][x] + wave1[y][x - 1] + wave1[y][x + 1]) >> 1) - wave2[y][x];
            // 波能衰减
            wave2[y][x] -= wave2[y][x] >> 4;
        }
    }
    // 交换 wave1 和 wave2，实现画面特效传递
    std::swap(wave1, wave2);
}

void WaterWaveEffectgdiplus::putStone(int x, int y, int stoneSize, int stoneWeight)
{
    if (x >= PIC_WIDTH - stoneSize || x < stoneSize || y >= PIC_HEIGHT - stoneSize || y < stoneSize) {
        return;
    }
    for (int posx = x - stoneSize; posx < x + stoneSize; posx++) {
        for (int posy = y - stoneSize; posy < y + stoneSize; posy++) {
            if ((posx - x) * (posx - x) + (posy - y) * (posy - y) < stoneSize * stoneSize) {
                wave1[posy][posx] = -stoneWeight;
            }
        }
    }
}

void WaterWaveEffectgdiplus::FreeRipple()
{
    for (int i = 0; i <= 86; i++)
    {
        if (w_pbitmap[i] != nullptr)
        {
            delete w_pbitmap[i];
            w_pbitmap[i] = nullptr;
        }
    }
    wave1.clear();
    wave2.clear();
    //释放缓冲池
    for (auto buffer : bufferPool)
    {
        delete[] buffer;
    }
    bufferPool.clear();
    GdiplusShutdown(gdiplusToken);  // 关闭 GDI+
}

//-----------------------------------------------------------------
// 从应用程序资源文件加载图像。从资源文件加载gdi+位图
//-----------------------------------------------------------------
HRESULT WaterWaveEffectgdiplus::LoadResourcegdiplusBitmap(PCWSTR resourceName, PCWSTR resourceType, Bitmap** ppBitmap)
{
    HRSRC imageResHandle = NULL;                        // 资源句柄，图像处理程序
    HGLOBAL imageResDataHandle = NULL;                  // 内存句柄,图像数据处理程序
    LPVOID pImageFile = NULL;                            // 无类型指针，图片文件
    DWORD imageFileSize = 0;                            // 32位无符号整数

    imageResHandle = FindResource(THIS_HINSTANCE, resourceName, resourceType);  // 查找资源
    HRESULT hr = imageResHandle ? S_OK : E_FAIL;
    if (SUCCEEDED(hr)) {
        imageResDataHandle = LoadResource(THIS_HINSTANCE, imageResHandle);
        hr = imageResDataHandle ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        pImageFile = LockResource(imageResDataHandle);
        hr = pImageFile ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        imageFileSize = SizeofResource(THIS_HINSTANCE, imageResHandle);
        hr = imageFileSize ? S_OK : E_FAIL;
    }

    // 创建内存流
    IStream* pStream = SHCreateMemStream((BYTE*)pImageFile, imageFileSize);
    if (pStream == NULL)
    {
        hr = E_FAIL;
        return hr;
    }

    *ppBitmap = Bitmap::FromStream(pStream);  // 从流创建位图

    // 释放流
    pStream->Release();
    return hr;
}

//-----------------------------------------------------------------
HRESULT WaterWaveEffectgdiplus::LoadResourcegdipluscolorBitmap(PCWSTR resourceName, PCWSTR resourceType, Bitmap** ppBitmap)
{
    HRSRC imageResHandle = NULL;                        // 资源句柄，图像处理程序
    HGLOBAL imageResDataHandle = NULL;                  // 内存句柄,图像数据处理程序
    LPVOID pImageFile = NULL;                            // 无类型指针，图片文件
    DWORD imageFileSize = 0;                            // 32位无符号整数

    imageResHandle = FindResource(THIS_HINSTANCE, resourceName, resourceType);  // 查找资源
    HRESULT hr = imageResHandle ? S_OK : E_FAIL;
    if (SUCCEEDED(hr)) {
        imageResDataHandle = LoadResource(THIS_HINSTANCE, imageResHandle);
        hr = imageResDataHandle ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        pImageFile = LockResource(imageResDataHandle);
        hr = pImageFile ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        imageFileSize = SizeofResource(THIS_HINSTANCE, imageResHandle);
        hr = imageFileSize ? S_OK : E_FAIL;
    }

    // 创建内存流
    IStream* pStream = SHCreateMemStream((BYTE*)pImageFile, imageFileSize);
    if (pStream == NULL)
    {
        hr = E_FAIL;
        return hr;
    }

    *ppBitmap = Bitmap::FromStream(pStream);  // 从流创建位图

    // 释放流
    pStream->Release();
    // 改变颜色通道
    if (*ppBitmap)
    {
        int width = (*ppBitmap)->GetWidth();
        int height = (*ppBitmap)->GetHeight();
        Rect rect(0, 0, width, height);
        BitmapData bitmapData;
        (*ppBitmap)->LockBits(&rect, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData);
        BYTE* pixels = static_cast<BYTE*>(bitmapData.Scan0);
        int stride = bitmapData.Stride;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int index = y * stride + x * 4;
                if (pixels[index + 3] != 0)
                {
                    std::cout << "At position (" << x << ", " << y << "), pixels[index + 1] = " << static_cast<int>(pixels[index + 1]) << std::endl;
                    pixels[index + 2] = pixels[index + 1];
                }
            }
        }
        (*ppBitmap)->UnlockBits(&bitmapData);
    }
    return hr;
}

// 创建位图
void WaterWaveEffectgdiplus::CreategdiplusBitmap()
{
    for (int i = 1; i <= 86; i++)
    {
        if (w_pbitmap[i] == nullptr)
            LoadResourcegdiplusBitmap(MAKEINTRESOURCE(2000 + i), L"PNG", &w_pbitmap[i]);
    }
    if (w_pbitmap[0] == nullptr)
        LoadResourcegdipluscolorBitmap(MAKEINTRESOURCE(2000), L"PNG", &w_pbitmap[0]);

}
