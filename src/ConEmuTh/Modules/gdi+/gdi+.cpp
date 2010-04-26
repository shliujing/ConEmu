
#include <windows.h>
#include <GdiPlus.h>
#include <crtdbg.h>
#include "../ThumbSDK.h"

//#include "../PVD2Helper.h"
//#include "../BltHelper.h"
//#include "../MStream.h"

typedef __int32 i32;
typedef __int64 i64;
typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef DWORD u32;

HMODULE ghModule;

//pvdInitPlugin2 ip = {0};

#define PGE_INVALID_FRAME        0x1001
#define PGE_ERROR_BASE           0x80000000
#define PGE_DLL_NOT_FOUND        0x80001001
#define PGE_FILE_NOT_FOUND       0x80001003
#define PGE_NOT_ENOUGH_MEMORY    0x80001004
#define PGE_INVALID_CONTEXT      0x80001005
#define PGE_FUNCTION_NOT_FOUND   0x80001006
#define PGE_WIN32_ERROR          0x80001007
#define PGE_UNKNOWN_COLORSPACE   0x80001008
#define PGE_UNSUPPORTED_PITCH    0x80001009
#define PGE_INVALID_PAGEDATA     0x8000100A
#define PGE_OLD_PICVIEW          0x8000100B
#define PGE_BITBLT_FAILED        0x8000100C
#define PGE_INVALID_VERSION      0x8000100D
#define PGE_INVALID_IMGSIZE      0x8000100E
#define PGE_UNSUPPORTEDFORMAT    0x8000100F


DWORD gnLastWin32Error = 0;

#ifdef _DEBUG
#define DebugString(x) OutputDebugString(x)
#define PaintDebugString(x) //OutputDebugString(x)
#else
#define DebugString(x)
#define PaintDebugString(x)
#endif



#define DllGetFunction(hModule, FunctionName) FunctionName = (FunctionName##_t)GetProcAddress(hModule, #FunctionName)
//#define MALLOC(n) HeapAlloc(GetProcessHeap(), 0, n)
#define CALLOC(n) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n)
#define FREE(p) HeapFree(GetProcessHeap(), 0, p)
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "
#ifdef HIDE_TODO
#define TODO(s) 
#define WARNING(s) 
#else
#define TODO(s) __pragma(message (FILE_LINE "TODO: " s))
#define WARNING(s) __pragma(message (FILE_LINE "warning: " s))
#endif
#define PRAGMA_ERROR(s) __pragma(message (FILE_LINE "error: " s))

enum tag_GdiStrMagics {
	eGdiStr_Decoder = 0x1002,
	eGdiStr_Image = 0x1003,
	eGdiStr_Bits = 0x1004,
};

struct GDIPlusDecoder
{
	DWORD   nMagic;
	HMODULE hGDIPlus;
	ULONG_PTR gdiplusToken; bool bTokenInitialized;
	HRESULT nErrNumber, nLastError;
	//bool bUseICM, bForceSelfDisplay, bCMYK2RGB;
	BOOL bCoInitialized;
	//const wchar_t* pszPluginKey;
	//BOOL bAsDisplay;
	BOOL bCancelled;

	typedef Gdiplus::Status (WINAPI *GdiplusStartup_t)(OUT ULONG_PTR *token, const Gdiplus::GdiplusStartupInput *input, OUT Gdiplus::GdiplusStartupOutput *output);
	typedef VOID (WINAPI *GdiplusShutdown_t)(ULONG_PTR token);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromFile_t)(GDIPCONST WCHAR* filename, Gdiplus::GpBitmap **bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromStream_t)(IStream* stream, Gdiplus::GpBitmap **bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromFileICM_t)(GDIPCONST WCHAR* filename, Gdiplus::GpBitmap **bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromStreamICM_t)(IStream* stream, Gdiplus::GpBitmap **bitmap);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImageWidth_t)(Gdiplus::GpImage *image, UINT *width);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImageHeight_t)(Gdiplus::GpImage *image, UINT *height);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImagePixelFormat_t)(Gdiplus::GpImage *image, Gdiplus::PixelFormat *format);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipBitmapLockBits_t)(Gdiplus::GpBitmap* bitmap, GDIPCONST Gdiplus::GpRect* rect, UINT flags, Gdiplus::PixelFormat format, Gdiplus::BitmapData* lockedBitmapData);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipBitmapUnlockBits_t)(Gdiplus::GpBitmap* bitmap, Gdiplus::BitmapData* lockedBitmapData);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipDisposeImage_t)(Gdiplus::GpImage *image);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipImageGetFrameCount_t)(Gdiplus::GpImage *image, GDIPCONST GUID* dimensionID, UINT* count);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipImageSelectActiveFrame_t)(Gdiplus::GpImage *image, GDIPCONST GUID* dimensionID, UINT frameIndex);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetPropertyItemSize_t)(Gdiplus::GpImage *image, PROPID propId, UINT* size);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetPropertyItem_t)(Gdiplus::GpImage *image, PROPID propId, UINT propSize, Gdiplus::PropertyItem* buffer);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImageRawFormat_t)(Gdiplus::GpImage *image, OUT GUID* format);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImageFlags_t)(Gdiplus::GpImage *image, UINT *flags);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImagePalette_t)(Gdiplus::GpImage *image, Gdiplus::ColorPalette *palette, INT size);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImagePaletteSize_t)(Gdiplus::GpImage *image, INT *size);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateFromHDC_t)(HDC hdc, Gdiplus::GpGraphics **graphics);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipDeleteGraphics_t)(Gdiplus::GpGraphics *graphics);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipDrawImageRectRectI_t)(Gdiplus::GpGraphics *graphics, Gdiplus::GpImage *image, INT dstx, INT dsty, INT dstwidth, INT dstheight, INT srcx, INT srcy, INT srcwidth, INT srcheight, Gdiplus::GpUnit srcUnit, const Gdiplus::GpImageAttributes* imageAttributes, Gdiplus::DrawImageAbort callback, VOID * callbackData);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromScan0_t)(INT width, INT height, INT stride, Gdiplus::PixelFormat format, BYTE* scan0, Gdiplus::GpBitmap** bitmap);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipFillRectangleI_t)(Gdiplus::GpGraphics *graphics, Gdiplus::GpBrush *brush, INT x, INT y, INT width, INT height);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateSolidFill_t)(Gdiplus::ARGB color, Gdiplus::GpSolidFill **brush);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipDeleteBrush_t)(Gdiplus::GpBrush *brush);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCloneImage_t)(Gdiplus::GpImage *image, Gdiplus::GpImage **cloneImage);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCloneBitmapAreaI_t)(INT x, INT y, INT width, INT height, Gdiplus::PixelFormat format, Gdiplus::GpBitmap *srcBitmap, Gdiplus::GpBitmap **dstBitmap);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipSetImagePalette_t)(Gdiplus::GpImage *image, GDIPCONST Gdiplus::ColorPalette *palette);



	GdiplusStartup_t GdiplusStartup;
	GdiplusShutdown_t GdiplusShutdown;
	GdipCreateBitmapFromFile_t GdipCreateBitmapFromFile;
	//GdipCreateBitmapFromStream_t GdipCreateBitmapFromStream;
	//GdipCreateBitmapFromFileICM_t GdipCreateBitmapFromFileICM;
	//GdipCreateBitmapFromStreamICM_t GdipCreateBitmapFromStreamICM;
	GdipGetImageWidth_t GdipGetImageWidth;
	GdipGetImageHeight_t GdipGetImageHeight;
	GdipGetImagePixelFormat_t GdipGetImagePixelFormat;
	GdipBitmapLockBits_t GdipBitmapLockBits;
	GdipBitmapUnlockBits_t GdipBitmapUnlockBits;
	GdipDisposeImage_t GdipDisposeImage;
	GdipImageGetFrameCount_t GdipImageGetFrameCount;
	GdipImageSelectActiveFrame_t GdipImageSelectActiveFrame;
	GdipGetPropertyItemSize_t GdipGetPropertyItemSize;
	GdipGetPropertyItem_t GdipGetPropertyItem;
	GdipGetImageRawFormat_t GdipGetImageRawFormat;
	GdipGetImageFlags_t GdipGetImageFlags;
	GdipGetImagePalette_t GdipGetImagePalette;
	GdipGetImagePaletteSize_t GdipGetImagePaletteSize;
	GdipCreateFromHDC_t GdipCreateFromHDC;
	GdipDeleteGraphics_t GdipDeleteGraphics;
	GdipDrawImageRectRectI_t GdipDrawImageRectRectI;
	GdipCreateBitmapFromScan0_t GdipCreateBitmapFromScan0;
	GdipFillRectangleI_t GdipFillRectangleI;
	GdipCreateSolidFill_t GdipCreateSolidFill;
	GdipDeleteBrush_t GdipDeleteBrush;
	//GdipCloneImage_t GdipCloneImage;
	GdipCloneBitmapAreaI_t GdipCloneBitmapAreaI;
	GdipSetImagePalette_t GdipSetImagePalette;
	
	GDIPlusDecoder() {
		nMagic = eGdiStr_Decoder;
		hGDIPlus = NULL; gdiplusToken = NULL; bTokenInitialized = false;
		nErrNumber = 0; nLastError = 0; //bUseICM = false; bCoInitialized = FALSE; bCMYK2RGB = false;
		//pszPluginKey = NULL; 
		bCancelled = FALSE; 
		//bForceSelfDisplay = false;
	}


	bool Init(struct CET_Init* pInit)
	{
		bool result = false;
		nErrNumber = 0;

		//pszPluginKey = pInit->pRegKey;

		//ReloadConfig();

		HRESULT hrCoInitialized = CoInitialize(NULL);
		bCoInitialized = SUCCEEDED(hrCoInitialized);

		wchar_t FullPath[MAX_PATH*2+15]; FullPath[0] = 0;
		//if (ghModule)
		//{
		//	PVDSettings::FindFile(L"GdiPlus.dll", FullPath, sizeofarray(FullPath));
		//	hGDIPlus = LoadLibraryW(FullPath);
		//}
		//if (!hGDIPlus)
		hGDIPlus = LoadLibraryW(L"GdiPlus.dll");

		if (!hGDIPlus) {
			nErrNumber = PGE_DLL_NOT_FOUND;

		} else {
			DllGetFunction(hGDIPlus, GdiplusStartup);
			DllGetFunction(hGDIPlus, GdiplusShutdown);
			DllGetFunction(hGDIPlus, GdipCreateBitmapFromFile);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromFileICM);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromStream);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromStreamICM);
			DllGetFunction(hGDIPlus, GdipGetImageWidth);
			DllGetFunction(hGDIPlus, GdipGetImageHeight);
			DllGetFunction(hGDIPlus, GdipGetImagePixelFormat);
			DllGetFunction(hGDIPlus, GdipBitmapLockBits);
			DllGetFunction(hGDIPlus, GdipBitmapUnlockBits);
			DllGetFunction(hGDIPlus, GdipDisposeImage);
			DllGetFunction(hGDIPlus, GdipImageGetFrameCount);
			DllGetFunction(hGDIPlus, GdipImageSelectActiveFrame);
			DllGetFunction(hGDIPlus, GdipGetPropertyItemSize);
			DllGetFunction(hGDIPlus, GdipGetPropertyItem);
			DllGetFunction(hGDIPlus, GdipGetImageRawFormat);
			DllGetFunction(hGDIPlus, GdipGetImageFlags);
			DllGetFunction(hGDIPlus, GdipGetImagePalette);
			DllGetFunction(hGDIPlus, GdipGetImagePaletteSize);
			DllGetFunction(hGDIPlus, GdipCreateFromHDC);
			DllGetFunction(hGDIPlus, GdipDeleteGraphics);
			DllGetFunction(hGDIPlus, GdipDrawImageRectRectI);
			DllGetFunction(hGDIPlus, GdipCreateBitmapFromScan0);
			DllGetFunction(hGDIPlus, GdipFillRectangleI);
			DllGetFunction(hGDIPlus, GdipCreateSolidFill);
			DllGetFunction(hGDIPlus, GdipDeleteBrush);
			DllGetFunction(hGDIPlus, GdipCloneBitmapAreaI);
			DllGetFunction(hGDIPlus, GdipSetImagePalette);

			//if (!GdipCreateBitmapFromFileICM && !GdipCreateBitmapFromStreamICM)
			//	bUseICM = false;

			if (GdiplusStartup && GdiplusShutdown && GdipCreateBitmapFromFile && GdipGetImageWidth && GdipGetImageHeight && GdipGetImagePixelFormat && GdipBitmapLockBits && GdipBitmapUnlockBits && GdipDisposeImage && GdipImageGetFrameCount && GdipImageSelectActiveFrame 
				&& GdipGetPropertyItemSize && GdipGetPropertyItem && GdipGetImageFlags
				&& GdipGetImagePalette && GdipGetImagePaletteSize && GdipCloneBitmapAreaI
				&& GdipCreateFromHDC && GdipDeleteGraphics && GdipDrawImageRectRectI
				&& GdipCreateBitmapFromScan0 && GdipFillRectangleI && GdipCreateSolidFill && GdipDeleteBrush
				&& GdipSetImagePalette)
			{
				Gdiplus::GdiplusStartupInput gdiplusStartupInput;
				Gdiplus::Status lRc = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
				result = (lRc == Gdiplus::Ok);
				if (!result) {
					nLastError = GetLastError();
					GdiplusShutdown(gdiplusToken); bTokenInitialized = false;
					nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
				} else {
					bTokenInitialized = true;
				}
			} else {
				nErrNumber = PGE_FUNCTION_NOT_FOUND;
			}
			if (!result)
				FreeLibrary(hGDIPlus);
		}
		if (result)
			pInit->pContext = this;
		return result;
	};

	void Close()
	{
		if (bTokenInitialized) {
			GdiplusShutdown(gdiplusToken);
			bTokenInitialized = false;
		}

		if (hGDIPlus) {
			FreeLibrary(hGDIPlus);
			hGDIPlus = NULL;
		}

		if (bCoInitialized) {
			bCoInitialized = FALSE;
			CoUninitialize();
		}
		
		FREE(this);
	};
};

#define DEFINE_GUID_(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID DECLSPEC_SELECTANY name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID_(FrameDimensionTime, 0x6aedbd6d,0x3fb5,0x418a,0x83,0xa6,0x7f,0x45,0x22,0x9d,0xc8,0x72);
DEFINE_GUID_(FrameDimensionPage, 0x7462dc86,0x6180,0x4c7e,0x8e,0x3f,0xee,0x73,0x33,0xa7,0xa4,0x83);

struct GDIPlusImage;

struct GDIPlusData
{
	DWORD nMagic;
	HDC hCompDc1;
	HBITMAP hDIB, hOld1;
	wchar_t szInfo[255];
	
	GDIPlusData() {
		nMagic = eGdiStr_Bits;
	};
	
	void Close() {
		if (hCompDc1 && hOld1)
			SelectObject(hCompDc1, hOld1);
		DeleteObject(hDIB);
		DeleteDC(hCompDc1);
		FREE(this);
	};
};

struct GDIPlusImage
{
	DWORD nMagic;
	
#ifdef _DEBUG
	wchar_t szFileName[MAX_PATH];
#endif
	GDIPlusDecoder *gdi;
	Gdiplus::GpBitmap *img;
	HRESULT nErrNumber, nLastError;
	//
	UINT lWidth, lHeight, pf, nBPP, nPages, /*lFrameTime,*/ nActivePage, nTransparent, nImgFlags;
	bool Animation;
	wchar_t FormatName[0x80];
	
	GDIPlusImage() {
		nMagic = eGdiStr_Image;
	};


	Gdiplus::GpBitmap* OpenBitmapFromFile(const wchar_t *pFileName)
	{
		Gdiplus::Status lRc = Gdiplus::Ok;
		Gdiplus::GpBitmap *img = NULL;

		lRc = gdi->GdipCreateBitmapFromFile(pFileName, &img);

		if (!img) {
			nLastError = GetLastError();
			nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
		}
		
		return img;
	}

	bool Open(const wchar_t *pFileName)
	{
		_ASSERTE(img == NULL);
		_ASSERTE(gdi != NULL);

		bool result = false;
		Gdiplus::Status lRc;

		nActivePage = -1; nTransparent = -1; nImgFlags = 0;
		img = OpenBitmapFromFile(pFileName);

		if (!img) {
			//nErrNumber = gdi->nErrNumber; -- ������ ��� � nErrNumber

		} else {
			lRc = gdi->GdipGetImageWidth(img, &lWidth);
			lRc = gdi->GdipGetImageHeight(img, &lHeight);
			lRc = gdi->GdipGetImagePixelFormat(img, (Gdiplus::PixelFormat*)&pf);
			nBPP = pf >> 8 & 0xFF;

			lRc = gdi->GdipGetImageFlags(img, &nImgFlags);
			

			Animation = false; nPages = 1;
			if (!(lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionTime, &nPages)))
				Animation = nPages > 1;
			else
				if ((lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionPage, &nPages)))
					nPages = 1;

			FormatName[0] = 0;
			if (gdi->GdipGetImageRawFormat)
			{
				const wchar_t Format[][5] = {L"BMP", L"EMF", L"WMF", L"JPEG", L"PNG", L"GIF", L"TIFF", L"EXIF", L"", L"", L"ICO"};
				GUID gformat;
				if (!(lRc = gdi->GdipGetImageRawFormat(img, &gformat))) {
					// DEFINE_GUID(ImageFormatUndefined, 0xb96b3ca9,0x0728,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
					// DEFINE_GUID(ImageFormatMemoryBMP, 0xb96b3caa,0x0728,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);

					if (gformat.Data1 >= 0xB96B3CAB && gformat.Data1 <= 0xB96B3CB5) {
						lstrcpy(FormatName, Format[gformat.Data1 - 0xB96B3CAB]);
					}
				}
			}

			result = SelectPage(0);
		}
		return result;
	};
	bool SelectPage(UINT iPage)
	{
		bool result = false;
		Gdiplus::Status lRc;

		if ((lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionTime, &pf)))
			lRc = gdi->GdipImageSelectActiveFrame(img, &FrameDimensionPage, iPage);
		else
			lRc = gdi->GdipImageSelectActiveFrame(img, &FrameDimensionTime, iPage);
		lRc = gdi->GdipGetImageWidth(img, &lWidth);
		lRc = gdi->GdipGetImageHeight(img, &lHeight);
		lRc = gdi->GdipGetImagePixelFormat(img, (Gdiplus::PixelFormat*)&pf);

		nBPP = pf >> 8 & 0xFF;

		nActivePage = iPage;
		result = true;

		return result;
	};
	void Close()
	{
		if (!gdi) return;

		Gdiplus::Status lRc;
		
		if (img) {
			lRc = gdi->GdipDisposeImage(img);
			img = NULL;
		}

		FREE(this);
	};
	static BOOL CALLBACK DrawImageAbortCallback(GDIPlusDecoder *pGDI)
	{
		if (pGDI)
			return pGDI->bCancelled;
		return FALSE;
	};
	bool GetPageBits(CET_LoadInfo *pDecodeInfo)
	{
		bool result = false;
		
		if (!lWidth || !lHeight) {
			pDecodeInfo->nErrNumber = PGE_INVALID_IMGSIZE;
			return false;
		}
		

		GDIPlusData *pData = (GDIPlusData*)CALLOC(sizeof(GDIPlusData));
		if (!pData)
		{
			pDecodeInfo->nErrNumber = PGE_NOT_ENOUGH_MEMORY;
		}
		else
		{
			pData->nMagic = eGdiStr_Bits;
		
			int nCanvasWidth  = pDecodeInfo->crLoadSize.X;
			int nCanvasWidthS = nCanvasWidth; //((nCanvasWidth+7) >> 3) << 3; // try to align x8 pixels
			int nCanvasHeight = pDecodeInfo->crLoadSize.Y;
			int nShowWidth, nShowHeight;
			//double fAspectX = (double)nCanvasWidth / (double)lWidth;
			//double fAspectY = (double)nCanvasHeight / (double)lHeight;
			i64 aSrc = (100 * (i64) lWidth / lHeight);
			i64 aCvs = (100 * (i64) nCanvasWidth / nCanvasHeight);
			//if (fAspectX < fAspectY)
			if (aSrc > aCvs)
			{
				//if (fAspectX > pDecodeInfo->nMaxZoom && pDecodeInfo->nMaxZoom > 0)
				//	fAspectX = (double)pDecodeInfo->nMaxZoom;
				if (lWidth >= (UINT)nCanvasWidth) {
					nShowWidth = nCanvasWidth;
					nShowHeight = (int)(((i64)lHeight) * nCanvasWidth / lWidth);
				} else {
					aCvs = 100 * ((i64)nCanvasWidth) / lWidth;
					if (aCvs > pDecodeInfo->nMaxZoom && pDecodeInfo->nMaxZoom > 0) aCvs = pDecodeInfo->nMaxZoom;
					nShowWidth = (int)(lWidth * aCvs / 100);
					nShowHeight = (int)(lHeight * aCvs / 100);
				}
			} else {
				//if (fAspectY > pDecodeInfo->nMaxZoom && pDecodeInfo->nMaxZoom > 0)
				//	fAspectY = (double)pDecodeInfo->nMaxZoom;
				if (lHeight >= (UINT)nCanvasHeight) {
					nShowWidth = (int)(((i64)lWidth) * nCanvasHeight / lHeight);
					nShowHeight = nCanvasHeight;
				} else {
					aCvs = 100 * ((i64)nCanvasHeight) / lHeight;
					if (aCvs > pDecodeInfo->nMaxZoom && pDecodeInfo->nMaxZoom > 0) aCvs = pDecodeInfo->nMaxZoom;
					nShowWidth = (int)(lWidth * aCvs / 100);
					nShowHeight = (int)(lHeight * aCvs / 100);
				}
			}

			
			pData->hCompDc1 = CreateCompatibleDC(NULL);

			BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
			bmi.biWidth = nCanvasWidthS;
			bmi.biHeight = -nCanvasHeight; // To-Down DIB
			bmi.biPlanes = 1;
			bmi.biBitCount = 32;
			bmi.biCompression = BI_RGB;

			LPBYTE pBits = NULL;
			pData->hDIB = CreateDIBSection(pData->hCompDc1, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
			_ASSERTE(pData->hDIB);

			pData->hOld1 = (HBITMAP)SelectObject(pData->hCompDc1, pData->hDIB);
			
			RECT rcFull = {0,0,nCanvasWidthS, nCanvasHeight};
			HBRUSH hBr = CreateSolidBrush(pDecodeInfo->crBackground);
			FillRect(pData->hCompDc1, &rcFull, hBr);
			DeleteObject(hBr);

			Gdiplus::GpGraphics *pGr = NULL;
			Gdiplus::Status stat = gdi->GdipCreateFromHDC(pData->hCompDc1, &pGr);
			if (!stat) {
				int x = (nCanvasWidth-nShowWidth)>>1;
				int y = (nCanvasHeight-nShowHeight)>>1;
				stat = gdi->GdipDrawImageRectRectI(
					pGr, img,
					x, y, nShowWidth, nShowHeight,
					0,0,lWidth,lHeight,
					Gdiplus::UnitPixel, NULL, //NULL, NULL);
					(Gdiplus::DrawImageAbort)DrawImageAbortCallback, gdi);
				gdi->GdipDeleteGraphics(pGr);
			}
	
			if (stat) {
				pDecodeInfo->nErrNumber = PGE_BITBLT_FAILED;
			} else {
				result = true;
				
				wsprintf(pData->szInfo, L"%i x %i x %ibpp", lWidth, lHeight, nBPP);
				if (nPages > 1) wsprintf(pData->szInfo+lstrlen(pData->szInfo), L" [%i]", nPages);
				if (FormatName) {
					lstrcat(pData->szInfo, L" ");
					lstrcat(pData->szInfo, FormatName);
				}
				
				pDecodeInfo->pFileContext = (LPVOID)pData;
				pDecodeInfo->crSize.X = nCanvasWidth; pDecodeInfo->crSize.Y = nCanvasHeight;
				pDecodeInfo->cbStride = nCanvasWidthS * 4;
				pDecodeInfo->nBits = 32;
				pDecodeInfo->ColorModel = CET_CM_BGR;
				pDecodeInfo->pszComments = pData->szInfo;
				pDecodeInfo->cbPixelsSize = pDecodeInfo->cbStride * nCanvasHeight;
				pDecodeInfo->Pixels = (const DWORD*)pBits;
			}

			if (!result) {
				pData->Close();
			}
		}
	

		return result;
	};
};





BOOL WINAPI CET_Init(struct CET_Init* pInit)
{
	_ASSERTE(pInit->cbSize >= sizeof(struct CET_Init));
	if (pInit->cbSize < sizeof(struct CET_Init)) {
		pInit->nErrNumber = PGE_OLD_PICVIEW;
		return FALSE;
	}

	ghModule = pInit->hModule;

	GDIPlusDecoder *pDecoder = (GDIPlusDecoder*) CALLOC(sizeof(GDIPlusDecoder));
	if (!pDecoder) {
		pInit->nErrNumber = PGE_NOT_ENOUGH_MEMORY;
		return FALSE;
	}
	pDecoder->nMagic = eGdiStr_Decoder;
	if (!pDecoder->Init(pInit)) {
		pInit->nErrNumber = pDecoder->nErrNumber;
		pDecoder->Close();
		FREE(pDecoder);
		return FALSE;
	}

	_ASSERTE(pInit->pContext == (LPVOID)pDecoder); // ��� ������ pDecoder->Init
	return TRUE;
}

VOID WINAPI CET_Done(struct CET_Init* pInit)
{
	if (pInit) {
		GDIPlusDecoder *pDecoder = (GDIPlusDecoder*)pInit->pContext;
		if (pDecoder) {
			_ASSERTE(pDecoder->nMagic == eGdiStr_Decoder);
			if (pDecoder->nMagic == eGdiStr_Decoder) {
				pDecoder->Close();
				FREE(pDecoder);
			}
		}
	}
}


#define SETERROR(n) if (pLoadPreview) pLoadPreview->nErrNumber = n;


BOOL WINAPI CET_Load(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo)) {
		_ASSERTE(*((LPDWORD)pLoadPreview) == sizeof(struct CET_LoadInfo));
		SETERROR(PGE_INVALID_VERSION);
		return FALSE;
	}
	
	
	if (!pLoadPreview->pContext) {
		SETERROR(PGE_INVALID_CONTEXT);
		return FALSE;
	}
	
	TODO("LoadFromStream ���� �����");
	if (pLoadPreview->bVirtualItem || !pLoadPreview->pFileData) {
		SETERROR(PGE_FILE_NOT_FOUND);
		return FALSE;
	}
	
	if (pLoadPreview->nFileSize < 16 || pLoadPreview->nFileSize > 209715200/*200 MB*/) {
		SETERROR(PGE_UNSUPPORTEDFORMAT);
		return FALSE;
	}
	
	BOOL lbKnown = FALSE;
	
	if (pLoadPreview->pFileData) {
		const BYTE  *pb  = (const BYTE*)pLoadPreview->pFileData;
		const WORD  *pw  = (const WORD*)pLoadPreview->pFileData;
		const DWORD *pdw = (const DWORD*)pLoadPreview->pFileData;
		
		TODO("ICO - ������. ����� ������������������ ����������")
		
		if (*pdw==0x474E5089 /* �PNG */)
			lbKnown = TRUE;
		else if (*pw==0x4D42 /* BM */)
			lbKnown = TRUE;
		else if (pb[0]==0xFF && pb[1]==0xD8 && pb[2]==0xFF) // JPEG?
			lbKnown = TRUE;
		else if (pw[0]==0x4949) // TIFF?
			lbKnown = TRUE;
		else if (pw[0]==0 && (pw[1]==1/*ICON*/ || pw[1]==2/*CURSOR*/) && (pw[2]>0 && pw[2]<=64/*IMG COUNT*/)) // .ico, .cur
			lbKnown = TRUE;
		else if (*pdw == 0x38464947 /*GIF8*/)
			lbKnown = TRUE;
		else
		{
			const wchar_t* pszFile = NULL;
			wchar_t szExt[6]; szExt[0] = 0;
			pszFile = wcsrchr(pLoadPreview->sFileName, L'\\');
			if (pszFile) {
				pszFile = wcsrchr(pszFile, L'.');
				if (pszFile) {
					int nLen = lstrlenW(pszFile);
					if (nLen && nLen<5) {
						lstrcpyW(szExt, pszFile+1);
						CharLowerBuffW(szExt, nLen-1);
					}
				}
			}
		
			if ((szExt[0]==L'w' || szExt[0]==L'e') && szExt[1]==L'm' && szExt[2]==L'f')
				lbKnown = TRUE;
		}
	}
	if (!lbKnown) {
		SETERROR(PGE_UNSUPPORTEDFORMAT);
		return FALSE;
	}

	
	GDIPlusImage *pImage = (GDIPlusImage*)CALLOC(sizeof(GDIPlusImage));
	if (!pImage) {
		SETERROR(PGE_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	pImage->nMagic = eGdiStr_Image;
	pLoadPreview->pFileContext = (void*)pImage;

	
	pImage->gdi = (GDIPlusDecoder*)pLoadPreview->pContext;
	pImage->gdi->bCancelled = FALSE;
	

	if (!pImage->Open(pLoadPreview->sFileName)) {
		SETERROR(pImage->nErrNumber);
		pImage->Close();
		return FALSE;
	}
	
	if (pLoadPreview) {
		if (!pImage->GetPageBits(pLoadPreview)) {
			SETERROR(pImage->nErrNumber);
			pImage->Close();
			return FALSE;
		}
	}
	
	if (pLoadPreview->pFileContext == (void*)pImage)
		pLoadPreview->pFileContext = NULL;
	
	pImage->Close();
	return TRUE;
}

VOID WINAPI CET_Free(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo)) {
		_ASSERTE(*((LPDWORD)pLoadPreview) == sizeof(struct CET_LoadInfo));
		SETERROR(PGE_INVALID_VERSION);
		return;
	}
	if (!pLoadPreview->pFileContext) {
		SETERROR(PGE_INVALID_CONTEXT);
		return;
	}

	switch (*(LPDWORD)pLoadPreview->pFileContext) {
		case eGdiStr_Image: {
			// ���� �� ������� ���� ��� exception � CET_Load
			GDIPlusImage *pImg = (GDIPlusImage*)pLoadPreview->pFileContext;
			pImg->Close();
		} break;
		case eGdiStr_Bits: {
			GDIPlusData *pData = (GDIPlusData*)pLoadPreview->pFileContext;
			pData->Close();
		} break;
		
		#ifdef _DEBUG
		default:
			_ASSERTE(*(LPDWORD)pLoadPreview->pFileContext == eGdiStr_Bits);
		#endif
	}
}

VOID WINAPI CET_Cancel(LPVOID pContext)
{
	if (!pContext) return;
	GDIPlusDecoder *pDecoder = (GDIPlusDecoder*)pContext;
	if (pDecoder->nMagic == eGdiStr_Decoder) {
		pDecoder->bCancelled = TRUE;
	}
}
