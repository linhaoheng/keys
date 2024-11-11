/***********************************************************************
功能：d2d水波纹效果
作者：林豪横
简介：
***********************************************************************/

App app;

class WaterWaveEffectd2d
{
public:
    WaterWaveEffectd2d();
    ~WaterWaveEffectd2d();
    // 功能：根据水波波幅渲染水波位图数据
    void WaveRender(ID2D1RenderTarget* pRenderTarget, IWICBitmap* pWicBitmap);
    // 功能：水波扩散
    void calcNextFrameWave();
    // 功能：扔石子（设定波源）
    void putStone(int x, int y, int stoneSize, int stoneWeight);

    void CreateWicBitmap();
private:
    // 功能：初始化
    void init();
    // 功能：释放水波对象资源
    void FreeRipple();

    HRESULT LoadResourceWicBitmap(const WCHAR* resourceName, const WCHAR* resourceType, IWICBitmap** pWIC, IWICImagingFactory* pIWICFactory);
    HRESULT LoadResourceWiccolorBitmap(const WCHAR* resourceName, const WCHAR* resourceType, IWICBitmap** pWIC, IWICImagingFactory* pIWICFactory);
    std::vector<BYTE*> bufferPool;  // 缓冲区池

public:
    IWICBitmap* w_pbitmap[87];

private:
    std::vector<std::vector<int>> wave1;  // 当前时刻的振幅数据
    std::vector<std::vector<int>> wave2;  // 上一时刻的振幅数据
    int PIC_WIDTH = 413; //窗口的宽度
    int PIC_HEIGHT = 149;  //窗口的高度


}waterd2d;

WaterWaveEffectd2d::WaterWaveEffectd2d()
{
    init();
}

WaterWaveEffectd2d::~WaterWaveEffectd2d()
{
    FreeRipple();
}

void WaterWaveEffectd2d::init()
{
    CreateWicBitmap();
    wave1.resize(PIC_HEIGHT, std::vector<int>(PIC_WIDTH, 0));
    wave2.resize(PIC_HEIGHT, std::vector<int>(PIC_WIDTH, 0));
    for (int i = 0; i < 87; i++)
    {
        BYTE* newBuffer = new BYTE[PIC_WIDTH * PIC_HEIGHT * sizeof(DWORD)];
        bufferPool.push_back(newBuffer);
    }
}

void WaterWaveEffectd2d::WaveRender(ID2D1RenderTarget* pRenderTarget, IWICBitmap* pWicBitmap)
{
    IWICBitmapLock* pILock = NULL;
    WICRect rcLock = { 0, 0, PIC_WIDTH, PIC_HEIGHT };
    pWicBitmap->Lock(&rcLock, WICBitmapLockWrite, &pILock);

    UINT cbBufferSize = 0;
    BYTE* pv = NULL;
    // 获取锁定矩形中左上角像素的指针
    pILock->GetDataPointer(&cbBufferSize, &pv);

    BYTE* newData = nullptr;
    if (!bufferPool.empty())
    {
        newData = bufferPool.back();
        bufferPool.pop_back();
        memset(newData, 0, sizeof(DWORD) * PIC_WIDTH * PIC_HEIGHT);  // 初始化 newData
    }
    else
    {
        // 如果 bufferPool 超过 87 个，移除最旧的一个
        if (bufferPool.size() > 87) {
            delete[] bufferPool.front();
            bufferPool.erase(bufferPool.begin());
        }
        // 创建新的缓冲区
        newData = new BYTE[PIC_WIDTH * PIC_HEIGHT * sizeof(DWORD)];
        memset(newData, 0, sizeof(DWORD) * PIC_WIDTH * PIC_HEIGHT);
    }
    for (int y = 1; y < PIC_HEIGHT - 1; y++) {
        for (int x = 1; x < PIC_WIDTH - 1; x++) {

            // 第一种偏移方式
            // int waveData = (1024 - wave1[y][x]);
            // int a = x * waveData / 1024;
            // int b = y * waveData / 1024;
            // 第二种偏移方式
            int a = x + (wave1[y][x - 1] - wave1[y][x + 1]);
            int b = y + (wave1[y - 1][x] - wave1[y + 1][x]);
            if (a >= 1 && a < PIC_WIDTH - 1 && b >= 1 && b < PIC_HEIGHT - 1) {
                int index = y * PIC_WIDTH * 4 + x * 4;
                int sourceIndex = a * 4 + b * PIC_WIDTH * 4;
                // 不进行任何处理，直接将原始像素值写回新数据缓冲区
                if (newData) {
                    newData[index + 0] = pv[sourceIndex + 0];
                    newData[index + 1] = pv[sourceIndex + 1];
                    newData[index + 2] = pv[sourceIndex + 2];
                    newData[index + 3] = pv[sourceIndex + 3];
                }
            }
        }
    }

    D2D1_BITMAP_PROPERTIES bitmapProperties = {
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.0f,96.0f };

    ID2D1Bitmap* pD2DBitmap = NULL;
    pRenderTarget->CreateBitmap(D2D1::SizeU(PIC_WIDTH, PIC_HEIGHT), (BYTE*)(newData), PIC_WIDTH * 4, &bitmapProperties, &pD2DBitmap);

    // 使用 pD2DBitmap 进行 Direct2D 的绘制操作
    if (pD2DBitmap)
        pRenderTarget->DrawBitmap(pD2DBitmap);

    // 释放 D2D 位图资源
    SafeRelease(&pD2DBitmap);

    pILock->Release();
    if (newData)
    {
        bufferPool.push_back(newData);
    }
    else
    {
        delete[] newData;
    }
}

void WaterWaveEffectd2d::calcNextFrameWave()
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

void WaterWaveEffectd2d::putStone(int x, int y, int stoneSize, int stoneWeight)
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

void WaterWaveEffectd2d::FreeRipple()
{
    for (int i = 0; i <= 86; i++) {
        SafeRelease(&w_pbitmap[i]);
    }
    wave1.clear();
    wave2.clear();
    //释放缓冲池
    for (auto buffer : bufferPool)
    {
        delete[] buffer;
    }
    bufferPool.clear();
}


// 从资源文件加载WIC位图,
//-----------------------------------------------------------------
HRESULT WaterWaveEffectd2d::LoadResourceWicBitmap(const WCHAR* resourceName, const WCHAR* resourceType, IWICBitmap** pWIC, IWICImagingFactory* pIWICFactory)
{
    HRSRC                   imageResHandle = NULL;              //资源句柄，图像处理程序
    HGLOBAL                 imageResDataHandle = NULL;          //内存句柄,图像数据处理程序
    void* pImageFile = NULL;                  //无类型指针，图片文件
    DWORD                   imageFileSize = 0;                  //32位无符号整数

    IWICStream* pStream = NULL;                     //表示用于引用映像和元数据内容的 WINDOWS 映像组件 (WIC) 流
    IWICBitmapDecoder* pDecoder = NULL;                    //公开表示解码器的方法。
    IWICBitmapFrameDecode* pSource = NULL;                     //是帧级接口，用于访问实际图像位
    UINT                    pwidth = PIC_WIDTH;
    UINT                    pheigh = PIC_HEIGHT;

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

    //---------------------------------------------把图像转为32位
    IWICFormatConverter* pConverter = NULL;
    // 转换图像格式
    if (SUCCEEDED(hr)) {
        WICPixelFormatGUID pixelFormat;
        pSource->GetPixelFormat(&pixelFormat);
        if (pixelFormat != GUID_WICPixelFormat32bppPBGRA) {
            hr = pIWICFactory->CreateFormatConverter(&pConverter);
            if (SUCCEEDED(hr)) {
                hr = pConverter->Initialize(
                    pSource,
                    GUID_WICPixelFormat32bppPBGRA,
                    WICBitmapDitherTypeNone,
                    NULL,
                    0.f,
                    WICBitmapPaletteTypeMedianCut
                );
            }
        }
    }

    //---------------------------------------------增加位图缩放功能
    IWICBitmapScaler* pScaler = NULL;
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
    }
    if (SUCCEEDED(hr)) {
        hr = pScaler->Initialize(pConverter, pwidth, pheigh, WICBitmapInterpolationModeCubic);
    }
    //---------------------------------------------
    // 使用缩放后的pScaler创建pWIC
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapFromSource(pScaler, WICBitmapCacheOnLoad, pWIC);
    }
    SafeRelease(&pDecoder);
    SafeRelease(&pSource);
    SafeRelease(&pStream);
    SafeRelease(&pConverter);
    SafeRelease(&pScaler);
    return hr;
}



HRESULT WaterWaveEffectd2d::LoadResourceWiccolorBitmap(const WCHAR* resourceName, const WCHAR* resourceType, IWICBitmap** pWIC, IWICImagingFactory* pIWICFactory)
{
    HRSRC                   imageResHandle = NULL;              //资源句柄，图像处理程序
    HGLOBAL                 imageResDataHandle = NULL;          //内存句柄,图像数据处理程序
    void* pImageFile = NULL;                  //无类型指针，图片文件
    DWORD                   imageFileSize = 0;                  //32位无符号整数

    IWICStream* pStream = NULL;                     //表示用于引用映像和元数据内容的 WINDOWS 映像组件 (WIC) 流
    IWICBitmapDecoder* pDecoder = NULL;                    //公开表示解码器的方法。
    IWICBitmapFrameDecode* pSource = NULL;                     //是帧级接口，用于访问实际图像位
    UINT                    pwidth = PIC_WIDTH;
    UINT                    pheigh = PIC_HEIGHT;

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

    //---------------------------------------------把图像转为32位
    IWICFormatConverter* pConverter = NULL;
    // 转换图像格式
    if (SUCCEEDED(hr)) {
        WICPixelFormatGUID pixelFormat;
        pSource->GetPixelFormat(&pixelFormat);
        if (pixelFormat != GUID_WICPixelFormat32bppPBGRA) {
            hr = pIWICFactory->CreateFormatConverter(&pConverter);
            if (SUCCEEDED(hr)) {
                hr = pConverter->Initialize(
                    pSource,
                    GUID_WICPixelFormat32bppPBGRA,
                    WICBitmapDitherTypeNone,
                    NULL,
                    0.f,
                    WICBitmapPaletteTypeMedianCut
                );
            }
        }
    }
    //---------------------------------------------增加位图缩放功能
    IWICBitmapScaler* pScaler = NULL;
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
    }
    if (SUCCEEDED(hr)) {
        hr = pScaler->Initialize(pConverter, pwidth, pheigh, WICBitmapInterpolationModeCubic);
    }
    //---------------------------------------------
    // 使用缩放后的pScaler创建pWIC
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapFromSource(pScaler, WICBitmapCacheOnLoad, pWIC);
    }

    if (SUCCEEDED(hr)) {        // 2.从 IWICBitmap 对象读取像素数据
        IWICBitmapLock* pILock = NULL;
        WICRect rcLock = { 0, 0, (INT)pwidth, (INT)pheigh };
        hr = (*pWIC)->Lock(&rcLock, WICBitmapLockWrite, &pILock);
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
    SafeRelease(&pDecoder);
    SafeRelease(&pSource);
    SafeRelease(&pStream);
    SafeRelease(&pConverter);
    SafeRelease(&pScaler);
    return hr;
}

// 创建WIC位图
void WaterWaveEffectd2d::CreateWicBitmap()
{
    for (int i = 1; i <= 86; i++)
    {
        if (w_pbitmap[i] == NULL)
            LoadResourceWicBitmap(MAKEINTRESOURCE(2000 + i), L"PNG", &w_pbitmap[i], app.m_pWICFactory);
    }
    if (w_pbitmap[0] == NULL)
        LoadResourceWiccolorBitmap(MAKEINTRESOURCE(2000), L"PNG", &w_pbitmap[0], app.m_pWICFactory);

}


