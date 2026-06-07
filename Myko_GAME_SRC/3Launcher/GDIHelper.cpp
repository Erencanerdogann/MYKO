
#include "stdafx.h"
//#include "Engine.h"
#include "GDIHelper.h"

extern GDIHelper gdiHelper;
extern HWND mainWindow;
HWND GDIHelper::staticControl = NULL;
HWND GDIHelper::MainHWNDControl = NULL;
Image* GDIHelper::m_pImage = NULL;
GUID* GDIHelper::m_pDimensionIDs = NULL;
UINT GDIHelper::m_FrameCount = 0;
PropertyItem* GDIHelper::m_pItem = NULL;
UINT GDIHelper::m_iCurrentFrame = 0;
UINT_PTR GDIHelper::unique_id = 0;
BOOL GDIHelper::m_bIsPlaying = FALSE;
BOOL GDIHelper::isPlayable = FALSE;
BOOL GDIHelper::isLooped = FALSE;
int GDIHelper::xPosition = 0;
int GDIHelper::yPosition = 0;
int GDIHelper::width = 0;
int GDIHelper::height = 0;
int GDIHelper::animation_duration = 0;

/** GDIHelper is a class helper to display images and animated GIF **/
GDIHelper::GDIHelper() {}

/** Function to destroy objects and arrays, call this function on WM_DESTROY of WinProc. **/
void GDIHelper::Destroy() {
    // S114: NULL setle ki ikinci SetupBanner cagrida double-free olmasin
    if(m_pDimensionIDs) { delete[] m_pDimensionIDs; m_pDimensionIDs = NULL; }
    if(m_pItem)         { free(m_pItem); m_pItem = NULL; }
    if(m_pImage)        { delete m_pImage; m_pImage = NULL; }
    m_bIsPlaying = FALSE;
    isPlayable = FALSE;
    if (staticControl) {
        RemoveWindowSubclass(staticControl, &StaticControlProc, unique_id);
        staticControl = NULL;
    }
}

extern HWND hLoadHwnd;
// S114: GIF bitince ne yapilacak (0=loop devam, 1=KO basla+exit, 2=sadece exit pencere kapan)
int g_gifEndAction = 1;

/** Private function, call this function as thread to animate the GIF image. **/
void GDIHelper::run()
{
    if(m_bIsPlaying == TRUE) {
        return;
    }
    // TODO#241 FIX-G (S127 v3.5): GIF crash guard — GDIHelper::run() use-after-free + OOB.
    // KOK (MATRIX crash analiz: launcher.exe+0xb514 0xc0000005 ACCESS_VIOLATION x5):
    //   (1) m_pItem/m_pImage NULL veya Destroy() ile free edilmis olabilir -> NULL deref
    //   (2) ((UINT*)m_pItem[0].value)[m_iCurrentFrame] frame index dizi siniri disi -> OOB read
    //   (3) m_FrameCount==0 -> '% m_FrameCount' DIV-BY-ZERO
    //   (4) sleep sirasinda baska thread Destroy() -> m_pImage delete -> sleep sonrasi
    //       SelectActiveFrame() silinmis pointer -> ACCESS_VIOLATION (use-after-free RACE)
    // FIX: her erisimden ONCE NULL + isPlayable + m_FrameCount guard; sleep SONRASI tekrar kontrol.
    if (m_pImage == NULL || m_pItem == NULL || m_FrameCount == 0) {
        return; // GIF init bozuk -> oynatma, crash etme
    }

    m_iCurrentFrame = 0;
    GUID Guid = FrameDimensionTime;
    m_pImage->SelectActiveFrame(&Guid, m_iCurrentFrame);
    ++m_iCurrentFrame;
    m_bIsPlaying = TRUE;
    // Frame delay oku — index m_FrameCount sinirinda tut (OOB read engelle)
    if (m_iCurrentFrame < m_FrameCount && m_pItem[0].value != NULL)
        animation_duration = ((UINT*)m_pItem[0].value)[m_iCurrentFrame] * 10;
    else
        animation_duration = 100; // guvenli default (10 FPS)

    while(isPlayable) {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_duration));

        // SLEEP SONRASI TEKRAR GUARD: sleep sirasinda Destroy() cagrilmis olabilir
        // (isPlayable=FALSE + m_pImage/m_pItem free). Silinmis pointer'a DOKUNMA.
        if (!isPlayable || m_pImage == NULL || m_pItem == NULL || m_FrameCount == 0)
            break;

        m_pImage->SelectActiveFrame(&Guid, m_iCurrentFrame);

        // Frame ilerlet — UB'siz (++ ayri ifade) + modulo guvenli (m_FrameCount>0 yukarida garanti)
        m_iCurrentFrame = (m_iCurrentFrame + 1) % m_FrameCount;
        InvalidateRect(staticControl, NULL, FALSE);
        UpdateWindow(staticControl);

        // Bir sonraki frame'in delay'ini guvenli oku (OOB engelle)
        if (m_iCurrentFrame < m_FrameCount && m_pItem[0].value != NULL)
            animation_duration = ((UINT*)m_pItem[0].value)[m_iCurrentFrame] * 10;

        if (!isLooped && m_iCurrentFrame == 0)
        {
            // S114: GIF 1 dongu bitti, sadece pencereye PostQuit gonder.
            // KO baslatma / Launcher kapatma SetupBanner cagrisinda yapilir (StartClick'te).
            ::PostMessage(hLoadHwnd, WM_CLOSE, 0, 0);
            break;
        }
    }
}

/** Private function, accessible only in this class, check if file exist. **/
bool GDIHelper::IsFileExist(string file_name) {
    struct stat buffer;
    return (stat(file_name.c_str(), &buffer) == 0);
}

/** Private function, function to count and get the frame of image. **/
void GDIHelper::GetImageFrame() {
    UINT count = m_pImage->GetFrameDimensionsCount();
    m_pDimensionIDs = new GUID[count];
    m_pImage->GetFrameDimensionsList(m_pDimensionIDs, count);

    m_FrameCount = m_pImage->GetFrameCount(&m_pDimensionIDs[0]);

    UINT TotalBuffer = m_pImage->GetPropertyItemSize(PropertyTagFrameDelay);
    m_pItem = (PropertyItem*)malloc(TotalBuffer);
    m_pImage->GetPropertyItem(PropertyTagFrameDelay, TotalBuffer, m_pItem);

    if(m_FrameCount > 1) {  // frame of GIF is more than one, all good, we don't want the error of `Access violation reading location`
        OutputDebugString(_T("NOTICED: GDIHelper::InitializeImage >> Image file has more than 1 frame, its playable (2).\n"));
        isPlayable = TRUE;  // is playable
    
        std::thread t(run); // Start the animation as thread.
        t.detach();         // this will be non-blocking thread.
    }
}

LRESULT CALLBACK GDIHelper::StaticControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg)
    {
    case WM_PAINT: 
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        Graphics g(hdc);
        g.DrawImage(m_pImage, 0, 0, width, height);

        EndPaint(hwnd, &ps);
        return TRUE;
    }
    break;
    case WM_LBUTTONDBLCLK:
    {
        int a = 0;
    }
    break;
    }
   return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

/** Function to set the image and initialize all required variables. **/
void GDIHelper::SetImage(int uunique_id, int xxPosition, int yyPosition, int wwidth, int hheight, Image* image, HWND hwnd) {
    unique_id = uunique_id;
    xPosition = xxPosition;
    yPosition = yyPosition;
    width = wwidth;
    height = hheight;

    staticControl = CreateWindowEx(0, "STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, xPosition, yPosition, width, height, hwnd, NULL, NULL, NULL); //create the static control.
    SetWindowSubclass(staticControl, &StaticControlProc, unique_id, 0);

    m_pImage = image;
    GetImageFrame(); //Initialize the image.
}

/** Function to Display Image from Local File. **/
void GDIHelper::DisplayImageFromFile(string file_name, HWND hWnd, UINT_PTR uunique_id, int xxPosition, int yyPosition, int wwidth, int hheight, bool looped) {
    if(!IsFileExist(file_name)) {
        OutputDebugString(_T("ERROR: GDIHelper::LoadImageFromFile >> Invalid file or not exist\n"));
        return; 
    }

    isLooped = looped;
    std::wstring widestr = std::wstring(file_name.begin(), file_name.end()); // Convert the string file_name to wstring.
    SetImage(uunique_id, xxPosition, yyPosition, wwidth, hheight, Image::FromFile(widestr.c_str()), hWnd); //Set image and Control
}


/** Function to Display Image from Local File. **/
void GDIHelper::thyke_display(string file_name, HWND hWndKO, HWND &hWnd, UINT_PTR uunique_id, int xxPosition, int yyPosition, int wwidth, int hheight, bool looped) {
    if(!IsFileExist(file_name)) {
        OutputDebugString(_T("ERROR: GDIHelper::LoadImageFromFile >> Invalid file or not exist\n"));
        return; 
    }

    isLooped = looped;
    std::wstring widestr = std::wstring(file_name.begin(), file_name.end()); // Convert the string file_name to wstring.
   
    unique_id = uunique_id;
    xPosition = xxPosition;
    yPosition = yyPosition;
    width = wwidth;
    height = hheight;

    staticControl = CreateWindowEx(0, "STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, xPosition, yPosition, width, height, hWndKO, NULL, NULL, NULL); //create the static control.
    hWnd = staticControl;
    SetWindowSubclass(staticControl, &StaticControlProc, unique_id, 0);

    m_pImage = Image::FromFile(widestr.c_str());
    GetImageFrame(); //Initialize the image.
}

/** Function to Load Image from Resources. **/
void GDIHelper::DisplayImageFromResource(HMODULE hMod, const wchar_t* resid, const wchar_t* restype, HWND hWnd, UINT_PTR uunique_id, int xxPosition, int yyPosition, int wwidth, int hheight, bool looped) {

    isLooped = looped;
    IStream* pStream = nullptr;
    Gdiplus::Bitmap* pBmp = nullptr;
    HGLOBAL hGlobal = nullptr;

    HRSRC hrsrc = FindResourceW(GetModuleHandle(NULL), resid, restype);     // get the handle to the resource
    if(hrsrc) {
        DWORD dwResourceSize = SizeofResource(hMod, hrsrc);
        if(dwResourceSize > 0) {
            HGLOBAL hGlobalResource = LoadResource(hMod, hrsrc); // load it
            if(hGlobalResource) {
                void* imagebytes = LockResource(hGlobalResource); // get a pointer to the file bytes

                hGlobal = ::GlobalAlloc(GHND, dwResourceSize); // copy image bytes into a real hglobal memory handle
                if(hGlobal) {
                    void* pBuffer = ::GlobalLock(hGlobal);
                    if(pBuffer) {
                        memcpy(pBuffer, imagebytes, dwResourceSize);
                        HRESULT hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pStream);
                        if(SUCCEEDED(hr)) {
                            hGlobal = nullptr; // pStream now owns the global handle and will invoke GlobalFree on release
                            pBmp = new Gdiplus::Bitmap(pStream);
                        }
                    }
                }
            }
        }
    }

    if(pStream) {
        pStream->Release();
        pStream = nullptr;
    }

    if(hGlobal) {
        GlobalFree(hGlobal);
    }

    SetImage(uunique_id, xxPosition, yyPosition, wwidth, hheight, pBmp, hWnd);
}
