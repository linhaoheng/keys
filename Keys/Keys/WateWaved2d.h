/***********************************************************************
���ܣ�d2dˮ����Ч��
���ߣ��ֺ���
��飺
***********************************************************************/

App app;

class WaterWaveEffectd2d
{
public:
    WaterWaveEffectd2d();
    ~WaterWaveEffectd2d();
    // ���ܣ�����ˮ��������Ⱦˮ��λͼ����
    void WaveRender(ID2D1RenderTarget* pRenderTarget, IWICBitmap* pWicBitmap);
    // ���ܣ�ˮ����ɢ
    void calcNextFrameWave();
    // ���ܣ���ʯ�ӣ��趨��Դ��
    void putStone(int x, int y, int stoneSize, int stoneWeight);

    void CreateWicBitmap();
private:
    // ���ܣ���ʼ��
    void init();
    // ���ܣ��ͷ�ˮ��������Դ
    void FreeRipple();

    HRESULT LoadResourceWicBitmap(const WCHAR* resourceName, const WCHAR* resourceType, IWICBitmap** pWIC, IWICImagingFactory* pIWICFactory);
    HRESULT LoadResourceWiccolorBitmap(const WCHAR* resourceName, const WCHAR* resourceType, IWICBitmap** pWIC, IWICImagingFactory* pIWICFactory);
    std::vector<BYTE*> bufferPool;  // ��������

public:
    IWICBitmap* w_pbitmap[87];

private:
    std::vector<std::vector<int>> wave1;  // ��ǰʱ�̵��������
    std::vector<std::vector<int>> wave2;  // ��һʱ�̵��������
    int PIC_WIDTH = 413; //���ڵĿ��
    int PIC_HEIGHT = 149;  //���ڵĸ߶�


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
    // ��ȡ�������������Ͻ����ص�ָ��
    pILock->GetDataPointer(&cbBufferSize, &pv);

    BYTE* newData = nullptr;
    if (!bufferPool.empty())
    {
        newData = bufferPool.back();
        bufferPool.pop_back();
        memset(newData, 0, sizeof(DWORD) * PIC_WIDTH * PIC_HEIGHT);  // ��ʼ�� newData
    }
    else
    {
        // ��� bufferPool ���� 87 �����Ƴ���ɵ�һ��
        if (bufferPool.size() > 87) {
            delete[] bufferPool.front();
            bufferPool.erase(bufferPool.begin());
        }
        // �����µĻ�����
        newData = new BYTE[PIC_WIDTH * PIC_HEIGHT * sizeof(DWORD)];
        memset(newData, 0, sizeof(DWORD) * PIC_WIDTH * PIC_HEIGHT);
    }
    for (int y = 1; y < PIC_HEIGHT - 1; y++) {
        for (int x = 1; x < PIC_WIDTH - 1; x++) {

            // ��һ��ƫ�Ʒ�ʽ
            // int waveData = (1024 - wave1[y][x]);
            // int a = x * waveData / 1024;
            // int b = y * waveData / 1024;
            // �ڶ���ƫ�Ʒ�ʽ
            int a = x + (wave1[y][x - 1] - wave1[y][x + 1]);
            int b = y + (wave1[y - 1][x] - wave1[y + 1][x]);
            if (a >= 1 && a < PIC_WIDTH - 1 && b >= 1 && b < PIC_HEIGHT - 1) {
                int index = y * PIC_WIDTH * 4 + x * 4;
                int sourceIndex = a * 4 + b * PIC_WIDTH * 4;
                // �������κδ���ֱ�ӽ�ԭʼ����ֵд�������ݻ�����
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

    // ʹ�� pD2DBitmap ���� Direct2D �Ļ��Ʋ���
    if (pD2DBitmap)
        pRenderTarget->DrawBitmap(pD2DBitmap);

    // �ͷ� D2D λͼ��Դ
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
            // ��ʽ��X0'= (X1+X2+X3+X4) / 2 - X0
            wave2[y][x] = ((wave1[y - 1][x] + wave1[y + 1][x] + wave1[y][x - 1] + wave1[y][x + 1]) >> 1) - wave2[y][x];
            // ����˥��
            wave2[y][x] -= wave2[y][x] >> 4;
        }
    }
    // ���� wave1 �� wave2��ʵ�ֻ�����Ч����
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
    //�ͷŻ����
    for (auto buffer : bufferPool)
    {
        delete[] buffer;
    }
    bufferPool.clear();
}


// ����Դ�ļ�����WICλͼ,
//-----------------------------------------------------------------
HRESULT WaterWaveEffectd2d::LoadResourceWicBitmap(const WCHAR* resourceName, const WCHAR* resourceType, IWICBitmap** pWIC, IWICImagingFactory* pIWICFactory)
{
    HRSRC                   imageResHandle = NULL;              //��Դ�����ͼ�������
    HGLOBAL                 imageResDataHandle = NULL;          //�ڴ���,ͼ�����ݴ������
    void* pImageFile = NULL;                  //������ָ�룬ͼƬ�ļ�
    DWORD                   imageFileSize = 0;                  //32λ�޷�������

    IWICStream* pStream = NULL;                     //��ʾ��������ӳ���Ԫ�������ݵ� WINDOWS ӳ����� (WIC) ��
    IWICBitmapDecoder* pDecoder = NULL;                    //������ʾ�������ķ�����
    IWICBitmapFrameDecode* pSource = NULL;                     //��֡���ӿڣ����ڷ���ʵ��ͼ��λ
    UINT                    pwidth = PIC_WIDTH;
    UINT                    pheigh = PIC_HEIGHT;

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

    //---------------------------------------------��ͼ��תΪ32λ
    IWICFormatConverter* pConverter = NULL;
    // ת��ͼ���ʽ
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

    //---------------------------------------------����λͼ���Ź���
    IWICBitmapScaler* pScaler = NULL;
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
    }
    if (SUCCEEDED(hr)) {
        hr = pScaler->Initialize(pConverter, pwidth, pheigh, WICBitmapInterpolationModeCubic);
    }
    //---------------------------------------------
    // ʹ�����ź��pScaler����pWIC
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
    HRSRC                   imageResHandle = NULL;              //��Դ�����ͼ�������
    HGLOBAL                 imageResDataHandle = NULL;          //�ڴ���,ͼ�����ݴ������
    void* pImageFile = NULL;                  //������ָ�룬ͼƬ�ļ�
    DWORD                   imageFileSize = 0;                  //32λ�޷�������

    IWICStream* pStream = NULL;                     //��ʾ��������ӳ���Ԫ�������ݵ� WINDOWS ӳ����� (WIC) ��
    IWICBitmapDecoder* pDecoder = NULL;                    //������ʾ�������ķ�����
    IWICBitmapFrameDecode* pSource = NULL;                     //��֡���ӿڣ����ڷ���ʵ��ͼ��λ
    UINT                    pwidth = PIC_WIDTH;
    UINT                    pheigh = PIC_HEIGHT;

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

    //---------------------------------------------��ͼ��תΪ32λ
    IWICFormatConverter* pConverter = NULL;
    // ת��ͼ���ʽ
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
    //---------------------------------------------����λͼ���Ź���
    IWICBitmapScaler* pScaler = NULL;
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
    }
    if (SUCCEEDED(hr)) {
        hr = pScaler->Initialize(pConverter, pwidth, pheigh, WICBitmapInterpolationModeCubic);
    }
    //---------------------------------------------
    // ʹ�����ź��pScaler����pWIC
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapFromSource(pScaler, WICBitmapCacheOnLoad, pWIC);
    }

    if (SUCCEEDED(hr)) {        // 2.�� IWICBitmap �����ȡ��������
        IWICBitmapLock* pILock = NULL;
        WICRect rcLock = { 0, 0, (INT)pwidth, (INT)pheigh };
        hr = (*pWIC)->Lock(&rcLock, WICBitmapLockWrite, &pILock);
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
    SafeRelease(&pDecoder);
    SafeRelease(&pSource);
    SafeRelease(&pStream);
    SafeRelease(&pConverter);
    SafeRelease(&pScaler);
    return hr;
}

// ����WICλͼ
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


