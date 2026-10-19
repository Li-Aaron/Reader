// Gfx.cpp : image helpers (background, cover, loading gif, resource images)
#include "Reader.h"
Gdiplus::Bitmap* LoadBGImage(int w, int h, BYTE alpha)
{
    static Gdiplus::Bitmap *bgimg = NULL;
    static TCHAR curfile[MAX_PATH] = {0};
    static int curWidth = 0;
    static int curHeight = 0;
    static int curmode = Stretch;
    static BYTE s_alpha = 0xFF;
    Gdiplus::Bitmap *image = NULL;
    Gdiplus::Graphics *graphics = NULL;
    Gdiplus::ImageAttributes ImgAtt;
    Gdiplus::RectF rcDrawRect;

    if (!_header->bg_image.enable || !_header->bg_image.file_name[0])
    {
        return NULL;
    }

    if (_tcscmp(_header->bg_image.file_name, curfile) == 0 && curWidth == w && curHeight == h 
        && curmode == _header->bg_image.mode && bgimg && alpha == s_alpha)
    {
        return bgimg;
    }

    if (!FileExists(_header->bg_image.file_name))
    {
        _header->bg_image.file_name[0] = 0;
        return NULL;
    }
    
    if (bgimg)
    {
        delete bgimg;
        bgimg = NULL;
    }
    _tcscpy(curfile, _header->bg_image.file_name);
    curWidth = w;
    curHeight = h;
    curmode = _header->bg_image.mode;
    s_alpha = alpha;
    
    // load image file
    image = Gdiplus::Bitmap::FromFile(curfile);
    if (image == NULL)
        return NULL;
    if (Gdiplus::Ok != image->GetLastStatus())
    {
        delete image;
        image = NULL;
        return NULL;
    }

    // create bg image
    bgimg = new Gdiplus::Bitmap(curWidth, curHeight, PixelFormat32bppARGB);
    rcDrawRect.X=0.0;
    rcDrawRect.Y=0.0;
    rcDrawRect.Width=(float)curWidth;
    rcDrawRect.Height=(float)curHeight;
    graphics = Gdiplus::Graphics::FromImage(bgimg);
    graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

    Gdiplus::ColorMatrix colorMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, alpha/255.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f
    };
    ImgAtt.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
    switch (curmode)
    {
    case Stretch:
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)image->GetWidth(),(float)image->GetHeight(), Gdiplus::UnitPixel,&ImgAtt);
        break;
    case Tile:
        ImgAtt.SetWrapMode(Gdiplus::WrapModeTile);
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)curWidth,(float)curHeight, Gdiplus::UnitPixel,&ImgAtt);
        break;
    case TileFlip:
        ImgAtt.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)curWidth,(float)curHeight, Gdiplus::UnitPixel,&ImgAtt);
        break;
    default:
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)image->GetWidth(),(float)image->GetHeight(), Gdiplus::UnitPixel,&ImgAtt);
        break;
    }

    delete image;
    delete graphics;
    return bgimg;
}

BOOL PlayLoadingImage(HWND hWnd)
{
    UINT count;
    GUID *pDimensionIDs = NULL;
    WCHAR strGuid[39];
    UINT size;
    GUID Guid = Gdiplus::FrameDimensionTime;

    if (!_loading)
    {
        // create loading data
        _loading = (loading_data_t *)malloc(sizeof(loading_data_t));
        memset(_loading, 0, sizeof(loading_data_t));

        // load image from resource
        if (!LoadResourceImage(MAKEINTRESOURCE(IDR_GIF_LOADING), _T("GIF"), &(_loading->image), &(_loading->hMemory)))
        {
            free(_loading);
            _loading = NULL;
            return FALSE;
        }

        count = _loading->image->GetFrameDimensionsCount();
        pDimensionIDs = (GUID*)malloc(sizeof(GUID)*count);
        _loading->image->GetFrameDimensionsList(pDimensionIDs, count);
        StringFromGUID2(pDimensionIDs[0], strGuid, 39);
        _loading->frameCount = _loading->image->GetFrameCount(&pDimensionIDs[0]);
        free(pDimensionIDs);
        size = _loading->image->GetPropertyItemSize(PropertyTagFrameDelay);
        _loading->item = (Gdiplus::PropertyItem*)malloc(size);
        _loading->image->GetPropertyItem(PropertyTagFrameDelay, size, _loading->item);
    }

    _loading->enable = TRUE;
    _loading->frameIndex = 0;
    Guid = Gdiplus::FrameDimensionTime;
    _loading->image->SelectActiveFrame(&Guid, _loading->frameIndex);

    SetTimer(hWnd, IDT_TIMER_LOADING, ((UINT*)(_loading->item[0].value))[_loading->frameIndex] * 10, NULL);
    _loading->frameIndex = (++ _loading->frameIndex) % _loading->frameCount;
    Invalidate(hWnd, TRUE, FALSE);
    return TRUE;
}

BOOL StopLoadingImage(HWND hWnd)
{
    if (!_loading)
        return FALSE;
    _loading->enable = FALSE;
    KillTimer(hWnd, IDT_TIMER_LOADING);
    Invalidate(hWnd, TRUE, FALSE);
    return TRUE;
}

HBITMAP CreateAlphaBGColorBitmap(HWND hWnd, COLORREF inColour, int width, int height, BYTE alpha)
{
    // Create DC and select font into it
    HDC hColorDC = CreateCompatibleDC(NULL);
    HBITMAP hDIB = NULL;
    BITMAPINFOHEADER BMIH;
    void *pvBits = NULL;
    HBITMAP hOldBMP = NULL;
    BYTE* DataPtr = NULL;
    BYTE FillR,FillG,FillB;
    int x,y;

    // Specify DIB setup
    memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
    BMIH.biSize = sizeof(BMIH);
    BMIH.biWidth = width;
    BMIH.biHeight = height;
    BMIH.biPlanes = 1;
    BMIH.biBitCount = 32;
    BMIH.biCompression = BI_RGB;

    // Create and select DIB into DC
    hDIB = CreateDIBSection(hColorDC, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&pvBits, NULL, 0); 
    hOldBMP = (HBITMAP)SelectObject(hColorDC, hDIB);
    if (hOldBMP)
    {
        DataPtr = (BYTE*)pvBits;
        FillR = GetRValue(inColour);
        FillG = GetGValue(inColour);
        FillB = GetBValue(inColour);
        for (y = 0; y < BMIH.biHeight; y++)
        { 
            for (x = 0; x < BMIH.biWidth; x++)
            {
                *DataPtr++ = (FillB * alpha) >> 8; 
                *DataPtr++ = (FillG * alpha) >> 8; 
                *DataPtr++ = (FillR * alpha) >> 8;
                *DataPtr++ = alpha;
            }
        }

        // De-select bitmap
        SelectObject(hColorDC, hOldBMP);
    }

    // destroy temp DC
    DeleteDC(hColorDC);

    // Return DIBSection 
    return hDIB;
}

BOOL LoadResourceImage(LPCWSTR lpName, LPCWSTR lpType, Gdiplus::Bitmap **image, HGLOBAL *hMemory)
{
    HRSRC hResource;
    HGLOBAL hResLoad;
    const void* pResourceData = NULL;
    IStream* pStream = NULL;
    DWORD imageSize;
    void* buffer = NULL;

    hResource = ::FindResource(hInst, lpName, lpType);
    if (!hResource)
        return FALSE;

    hResLoad = ::LoadResource(hInst, hResource);
    if (!hResLoad)
        return FALSE;

    pResourceData = ::LockResource(hResLoad);
    if (!pResourceData)
    {
        ::FreeResource(hResLoad);
        return FALSE;
    }

    imageSize = ::SizeofResource(hInst, hResource);
    if (0 == imageSize)
    {
        ::FreeResource(hResLoad);
        return FALSE;
    }

    *hMemory = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);
    if (!(*hMemory))
    {
        ::FreeResource(hResLoad);
        return FALSE;
    }

    buffer = ::GlobalLock(*hMemory);
    if (!buffer)
    {
        ::FreeResource(hResLoad);
        ::GlobalFree(*hMemory);
        return FALSE;
    }

    CopyMemory(buffer, pResourceData, imageSize);

    if (::CreateStreamOnHGlobal(*hMemory, FALSE, &pStream) == S_OK)
    {
        *image = Gdiplus::Bitmap::FromStream(pStream);
        pStream->Release();
    }
    ::FreeResource(hResLoad);
    ::GlobalUnlock(*hMemory);
    //::GlobalFree(hMemory);

    if ((*image) && Gdiplus::Ok == (*image)->GetLastStatus())
    {
        return TRUE;
    }

    if (*image)
        delete (*image);
    *image = NULL;
    ::GlobalFree(*hMemory);
    *hMemory = NULL;
    return FALSE;
}
