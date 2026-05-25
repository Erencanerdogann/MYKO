#define DIRECTINPUT_VERSION 0x0800
#include "stdafx.h"
#include <tlhelp32.h>
#include "resource.h"
#include <d3d9.h>
#include <d3dx9core.h>
#include <dinput.h>
#include "WinUser.h"
#include <tchar.h>
#include "LauncherEngine.h"
#include "hdr.h"
#include "CRC.h"
#include <thread>
#include "MD5.h"
#include "sha.hpp"
#include <dwmapi.h>
#include "gif.h"
#include "GDIHelper.h"
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma warning(disable:4244)
#pragma warning(disable:4129)
static int iW = 802;
static int iH = 594;
static int iWOff = 0;
CheckSum Check;
MD5	md5;

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};


thyke_Test* thyke_t = NULL;
// Font
LPD3DXFONT m_font = NULL;
RECT TextStatePos;
RECT TextNoticePos;
RECT TextPagePos;
int NoticeOffsetY = 0;
int TextStateBaseX = 0;

// Textures
IDirect3DTexture9* LauncherBackgroundTexture = NULL;

IDirect3DTexture9* StartButtonTexture = NULL;
IDirect3DTexture9* StartButtonHoverTexture = NULL;
IDirect3DTexture9* StartButtonDownTexture = NULL;

IDirect3DTexture9* HomePageButtonTexture = NULL;
IDirect3DTexture9* HomePageButtonHoverTexture = NULL;
IDirect3DTexture9* HomePageButtonDownTexture = NULL;

IDirect3DTexture9* SettingsButtonTexture = NULL;
IDirect3DTexture9* SettingsButtonDownTexture = NULL;
IDirect3DTexture9* SettingsButtonHoverTexture = NULL;

IDirect3DTexture9* CloseButtonTexture = NULL;
IDirect3DTexture9* CloseButtonHoverTexture = NULL;
IDirect3DTexture9* CloseButtonDownTexture = NULL;

IDirect3DTexture9* DiscordButtonTexture = NULL;
IDirect3DTexture9* DiscordButtonHoverTexture = NULL;
IDirect3DTexture9* DiscordButtonDownTexture = NULL;
//
IDirect3DTexture9* ForumButtonTexture = NULL;
IDirect3DTexture9* ForumButtonHoverTexture = NULL;
IDirect3DTexture9* ForumButtonDownTexture = NULL;
//
IDirect3DTexture9* FacebookButtonTexture = NULL;
IDirect3DTexture9* FacebookButtonHoverTexture = NULL;
IDirect3DTexture9* FacebookButtonDownTexture = NULL;

IDirect3DTexture9* ProgressTexture = NULL;
IDirect3DTexture9* ProgressFillTexture = NULL;
// S113: Compact butonu
IDirect3DTexture9* CompactButtonTexture = NULL;
IDirect3DTexture9* CompactButtonDownTexture = NULL;
IDirect3DTexture9* CompactButtonHoverTexture = NULL;
// S113: Register butonu
IDirect3DTexture9* RegisterButtonTexture = NULL;
IDirect3DTexture9* RegisterButtonDownTexture = NULL;
IDirect3DTexture9* RegisterButtonHoverTexture = NULL;
// S113: Repair butonu (patch reset)
IDirect3DTexture9* RepairButtonTexture = NULL;
IDirect3DTexture9* RepairButtonDownTexture = NULL;
IDirect3DTexture9* RepairButtonHoverTexture = NULL;


// Sprites
LPD3DXSPRITE LauncherSprite = NULL;

// Vectors
D3DXVECTOR3 LauncherBackgorundPosition(0, 0, 0);
D3DXVECTOR3 StartButtonPosition(608.0f + iWOff, 509.0f, 0);
D3DXVECTOR3 HomePageButtonPosition(23.0f + iWOff, 511.0f, 0);
D3DXVECTOR3 SettingsButtonPosition(161.0f + iWOff, 511.0f, 0);
D3DXVECTOR3 CloseButtonPosition(765.0f + iWOff, 3.0f, 0);
D3DXVECTOR3 DiscordButtonPosition(608.0f + iWOff, 509.0f, 0);
D3DXVECTOR3 ForumButtonPosition(23.0f + iWOff, 511.0f, 0);
D3DXVECTOR3 FacebookButtonPosition(161.0f + iWOff, 511.0f, 0);
D3DXVECTOR3 ProgressPosition(24.0f + iWOff, 557.0f, 0);
D3DXVECTOR3 CompactButtonPosition(900.0f + iWOff, 79.0f, 0);
D3DXVECTOR3 RegisterButtonPosition(618.0f + iWOff, 86.0f, 0);
D3DXVECTOR3 RepairButtonPosition(800.0f + iWOff, 40.0f, 0);
RECT pbFill;

// Surfaces
D3DSURFACE_DESC StartButtonSurface;
D3DSURFACE_DESC HomePageButtonSurface;
D3DSURFACE_DESC SettingsButtonSurface;
D3DSURFACE_DESC CloseButtonSurface;
D3DSURFACE_DESC CompactButtonSurface;
D3DSURFACE_DESC RegisterButtonSurface;
D3DSURFACE_DESC RepairButtonSurface;
int g_RepairHitX = 0, g_RepairHitY = 0, g_RepairHitW = 0, g_RepairHitH = 0;
// S113: Yeni tasarim — full-frame sprite + ayri hittest bolgesi (UIXSettings HIT_X/HIT_Y/HIT_W/HIT_H)
int g_StartHitX = 0, g_StartHitY = 0, g_StartHitW = 0, g_StartHitH = 0;
int g_SettingsHitX = 0, g_SettingsHitY = 0, g_SettingsHitW = 0, g_SettingsHitH = 0;
int g_CloseHitX = 0, g_CloseHitY = 0, g_CloseHitW = 0, g_CloseHitH = 0;
// S113: Web link URL'leri (UIXSettings.ini'den yuklenir)
char g_HomepageURL[512] = {0};
char g_ForumURL[512] = {0};
char g_DiscordURL[512] = {0};
char g_FacebookURL[512] = {0};
char g_RegisterURL[512] = {0};
D3DSURFACE_DESC DiscordButtonSurface;
D3DSURFACE_DESC ForumButtonSurface;
D3DSURFACE_DESC FacebookButtonSurface;
D3DSURFACE_DESC ProgressSurface;
// States
enum ButtonState
{
    STATE_NORMAL = 0,
    STATE_HOVER,
    STATE_DOWN,
    STATE_UP
};
// Statics
#define COL_N D3DCOLOR_ARGB(255, 200, 200, 200)
#define COL_H D3DCOLOR_ARGB(255, 255, 255, 255)
#define COL_D D3DCOLOR_ARGB(255, 150, 150, 150)
#define COL_X D3DCOLOR_ARGB(255, 80, 80, 80)
ButtonState states[10] = {};
static ButtonState lastStartState = STATE_NORMAL;
static ButtonState lastHomepageState = STATE_NORMAL;
static ButtonState lastSettingsState = STATE_NORMAL;
static ButtonState lastCloseState = STATE_NORMAL;
static ButtonState lastCompactState = STATE_NORMAL;
static ButtonState lastRegisterState = STATE_NORMAL;
static ButtonState lastRepairState = STATE_NORMAL;
static ButtonState lastDiscordState = STATE_NORMAL;
static ButtonState lastForumState = STATE_NORMAL;
static ButtonState lastFacebookState = STATE_NORMAL;

static HCURSOR hCursorNormal;
static HCURSOR hCursorHand;
static HCURSOR hCursorClick;
HWND mainWindow;
HMODULE mainInstance;
size_t sayfalar;
size_t sayfa;

// Engine
Launcher* Engine;
int loadCount = 0;
std::string launcherdir;
//std::string Address_HomePage;
//std::string Address_Discord;
//std::string Address_Forum;
//std::string Address_Facebook;
CHAR WP[MAXCHAR];

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();

static void _string_format(const std::string fmt, std::string* result, va_list args)
{
    char buffer[1024];
    _vsnprintf(buffer, sizeof(buffer), fmt.c_str(), args);
    *result = buffer;
}

static std::string string_format(const std::string fmt, ...)
{
    std::string result;
    va_list ap;

    va_start(ap, fmt);
    _string_format(fmt, &result, ap);
    va_end(ap);

    return result;
}

extern std::string hashfile(const std::string filename)
{
    SHA1 checksum;
    return checksum.from_file(filename);
}

std::string GetDLLDir()
{
    char result[MAX_PATH];
    return std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
}

std::string GetDir()
{
    std::string dir = GetDLLDir();
   dir.replace(dir.size() - 13, 13, "");
    return dir;
}

std::string FileDirGet(std::string FileName)
{
    std::string x;
    std::string name = xorstr("%s/%s");
    x = string_format(name, GetDir().c_str(), FileName.c_str());
    return x;
}
void CodeGuardEncrypt()
{
  
}

void CodeGuardEncryptt()
{
   
}

void WebLinkAdded()
{
    //Address_HomePage = "https://mykozone.com/";
    //Address_Forum = "https://forum.mykozone.com/";
    //Address_Facebook = "https://www.facebook.com/mykozone1098/";
    //Address_Discord = "https://discord.gg/mykozone";
}

int WINAPI SocketSystem()
{
    Engine->Start();
    return 1;
}

// dosyalardan
bool LoadTextures()
{
    D3DXCreateSprite(g_pd3dDevice, &LauncherSprite);
    bool textureFail = false;
    std::vector<std::string> unloadedResources;

    HRESULT res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\Background.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &LauncherBackgroundTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\Background.code"));
    }
    // Start
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\StartMouseOut.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &StartButtonTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\StartMouseOut.code"));
    }
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\StartMouseOver.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &StartButtonHoverTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\StartMouseOver.code"));
    }
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\StartMouseClick.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &StartButtonDownTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\StartMouseClick.code"));
    } 
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\OptionsMouseOut.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &SettingsButtonTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\OptionsMouseOut.code"));
    }
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\OptionsMouseOver.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &SettingsButtonHoverTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\OptionsMouseOver.code"));
    }
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\OptionsMouseClick.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &SettingsButtonDownTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\OptionsMouseClick.code"));
    }
    // Close
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\CloseMouseOut.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &CloseButtonTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\CloseMouseOut.code"));
    }
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\CloseMouseOver.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &CloseButtonHoverTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\CloseMouseOver.code"));
    }
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\CloseMouseClick.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &CloseButtonDownTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\CloseMouseClick.code"));
    }
 
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\ProgressEmpty.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &ProgressTexture);
    if (res != D3D_OK) {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\ProgressEmpty.code"));
    }
    res = D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\ProgressValue.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &ProgressFillTexture);
    if (res != D3D_OK)
    {
        textureFail = true;
        unloadedResources.push_back(xorstr("CodeGuard\\Launcher\\ProgressValue.code"));
    }
    // S113: Compact + Home + Forum AKTIF (texture yoksa sessiz atla, gozukmez)
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\CompactMouseOut.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &CompactButtonTexture);
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\CompactMouseOver.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &CompactButtonHoverTexture);
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\CompactMouseClick.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &CompactButtonDownTexture);
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\HomeMouseOut.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &HomePageButtonTexture);
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\HomeMouseOver.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &HomePageButtonHoverTexture);
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\HomeMouseClick.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &HomePageButtonDownTexture);
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\ForumMouseOut.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &ForumButtonTexture);
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\ForumMouseOver.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &ForumButtonHoverTexture);
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\ForumMouseClick.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &ForumButtonDownTexture);
    // S113: Repair butonu (patch reset)
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\RepairMouseOut.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &RepairButtonTexture);
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\RepairMouseOver.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &RepairButtonHoverTexture);
    D3DXCreateTextureFromFileEx(g_pd3dDevice, xorstr("CodeGuard\\Launcher\\RepairMouseClick.code"), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, D3DCOLOR_ARGB(128, 128, 128, 128), 0, 0, &RepairButtonDownTexture);

    launcherdir = GetDir();
    WebLinkAdded();

    if (textureFail)
    {
        std::string fails = "";
        for (std::string i : unloadedResources)
            fails += i + "\n";

        MessageBoxA(mainWindow, std::string(xorstr("Resources couldn't be loaded:\n") + fails).c_str(), "Error", MB_ICONEXCLAMATION);
        return false;
    }

    StartButtonTexture->GetLevelDesc(0, &StartButtonSurface);
    if (HomePageButtonTexture) HomePageButtonTexture->GetLevelDesc(0, &HomePageButtonSurface); else { HomePageButtonSurface.Width = 0; HomePageButtonSurface.Height = 0; }
    SettingsButtonTexture->GetLevelDesc(0, &SettingsButtonSurface);
    if (CompactButtonTexture) CompactButtonTexture->GetLevelDesc(0, &CompactButtonSurface); else { CompactButtonSurface.Width = 0; CompactButtonSurface.Height = 0; }
    CloseButtonTexture->GetLevelDesc(0, &CloseButtonSurface);
    if (DiscordButtonTexture) DiscordButtonTexture->GetLevelDesc(0, &DiscordButtonSurface); else { DiscordButtonSurface.Width = 0; DiscordButtonSurface.Height = 0; }
    if (ForumButtonTexture) ForumButtonTexture->GetLevelDesc(0, &ForumButtonSurface); else { ForumButtonSurface.Width = 0; ForumButtonSurface.Height = 0; }
    if (FacebookButtonTexture) FacebookButtonTexture->GetLevelDesc(0, &FacebookButtonSurface); else { FacebookButtonSurface.Width = 0; FacebookButtonSurface.Height = 0; }
    if (RegisterButtonTexture) RegisterButtonTexture->GetLevelDesc(0, &RegisterButtonSurface); else { RegisterButtonSurface.Width = 0; RegisterButtonSurface.Height = 0; }
    if (RepairButtonTexture) RepairButtonTexture->GetLevelDesc(0, &RepairButtonSurface); else { RepairButtonSurface.Width = 0; RepairButtonSurface.Height = 0; }
    ProgressTexture->GetLevelDesc(0, &ProgressSurface);

    return true;
}

VOID CenterWindow(HWND hwnd, HWND hwndParent, int Width, int Height)
{
    RECT rc;
    if (hwndParent == NULL)
        hwndParent = GetDesktopWindow();

    GetClientRect(hwndParent, &rc);
    MoveWindow(hwnd,(rc.right - rc.left - Width) / 2,(rc.bottom - rc.top - Height) / 2,Width,Height,TRUE);
    return;
}

int GetTextWidth(const char* szText, LPD3DXFONT pFont)
{
    RECT rcRect = { 0,0,0,0 };
    if (pFont)
    {
        pFont->DrawTextA(NULL, szText, strlen(szText), &rcRect, DT_CALCRECT,
            D3DCOLOR_XRGB(0, 0, 0));
    }
    return rcRect.right - rcRect.left;
}

static int lisansTarih[] = { 01, 12, 2023 };
// iki lisans �ekli de ayn� anda �al���r
static std::string ipLisanslari[] = { xorstr("50.114.185.109"), xorstr("50.114.185.109"), xorstr("50.114.185.109") };    //
// x den �ncesine bakar
static std::string subnetLisanlar[] = { xorstr("50.114.185.109") };

bool IsLicensed(std::string ip)
{
    bool ret = false;
    for (std::string pattern : subnetLisanlar)
    {
        const char* tmp = pattern.c_str();
        bool f = true;
        for (size_t i = 0; i < pattern.length(); i++)
            if (tmp[i] != 'x' && tmp[i] != ip.c_str()[i])
                f = false;
        ret = f;
    }

    for (std::string license : ipLisanslari)
        if (ip == license)
            ret = true;

    return ret;
}

HBITMAP hBMP;
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // S113: DPI scaling kapali — yuksek DPI ekranlarda Windows otomatik scaling kapatilir, ham 945x580 cizilir
    SetProcessDPIAware();

    CreateMutexA(0, FALSE, xorstr("Local\\$launcher$"));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return false;

    GetCurrentDirectoryA(MAX_PATH, WP);

    std::string dosyalarNerdeLenAmq = xorstr("\\CodeGuard\\Launcher\\");

    hBMP = (HBITMAP)LoadImage(NULL, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("bg.bmp")).c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    iW = GetPrivateProfileIntA(xorstr("LAUNCHER"), xorstr("WIDTH"), 802, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    iH = GetPrivateProfileIntA(xorstr("LAUNCHER"), xorstr("HEIGHT"), 594, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());

    int tmpX = 0, tmpY = 0, fontSize = 0, fontWeight;
    char InfoFont[25];
    // BG
    tmpX = GetPrivateProfileIntA(xorstr("BG"), xorstr("X"), 0, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("BG"), xorstr("Y"), 0, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    LauncherBackgorundPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    // Start Button
    tmpX = GetPrivateProfileIntA(xorstr("START_BUTTON"), xorstr("X"), 608, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("START_BUTTON"), xorstr("Y"), 509, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    StartButtonPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    // S113: hittest (HIT_X/Y/W/H — yoksa position+surface size kullan, eski uyumlu)
    g_StartHitX = GetPrivateProfileIntA(xorstr("START_BUTTON"), xorstr("HIT_X"), -1, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_StartHitY = GetPrivateProfileIntA(xorstr("START_BUTTON"), xorstr("HIT_Y"), -1, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_StartHitW = GetPrivateProfileIntA(xorstr("START_BUTTON"), xorstr("HIT_W"), 0, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_StartHitH = GetPrivateProfileIntA(xorstr("START_BUTTON"), xorstr("HIT_H"), 0, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());

    tmpX = GetPrivateProfileIntA(xorstr("SETTINGS_BUTTON"), xorstr("X"), 161, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("SETTINGS_BUTTON"), xorstr("Y"), 511, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    SettingsButtonPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    g_SettingsHitX = GetPrivateProfileIntA(xorstr("SETTINGS_BUTTON"), xorstr("HIT_X"), -1, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_SettingsHitY = GetPrivateProfileIntA(xorstr("SETTINGS_BUTTON"), xorstr("HIT_Y"), -1, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_SettingsHitW = GetPrivateProfileIntA(xorstr("SETTINGS_BUTTON"), xorstr("HIT_W"), 0, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_SettingsHitH = GetPrivateProfileIntA(xorstr("SETTINGS_BUTTON"), xorstr("HIT_H"), 0, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());

    // S113: Compact butonu pozisyon (default sag ust, Close butonunun yaninda)
    tmpX = GetPrivateProfileIntA(xorstr("COMPACT_BUTTON"), xorstr("X"), 900, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("COMPACT_BUTTON"), xorstr("Y"), 79, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    CompactButtonPosition = D3DXVECTOR3(tmpX, tmpY, 0);

    // S113: Web link butonlari pozisyon + URL
    tmpX = GetPrivateProfileIntA(xorstr("HOMEPAGE_BUTTON"), xorstr("X"), 20, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("HOMEPAGE_BUTTON"), xorstr("Y"), 393, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    HomePageButtonPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    GetPrivateProfileStringA(xorstr("HOMEPAGE_BUTTON"), xorstr("URL"), "", g_HomepageURL, sizeof(g_HomepageURL), (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());

    tmpX = GetPrivateProfileIntA(xorstr("FORUM_BUTTON"), xorstr("X"), 172, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("FORUM_BUTTON"), xorstr("Y"), 393, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    ForumButtonPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    GetPrivateProfileStringA(xorstr("FORUM_BUTTON"), xorstr("URL"), "", g_ForumURL, sizeof(g_ForumURL), (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());

    tmpX = GetPrivateProfileIntA(xorstr("DISCORD_BUTTON"), xorstr("X"), 320, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("DISCORD_BUTTON"), xorstr("Y"), 393, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    DiscordButtonPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    GetPrivateProfileStringA(xorstr("DISCORD_BUTTON"), xorstr("URL"), "", g_DiscordURL, sizeof(g_DiscordURL), (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());

    tmpX = GetPrivateProfileIntA(xorstr("FACEBOOK_BUTTON"), xorstr("X"), 830, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("FACEBOOK_BUTTON"), xorstr("Y"), 389, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    FacebookButtonPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    GetPrivateProfileStringA(xorstr("FACEBOOK_BUTTON"), xorstr("URL"), "", g_FacebookURL, sizeof(g_FacebookURL), (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());

    tmpX = GetPrivateProfileIntA(xorstr("REGISTER_BUTTON"), xorstr("X"), 618, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("REGISTER_BUTTON"), xorstr("Y"), 86, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    RegisterButtonPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    GetPrivateProfileStringA(xorstr("REGISTER_BUTTON"), xorstr("URL"), "", g_RegisterURL, sizeof(g_RegisterURL), (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());

    // S113: Repair butonu pozisyon + HIT
    tmpX = GetPrivateProfileIntA(xorstr("REPAIR_BUTTON"), xorstr("X"), 800, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("REPAIR_BUTTON"), xorstr("Y"), 40, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    RepairButtonPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    g_RepairHitX = GetPrivateProfileIntA(xorstr("REPAIR_BUTTON"), xorstr("HIT_X"), -1, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_RepairHitY = GetPrivateProfileIntA(xorstr("REPAIR_BUTTON"), xorstr("HIT_Y"), -1, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_RepairHitW = GetPrivateProfileIntA(xorstr("REPAIR_BUTTON"), xorstr("HIT_W"), 0, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_RepairHitH = GetPrivateProfileIntA(xorstr("REPAIR_BUTTON"), xorstr("HIT_H"), 0, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    // Close Button
    tmpX = GetPrivateProfileIntA(xorstr("CLOSE_BUTTON"), xorstr("X"), 765, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("CLOSE_BUTTON"), xorstr("Y"), 3, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    CloseButtonPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    g_CloseHitX = GetPrivateProfileIntA(xorstr("CLOSE_BUTTON"), xorstr("HIT_X"), -1, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_CloseHitY = GetPrivateProfileIntA(xorstr("CLOSE_BUTTON"), xorstr("HIT_Y"), -1, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_CloseHitW = GetPrivateProfileIntA(xorstr("CLOSE_BUTTON"), xorstr("HIT_W"), 0, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    g_CloseHitH = GetPrivateProfileIntA(xorstr("CLOSE_BUTTON"), xorstr("HIT_H"), 0, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());

    tmpX = GetPrivateProfileIntA(xorstr("PROGRESSBAR"), xorstr("X"), 24, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    tmpY = GetPrivateProfileIntA(xorstr("PROGRESSBAR"), xorstr("Y"), 557, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    ProgressPosition = D3DXVECTOR3(tmpX, tmpY, 0);
    // Font
    fontSize = GetPrivateProfileIntA(xorstr("FONT"), xorstr("FONT_SIZE"), 14, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    fontWeight = GetPrivateProfileIntA(xorstr("FONT"), xorstr("FONT_WEIGHT"), 500, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    GetPrivateProfileStringA(xorstr("FONT"), xorstr("FAMILY"), "Verdana", InfoFont, 25, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    // Info Text
    TextStateBaseX = GetPrivateProfileIntA(xorstr("INFO_TEXT"), xorstr("X"), 578, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    TextStatePos.top = GetPrivateProfileIntA(xorstr("INFO_TEXT"), xorstr("Y"), 530, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    // Page Text
    TextPagePos.left = GetPrivateProfileIntA(xorstr("PAGE_TEXT"), xorstr("X"), 610, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    TextPagePos.top = GetPrivateProfileIntA(xorstr("PAGE_TEXT"), xorstr("Y"), 430, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    // Notices
    TextNoticePos.left = GetPrivateProfileIntA(xorstr("NOTICES"), xorstr("START_X"), 200, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    TextNoticePos.top = GetPrivateProfileIntA(xorstr("NOTICES"), xorstr("START_Y"), 480, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());
    NoticeOffsetY = GetPrivateProfileIntA(xorstr("NOTICES"), xorstr("OFFSET_Y"), 23, (std::string(WP) + dosyalarNerdeLenAmq + xorstr("UIXSettings.ini")).c_str());

    TextStatePos.right = iW;
    TextStatePos.bottom = iH;
    TextPagePos.right = iW;
    TextPagePos.bottom = iH;
    TextNoticePos.bottom = iH;
    TextNoticePos.right = iW;

    static int noticeBaseY = TextNoticePos.top;

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("Launcher"), NULL };
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindowA(wc.lpszClassName, _T(xorstr("Launcher")), WS_POPUP, 100, 100, iW, iH, NULL, NULL, wc.hInstance, NULL);
    CenterWindow(hwnd, NULL, iW, iH);
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, RGB(1, 1, 1), 0, LWA_COLORKEY);  // S113: cok koyu siyah (1,1,1) color-key — bg.png anti-alias bozulmasin diye gercek siyah (0,0,0) degil
    mainWindow = hwnd;

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // S113: HIZLI ACILIS — texture yuklemeden once 1 frame Clear + Present yap, pencere bos kara durmasin
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 1, 1, 1), 1.0f, 0);
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

    if (!LoadTextures())
    {
        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }

    mainInstance = hInstance;
    sayfa = 1;

    for (int i = 0; i < 6; i++)
        states[i] = STATE_NORMAL;

	Engine = new Launcher();
    Engine->window = mainWindow;
    Engine->cmd = lpCmdLine;

    if (m_font == NULL)
        D3DXCreateFontA(g_pd3dDevice, fontSize, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, MONO_FONT | FF_DONTCARE, InfoFont, &m_font);

//#define GUN 0
//#define AY 1
//#define YIL 2
//    time_t rawtime;
//    time(&rawtime);
//    struct tm* timeInfoEx = localtime(&rawtime);
//
//    bool kappa = false;
//
//    int yil = timeInfoEx->tm_year + 1900;
//
//    if (yil > lisansTarih[YIL])
//        kappa = true;
//    else if (timeInfoEx->tm_mon > lisansTarih[AY] - 1 && yil == lisansTarih[YIL])
//        kappa = true;
//    else if (timeInfoEx->tm_mday > lisansTarih[GUN] && timeInfoEx->tm_mon == lisansTarih[AY] - 1 && yil == lisansTarih[YIL])
//        kappa = true;
//
//    if (!IsLicensed(Engine->m_settingsIP))
//        kappa = true;
//
//    if (kappa)
//        Engine->SetState(xorstr("Unknown data stream."));
//    else 
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SocketSystem, NULL, NULL, NULL);
   
    hCursorNormal = LoadCursor(NULL, IDC_ARROW);
    hCursorHand = LoadCursor(NULL, IDC_ARROW);
	hCursorClick = LoadCursor(NULL, IDC_ARROW);
    SetCursor(hCursorNormal);

    GetCurrentDirectoryA(MAX_PATH, Engine->WorkingPath);
    thyke_t = new thyke_Test;

    // S114: Periyodik KOXP tarama - her 30 saniyede bir yeniden kontrol
    DWORD lastCheatScanTick = GetTickCount();
    const DWORD CHEAT_SCAN_INTERVAL_MS = 30000;

    // Main loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // S114: 30 sn'de bir cheat scan
            if (GetTickCount() - lastCheatScanTick >= CHEAT_SCAN_INTERVAL_MS) {
                lastCheatScanTick = GetTickCount();
                std::string detected;
                if (Engine && Engine->ScanCheatTools(detected)) {
                    std::string msg2 = "MalaysiaKO - Anti-Cheat Uyarisi\n\n";
                    msg2 += "Cheat/makro yazilim tespit edildi:\n  >> " + detected + "\n\n";
                    msg2 += "Launcher kapatiliyor.";
                    MessageBoxA(NULL, msg2.c_str(), "Anti-Cheat", MB_OK | MB_ICONERROR);
                    PostQuitMessage(0);
                    continue;
                }
            }


            if (GetAsyncKeyState(VK_LBUTTON))
                SetCursor(hCursorClick);
            else
                SetCursor(hCursorNormal);

            g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
            g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
            g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
            g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 1, 1, 1), 1.0f, 0);
            if (g_pd3dDevice->BeginScene() >= 0)
            {
                pbFill.left = 0;
                pbFill.right = ProgressSurface.Width * Engine->GetPercent() / 100;
                pbFill.top = 0;
                pbFill.bottom = ProgressSurface.Height;

                LauncherSprite->Begin(D3DXSPRITE_ALPHABLEND);
                LauncherSprite->Draw(LauncherBackgroundTexture, NULL, NULL, &LauncherBackgorundPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                LauncherSprite->Draw(states[0] == STATE_NORMAL ? StartButtonTexture : (states[0] == STATE_DOWN ? StartButtonDownTexture : StartButtonHoverTexture), NULL, NULL, &StartButtonPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                if (HomePageButtonTexture)
                    LauncherSprite->Draw(states[1] == STATE_NORMAL ? HomePageButtonTexture : (states[1] == STATE_DOWN ? (HomePageButtonDownTexture ? HomePageButtonDownTexture : HomePageButtonTexture) : (HomePageButtonHoverTexture ? HomePageButtonHoverTexture : HomePageButtonTexture)), NULL, NULL, &HomePageButtonPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                LauncherSprite->Draw(states[2] == STATE_NORMAL ? SettingsButtonTexture : (states[2] == STATE_DOWN ? SettingsButtonDownTexture : SettingsButtonHoverTexture), NULL, NULL, &SettingsButtonPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                LauncherSprite->Draw(states[3] == STATE_NORMAL ? CloseButtonTexture : (states[3] == STATE_DOWN ? CloseButtonDownTexture : CloseButtonHoverTexture), NULL, NULL, &CloseButtonPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                // S113: Compact butonu — texture varsa ciz
                if (CompactButtonTexture)
                    LauncherSprite->Draw(states[7] == STATE_NORMAL ? CompactButtonTexture : (states[7] == STATE_DOWN ? (CompactButtonDownTexture ? CompactButtonDownTexture : CompactButtonTexture) : (CompactButtonHoverTexture ? CompactButtonHoverTexture : CompactButtonTexture)), NULL, NULL, &CompactButtonPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                if (DiscordButtonTexture)
                    LauncherSprite->Draw(states[4] == STATE_NORMAL ? DiscordButtonTexture : (states[4] == STATE_DOWN ? (DiscordButtonDownTexture ? DiscordButtonDownTexture : DiscordButtonTexture) : (DiscordButtonHoverTexture ? DiscordButtonHoverTexture : DiscordButtonTexture)), NULL, NULL, &DiscordButtonPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                if (ForumButtonTexture)
                    LauncherSprite->Draw(states[5] == STATE_NORMAL ? ForumButtonTexture : (states[5] == STATE_DOWN ? (ForumButtonDownTexture ? ForumButtonDownTexture : ForumButtonTexture) : (ForumButtonHoverTexture ? ForumButtonHoverTexture : ForumButtonTexture)), NULL, NULL, &ForumButtonPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                if (FacebookButtonTexture)
                    LauncherSprite->Draw(states[6] == STATE_NORMAL ? FacebookButtonTexture : (states[6] == STATE_DOWN ? (FacebookButtonDownTexture ? FacebookButtonDownTexture : FacebookButtonTexture) : (FacebookButtonHoverTexture ? FacebookButtonHoverTexture : FacebookButtonTexture)), NULL, NULL, &FacebookButtonPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                if (RegisterButtonTexture)
                    LauncherSprite->Draw(states[8] == STATE_NORMAL ? RegisterButtonTexture : (states[8] == STATE_DOWN ? (RegisterButtonDownTexture ? RegisterButtonDownTexture : RegisterButtonTexture) : (RegisterButtonHoverTexture ? RegisterButtonHoverTexture : RegisterButtonTexture)), NULL, NULL, &RegisterButtonPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                if (RepairButtonTexture)
                    LauncherSprite->Draw(states[9] == STATE_NORMAL ? RepairButtonTexture : (states[9] == STATE_DOWN ? (RepairButtonDownTexture ? RepairButtonDownTexture : RepairButtonTexture) : (RepairButtonHoverTexture ? RepairButtonHoverTexture : RepairButtonTexture)), NULL, NULL, &RepairButtonPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                LauncherSprite->Draw(ProgressTexture, NULL, NULL, &ProgressPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
                LauncherSprite->Draw(ProgressFillTexture, &pbFill, NULL, &ProgressPosition, D3DCOLOR_ARGB(255, 255, 255, 255));
            
                LauncherSprite->End();

                int width = GetTextWidth(Engine->GetState().c_str(), m_font);

                // S113: TextStateBaseX = X CENTER pozisyonu. Text genisliginin yarisi kadar sola kaydir.
                TextStatePos.left = TextStateBaseX - width / 2;

                m_font->DrawTextA(NULL, Engine->GetState().c_str(), -1, &TextStatePos, 0, D3DCOLOR_ARGB(255, 255, 255, 255));

                // S113: Tooltip — buton hover'inda kucuk metin (buton ustunde)
                struct TT { ButtonState* state; const char* text; const D3DXVECTOR3* pos; int hitX; int hitY; };
                TT tooltips[] = {
                    {&states[1], "Web Sitesi",                          &HomePageButtonPosition,  g_StartHitX, 0},  // dummy hitX, gercek pos kullanilir
                    {&states[5], "Forum",                                &ForumButtonPosition,     0, 0},
                    {&states[7], "UI Cache Temizle",                     &CompactButtonPosition,   0, 0},
                    {&states[9], "Patch Sifirla (Repair)",               &RepairButtonPosition,    g_RepairHitX, g_RepairHitY}
                };
                for (auto& t : tooltips) {
                    if (*t.state == STATE_HOVER) {
                        RECT tr;
                        tr.left = (LONG)t.pos->x - 60;
                        tr.top  = (LONG)t.pos->y - 25;
                        tr.right = tr.left + 200;
                        tr.bottom = tr.top + 22;
                        // Kara gölge + ana metin (basit shadow)
                        RECT trs = tr; trs.left += 1; trs.top += 1;
                        m_font->DrawTextA(NULL, t.text, -1, &trs, 0, D3DCOLOR_ARGB(255, 0, 0, 0));
                        m_font->DrawTextA(NULL, t.text, -1, &tr,  0, D3DCOLOR_ARGB(255, 200, 168, 75));
                    }
                }

                g_pd3dDevice->EndScene();
            }
            HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

            if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
                ResetDevice();
        }
    }
  

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
    
    return msg.wParam;
}

bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    HRESULT ret = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice);

    if (ret != D3D_OK)
    {
        switch (ret) 
        {
        case D3DERR_DEVICELOST:
            MessageBoxA(mainWindow, xorstr("DirectX device cannot be created."), "D3DERR_DEVICELOST", MB_ICONEXCLAMATION);
            break;
        case D3DERR_INVALIDCALL:
            MessageBoxA(mainWindow, xorstr("DirectX device cannot be created."), "D3DERR_INVALIDCALL", MB_ICONEXCLAMATION);
            break;
        case D3DERR_NOTAVAILABLE:
            MessageBoxA(mainWindow, xorstr("DirectX device cannot be created."), "D3DERR_NOTAVAILABLE", MB_ICONEXCLAMATION);
            break;
        case D3DERR_OUTOFVIDEOMEMORY:
            MessageBoxA(mainWindow, xorstr("DirectX device cannot be created."), "D3DERR_OUTOFVIDEOMEMORY", MB_ICONEXCLAMATION);
            break;
        default:
            MessageBoxA(mainWindow, xorstr("DirectX device cannot be created."), std::to_string(ret).c_str(), MB_ICONEXCLAMATION);
            break;
        }
        return false;
    }

	return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    //while (hr != D3D_OK)
    //    hr = g_pd3dDevice->Reset(&g_d3dpp);

	//HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	/*if (hr == D3DERR_INVALIDCALL)
		ASSERT(0);*/
}

#define YOUR_UNIQUE_ID 5576
GDIHelper gdiHelper;

static TCHAR szWindowClass[] = _T("Knight OnLine Code Guard");
static TCHAR szTitle[] = _T("Knight OnLine Code Guard");
HWND hLoadHwnd;
LRESULT CALLBACK WndProc222(HWND, UINT, WPARAM, LPARAM);
const int IMAGE_WIDTH = 175;
const int IMAGE_HEIGHT = 263;
// S114: Aktif GIF resource ID + loop flag (SetupBanner cagrisi oncesi set edilir)
static int g_currentGifResId = IDB_LOADING;
static bool g_currentGifLooped = false;
extern int g_gifEndAction;  // GDIHelper.cpp: 0=loop, 1=KO+exit, 2=sadece exit

LRESULT CALLBACK WndProc222(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message)
    {
    case WM_ACTIVATE:
        break;
    case WM_COMMAND:
        break;
    case WM_CREATE:
    {
        //std::string Path = std::string(Engine->WorkingPath) + "\\CodeGuard\\Launcher\\" + "curse_loading.gif";
        //gdiHelper.DisplayImageFromFile(
        //    Path.c_str(),						//File location
        //    hWnd,								//Handle to the Window
        //    YOUR_UNIQUE_ID,						//Unique ID of your control, declare your own.
        //    0,									//xPosition
        //    0,									//yPosition
        //    700,								//width 
        //    400,								//height
        //    false								//looped
        //);
        
        // S113: Image penceredeki (0,0) noktasinda cizilir, ekran degil
        // S114: Global g_currentGifResId'den dinamik GIF (SCANNING/SAFE/ERROR)
        gdiHelper.DisplayImageFromResource(
            mainInstance,
            MAKEINTRESOURCEW(g_currentGifResId),
            (LPCWSTR)RT_RCDATA,
            hWnd,
            YOUR_UNIQUE_ID,						//Unique ID of your control
            0,									//xPosition (penceredeki sol)
            0,									//yPosition (penceredeki ust)
            175,								//width
            263,							    //height
            g_currentGifLooped					// S114: SCANNING -> true (sonsuz), SAFE/ERROR -> false (1 dongu sonra action)
        );
    }
    break;
    case WM_DESTROY:
    {
        // S114: TerminateProcess KALDIRILDI - SCANNING bittikten sonra StartClick devam etmeli
        gdiHelper.Destroy();
        PostQuitMessage(0);
        break;
    }
    break;
    case WM_LBUTTONDBLCLK:
    {
        int a = 0;
    }
    break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

int thyke_Test::SetupBanner(int gifResId, DWORD minMs, bool launchGame)
{
    // S114: Caller'in istegine gore GIF + davranis konfigure
    g_currentGifResId = gifResId;
    // S114: TUM GIF'ler sonsuz loop. SetupBanner minMs ile pencereyi kapatir.
    // Boylece SAFE GIF'i (1.2sn) MIN_MS dolmadan bitmesin kullanici tam goremesin.
    g_currentGifLooped = true;
    // SAFE -> 1 (KO basla + exit), ERROR -> 2 (sadece exit, KO YOK)
    if (gifResId == IDB_LOADING_ERROR)      g_gifEndAction = 2;
    else if (gifResId == IDB_LOADING_SAFE)  g_gifEndAction = 1;
    else                                    g_gifEndAction = 0;  // SCANNING: GIF loop, MIN_MS bitince SetupBanner kapatir

    ShowWindow(mainWindow, FALSE);

    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSEX wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc222;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = mainInstance;
    wcex.hIcon = LoadIcon(mainInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex)) {
        // S114: Cifte cagrida ERROR_CLASS_ALREADY_EXISTS olabilir, devam et
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            MessageBox(NULL, _T("Call to RegisterClassEx failed!"), szTitle, NULL);
            return 1;
        }
    }

    HWND hwnd = CreateWindow(szWindowClass, szTitle, WS_POPUP/*WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME*/, 0, 0, 175, 263, NULL, NULL, wcex.hInstance, NULL);
    hLoadHwnd = hwnd;

    if (!hwnd) {
        MessageBox(NULL, _T("Call to CreateWindow failed!"), szTitle, NULL);
        return 1;
    }

    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);

    // S113: SAG ALT kose (eskiden orta idi)
    int xPos = GetSystemMetrics(SM_CXSCREEN) - 175 - 30;  // sag kenardan 30px ic
    int yPos = GetSystemMetrics(SM_CYSCREEN) - 263 - 60;  // alt kenardan 60px ic (taskbar uzeri)
    SetWindowPos(hwnd, HWND_TOP, xPos, yPos, 175, 263, SWP_NOZORDER);

    //SetWindowPos(FindWindow(Nil, PCHAR(program1)), HWND_TOP, 0, 0, Screen.Width, Screen.Height, SWP_SHOWWINDOW)

    ShowWindow(hwnd, TRUE);
    UpdateWindow(hwnd);

    // S114: Caller'in verdigi minMs kadar splash kalir
    DWORD startTick = GetTickCount();
    const DWORD MIN_SPLASH_MS = minMs;

    MSG msg;
    BOOL bRet;
    ZeroMemory(&msg, sizeof(msg));

    while (true) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                // Min sure dolmadiysa bekle
                DWORD elapsed = GetTickCount() - startTick;
                if (elapsed < MIN_SPLASH_MS) {
                    Sleep(MIN_SPLASH_MS - elapsed);
                }
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            Sleep(10);  // CPU yormasin
            // Min sure dolduysa otomatik cik
            if (GetTickCount() - startTick >= MIN_SPLASH_MS) {
                PostQuitMessage(0);
            }
        }
    }

end:
    if (IsWindow(hwnd)) ::DestroyWindow(hwnd);
    ::UnregisterClass(wcex.lpszClassName, wcex.hInstance);
    GdiplusShutdown(gdiplusToken); //dont forget to shut down the gdi+ token.

    // S114: GIF kapandi, simdi caller'in istedigi action'i yap
    // launchGame=true ise KO basla + exit (SAFE durumu)
    // gifResId=ERROR ise sadece exit (KO yok)
    // SCANNING (loop, action=0) ise hicbir sey yapma, donsun
    if (g_gifEndAction == 1 && launchGame) {
        // S114 K3: Launcher param formati: "<pid> /HWID:<md5>"
        // KO client launch parametrelerini parse edip login paketine HWID ekleyecek.
        std::string hwid = (Engine ? Engine->ComputeHwidA() : "");
        std::string param = std::to_string(GetCurrentProcessId());
        if (!hwid.empty()) param += " /HWID:" + hwid;
        if ((long)ShellExecuteA(NULL, NULL, xorstr("KnightOnLine.exe"), param.c_str(), NULL, SW_RESTORE) == ERROR_FILE_NOT_FOUND) {
            MessageBoxA(mainWindow, xorstr("KnightOnLine.exe not found."), xorstr("Launcher"), MB_ICONINFORMATION);
            return 0;
        }
        ExitProcess(0);
    } else if (g_gifEndAction == 2) {
        ExitProcess(0);
    }
    return 1;
}


bool _fexists(std::string& filename)
{
    std::ifstream ifile(filename.c_str());
    return (bool)ifile;
}

void StartClick()
{
    if (lastStartState == STATE_DOWN && Engine->IsReady())
    {
        states[0] = STATE_HOVER;
        lastStartState = STATE_HOVER;

        // S114: 3-asama GIF anti-cheat akisi
        // Faz 0: Taze scan — kullanici Launcher acikken sonradan cheat acmis olabilir
        // Background thread, SCANNING GIF (4sn) icinde rahat biter (~50-200ms)
        std::thread([](){
            if (!Engine) return;
            std::string detected;
            Engine->m_scanThreatDetected = Engine->ScanCheatTools(detected);
            Engine->m_scanThreatName = detected;
        }).detach();

        // Faz 1: SCANNING GIF (4 sn looped) — kullaniciya tarama animasyonu goster
        thyke_t->SetupBanner(IDB_LOADING, 4000, false);

        // Faz 2: Sonuca gore SAFE veya ERROR
        // Tarama LauncherEngine constructor'da yapildi, sonuc Engine->m_scanThreatDetected'de
        // TBL mismatch >0 ise tehdit say (cheat sup hesi VEYA bozuk dosya — KO acilmasi tehlikeli)
        bool threat = Engine->m_scanThreatDetected || (Engine->m_tblMismatchCount > 0);
        if (threat) {
            // ERROR: 3 sn THREAT DETECTED GIF
            thyke_t->SetupBanner(IDB_LOADING_ERROR, 3000, false);
            // GIF kapandi - kullaniciya neyin tespit edildigini bilgilendir, sonra kapan
            std::string msg = "MalaysiaKO Anti-Cheat\n\nOyuna giris engellendi.\n\n";
            if (Engine->m_scanThreatDetected && !Engine->m_scanThreatName.empty()) {
                msg += "Tespit: " + Engine->m_scanThreatName + "\n";
            }
            if (Engine->m_tblMismatchCount > 0) {
                char tbuf[128];
                sprintf_s(tbuf, "TBL dosya butunlugu: %d dosya degistirilmis\n", Engine->m_tblMismatchCount);
                msg += tbuf;
            }
            msg += "\nLutfen sebep oluyor olabilecek programi kapatip Launcher'i yeniden acin.";
            MessageBoxA(NULL, msg.c_str(), "MalaysiaKO Anti-Cheat", MB_OK | MB_ICONERROR);
            ExitProcess(0);
        } else {
            // SAFE: 2.5 sn SECURE GIF, sonra KO basla
            thyke_t->SetupBanner(IDB_LOADING_SAFE, 2500, true);
            // SetupBanner icindeki GDIHelper run() loop'u GIF 1 dongu bittikten sonra
            // g_gifEndAction==1 ile KnightOnLine.exe baslatip ExitProcess yapar
        }
    }
}

void HomepageClick()
{
    if (lastHomepageState == STATE_DOWN) {
        states[1] = STATE_HOVER;
        lastHomepageState = STATE_HOVER;
        if (strlen(g_HomepageURL) > 0)
            ShellExecuteA(NULL, "open", g_HomepageURL, NULL, NULL, SW_SHOW);
    }
}

void SettingsClick()
{
    if (lastSettingsState == STATE_DOWN && Engine->IsReady()) {
        states[2] = STATE_HOVER;
        lastSettingsState = STATE_HOVER;
        if ((long)ShellExecuteA(NULL, NULL, xorstr("Option.exe"), NULL, NULL, SW_RESTORE) == ERROR_FILE_NOT_FOUND)
            MessageBoxA(mainWindow, xorstr("Option.exe not found."), xorstr("Launcher"), MB_ICONINFORMATION);
        else
            ::PostQuitMessage(0);
    }
}

void CloseClick()
{
    if (lastCloseState == STATE_DOWN)
        ::PostQuitMessage(0);
}

// S113: Compact progress thread-safe callback
static void CompactProgress(int percent)
{
    if (Engine) {
        Engine->SetPercent((uint8)percent);
        Engine->SetState(xorstr("Compact: %") + std::to_string(percent));
    }
}

static bool g_compactRunning = false;

// S113: Compact butonu click — UI .src/.hdr sismeyi temizler (ARKA THREAD)
void CompactClick()
{
    if (lastCompactState == STATE_DOWN && !g_compactRunning) {
        states[7] = STATE_HOVER;
        lastCompactState = STATE_HOVER;
        if (MessageBoxA(mainWindow, xorstr("UI cache temizlenecek (2-3 dakika surebilir). Launcher arka planda calisir, kapatma. Devam?"), xorstr("Compact"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
            g_compactRunning = true;
            std::thread([](){
                CHDRSystem* compactor = new CHDRSystem;
                compactor->m_progressCallback = CompactProgress;
                CHAR cwd[MAX_PATH];
                GetCurrentDirectoryA(MAX_PATH, cwd);
                DWORD sizeBeforeLo = 0, sizeBeforeHi = 0;
                HANDLE hSrc = CreateFileA((std::string(cwd) + "\\ui\\ui.src").c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hSrc != INVALID_HANDLE_VALUE) { sizeBeforeLo = GetFileSize(hSrc, &sizeBeforeHi); CloseHandle(hSrc); }
                CompactProgress(0);
                compactor->Compact("ui");
                delete compactor;
                DWORD sizeAfterLo = 0, sizeAfterHi = 0;
                HANDLE hSrc2 = CreateFileA((std::string(cwd) + "\\ui\\ui.src").c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hSrc2 != INVALID_HANDLE_VALUE) { sizeAfterLo = GetFileSize(hSrc2, &sizeAfterHi); CloseHandle(hSrc2); }
                CompactProgress(100);
                if (Engine) Engine->SetState(xorstr("Compact tamamlandi."));
                char msg[256];
                sprintf_s(msg, "Once: %u MB\nSonra: %u MB", sizeBeforeLo / (1024*1024), sizeAfterLo / (1024*1024));
                MessageBoxA(mainWindow, msg, xorstr("Compact tamamlandi"), MB_OK | MB_ICONINFORMATION);
                g_compactRunning = false;
            }).detach();
        }
    }
}

void DiscordClick()
{
    if (lastDiscordState == STATE_DOWN) {
        states[4] = STATE_HOVER;
        lastDiscordState = STATE_HOVER;
        if (strlen(g_DiscordURL) > 0)
            ShellExecuteA(NULL, "open", g_DiscordURL, NULL, NULL, SW_SHOW);
    }
}

void ForumClick()
{
    if (lastForumState == STATE_DOWN) {
        states[5] = STATE_HOVER;
        lastForumState = STATE_HOVER;
        if (strlen(g_ForumURL) > 0)
            ShellExecuteA(NULL, "open", g_ForumURL, NULL, NULL, SW_SHOW);
    }
}

void FacebookClick()
{
    if (lastFacebookState == STATE_DOWN) {
        states[6] = STATE_HOVER;
        lastFacebookState = STATE_HOVER;
        if (strlen(g_FacebookURL) > 0)
            ShellExecuteA(NULL, "open", g_FacebookURL, NULL, NULL, SW_SHOW);
    }
}

// S113: Register butonu — tarayicida register sayfasi acar
void RegisterClick()
{
    if (lastRegisterState == STATE_DOWN) {
        states[8] = STATE_HOVER;
        lastRegisterState = STATE_HOVER;
        if (strlen(g_RegisterURL) > 0)
            ShellExecuteA(NULL, "open", g_RegisterURL, NULL, NULL, SW_SHOW);
    }
}

// S113: Repair butonu — sadece Server.ini=2369 (.src/.hdr DOKUNMA, append-only ile uzerine yazilir)
void RepairClick()
{
    if (lastRepairState == STATE_DOWN) {
        states[9] = STATE_HOVER;
        lastRepairState = STATE_HOVER;
        if (MessageBoxA(mainWindow, xorstr("PATCH SIFIRLANACAK!\n\nTum patch yeniden indirilecek (.src/.hdr dosyalarinin UZERINE yazilir).\nLauncher yeniden acilacak.\n\nDevam edilsin mi?"), xorstr("Repair - Patch Sifirlama"), MB_YESNO | MB_ICONWARNING) == IDYES) {
            CHAR cwd[MAX_PATH];
            GetCurrentDirectoryA(MAX_PATH, cwd);
            // S113: SADECE Server.ini sifirla (.src/.hdr DOKUNMA — silmek UI'i komple bozar)
            WritePrivateProfileStringA(xorstr("Version"), xorstr("Files"), "2369", (std::string(cwd) + "\\Server.ini").c_str());
            char msg[256];
            sprintf_s(msg, "Patch sifirlandi.\nServer.ini = Files=2369\n\nLauncher 2 saniye sonra yeniden acilacak, tum patch'ler yeniden indirilecek.");
            MessageBoxA(mainWindow, msg, xorstr("Repair tamam"), MB_OK | MB_ICONINFORMATION);

            // S113: helper.bat olustur, kendini yeniden baslat
            std::string batPath = std::string(cwd) + "\\_repair_restart.bat";
            FILE* bat = nullptr;
            fopen_s(&bat, batPath.c_str(), "w");
            if (bat) {
                fprintf(bat, "@echo off\r\n");
                fprintf(bat, "timeout /t 2 /nobreak >nul\r\n");
                fprintf(bat, "start \"\" \"%s\\Launcher.exe\"\r\n", cwd);
                fprintf(bat, "del \"%%~f0\"\r\n");  // kendini sil
                fclose(bat);
                ShellExecuteA(NULL, "open", batPath.c_str(), NULL, cwd, SW_HIDE);
                ::PostQuitMessage(0);
            }
        }
    }
}


// S113: Hittest yardimcisi — HIT_X >=0 ise HIT bolge, yoksa eski position+surface
static bool inHit(int x, int y, int hitX, int hitY, int hitW, int hitH, const D3DXVECTOR3& pos, const D3DSURFACE_DESC& surf) {
    if (hitX >= 0 && hitY >= 0 && hitW > 0 && hitH > 0)
        return x >= hitX && x <= hitX + hitW && y >= hitY && y <= hitY + hitH;
    return x >= pos.x && x <= pos.x + surf.Width && y >= pos.y && y <= pos.y + surf.Height;
}

bool isInArea(int x, int y)
{
    if (inHit(x, y, g_StartHitX, g_StartHitY, g_StartHitW, g_StartHitH, StartButtonPosition, StartButtonSurface) && Engine->IsReady())
    {
        return true;
    }
    if (HomePageButtonTexture && x >= HomePageButtonPosition.x && x <= HomePageButtonPosition.x + HomePageButtonSurface.Width && y >= HomePageButtonPosition.y && y <= HomePageButtonPosition.y + HomePageButtonSurface.Height)
    {
        return true;
    }
    if (inHit(x, y, g_SettingsHitX, g_SettingsHitY, g_SettingsHitW, g_SettingsHitH, SettingsButtonPosition, SettingsButtonSurface) && Engine->IsReady())
    {
        return true;
    }
    if (inHit(x, y, g_CloseHitX, g_CloseHitY, g_CloseHitW, g_CloseHitH, CloseButtonPosition, CloseButtonSurface))
    {
        return true;
    }
    // S113: Compact buton tikla — IsReady() kontrolu YOK (her zaman aktif)
    if (CompactButtonTexture && x >= CompactButtonPosition.x && x <= CompactButtonPosition.x + CompactButtonSurface.Width && y >= CompactButtonPosition.y && y <= CompactButtonPosition.y + CompactButtonSurface.Height)
    {
        return true;
    }
    if (DiscordButtonTexture && x >= DiscordButtonPosition.x && x <= DiscordButtonPosition.x + DiscordButtonSurface.Width && y >= DiscordButtonPosition.y && y <= DiscordButtonPosition.y + DiscordButtonSurface.Height)
    {
        return true;
    }
    if (ForumButtonTexture && x >= ForumButtonPosition.x && x <= ForumButtonPosition.x + ForumButtonSurface.Width && y >= ForumButtonPosition.y && y <= ForumButtonPosition.y + ForumButtonSurface.Height)
    {
        return true;
    }
    if (FacebookButtonTexture && x >= FacebookButtonPosition.x && x <= FacebookButtonPosition.x + FacebookButtonSurface.Width && y >= FacebookButtonPosition.y && y <= FacebookButtonPosition.y + FacebookButtonSurface.Height)
    {
        return true;
    }
    if (RegisterButtonTexture && x >= RegisterButtonPosition.x && x <= RegisterButtonPosition.x + RegisterButtonSurface.Width && y >= RegisterButtonPosition.y && y <= RegisterButtonPosition.y + RegisterButtonSurface.Height)
    {
        return true;
    }
    if (RepairButtonTexture && inHit(x, y, g_RepairHitX, g_RepairHitY, g_RepairHitW, g_RepairHitH, RepairButtonPosition, RepairButtonSurface))
    {
        return true;
    }
   
    return false;
}

void HandleMouse(ButtonState state, int x, int y)
{
    if (inHit(x, y, g_StartHitX, g_StartHitY, g_StartHitW, g_StartHitH, StartButtonPosition, StartButtonSurface) && Engine->IsReady())
    {
        if (lastStartState != STATE_DOWN) states[0] = state;
        if (state == STATE_UP) StartClick();
        else if (lastStartState != STATE_DOWN) lastStartState = state;
    }
    else {
        states[0] = STATE_NORMAL;
        if (state == STATE_UP) lastStartState = STATE_NORMAL;
    }
    if (HomePageButtonTexture && x >= HomePageButtonPosition.x && x <= HomePageButtonPosition.x + HomePageButtonSurface.Width && y >= HomePageButtonPosition.y && y <= HomePageButtonPosition.y + HomePageButtonSurface.Height)
    {
        if (lastHomepageState != STATE_DOWN) states[1] = state;
        if (state == STATE_UP) HomepageClick();
        else if (lastHomepageState != STATE_DOWN) lastHomepageState = state;
    }
    else {
        states[1] = STATE_NORMAL;
        if (state == STATE_UP) lastHomepageState = STATE_NORMAL;
    }
    if (inHit(x, y, g_SettingsHitX, g_SettingsHitY, g_SettingsHitW, g_SettingsHitH, SettingsButtonPosition, SettingsButtonSurface) && Engine->IsReady())
    {
        if (lastSettingsState != STATE_DOWN) states[2] = state;
        if (state == STATE_UP) SettingsClick();
        else if (lastSettingsState != STATE_DOWN) lastSettingsState = state;
    }
    else {
        states[2] = STATE_NORMAL;
        if (state == STATE_UP) lastSettingsState = STATE_NORMAL;
    }
    if (inHit(x, y, g_CloseHitX, g_CloseHitY, g_CloseHitW, g_CloseHitH, CloseButtonPosition, CloseButtonSurface))
    {
        if (lastCloseState != STATE_DOWN) states[3] = state;
        if (state == STATE_UP) CloseClick();
        else if (lastCloseState != STATE_DOWN) lastCloseState = state;
    }
    else {
        states[3] = STATE_NORMAL;
        if (state == STATE_UP) lastCloseState = STATE_NORMAL;
    }
    // S113: Compact buton mouse handler — IsReady() kontrolu YOK
    if (CompactButtonTexture && x >= CompactButtonPosition.x && x <= CompactButtonPosition.x + CompactButtonSurface.Width && y >= CompactButtonPosition.y && y <= CompactButtonPosition.y + CompactButtonSurface.Height)
    {
        if (lastCompactState != STATE_DOWN) states[7] = state;
        if (state == STATE_UP) CompactClick();
        else if (lastCompactState != STATE_DOWN) lastCompactState = state;
    }
    else {
        states[7] = STATE_NORMAL;
        if (state == STATE_UP) lastCompactState = STATE_NORMAL;
    }
    if (DiscordButtonTexture && x >= DiscordButtonPosition.x && x <= DiscordButtonPosition.x + DiscordButtonSurface.Width && y >= DiscordButtonPosition.y && y <= DiscordButtonPosition.y + DiscordButtonSurface.Height)
    {
        if (lastDiscordState != STATE_DOWN) states[4] = state;
        if (state == STATE_UP) DiscordClick();
        else if (lastDiscordState != STATE_DOWN) lastDiscordState = state;
    }
    else {
        states[4] = STATE_NORMAL;
        if (state == STATE_UP) lastDiscordState = STATE_NORMAL;
    }
    if (ForumButtonTexture && x >= ForumButtonPosition.x && x <= ForumButtonPosition.x + ForumButtonSurface.Width && y >= ForumButtonPosition.y && y <= ForumButtonPosition.y + ForumButtonSurface.Height)
    {
        if (lastForumState != STATE_DOWN) states[5] = state;
        if (state == STATE_UP) ForumClick();
        else if (lastForumState != STATE_DOWN) lastForumState = state;
    }
    else {
        states[5] = STATE_NORMAL;
        if (state == STATE_UP) lastForumState = STATE_NORMAL;
    }
    if (FacebookButtonTexture && x >= FacebookButtonPosition.x && x <= FacebookButtonPosition.x + FacebookButtonSurface.Width && y >= FacebookButtonPosition.y && y <= FacebookButtonPosition.y + FacebookButtonSurface.Height)
    {
        if (lastFacebookState != STATE_DOWN) states[6] = state;
        if (state == STATE_UP) FacebookClick();
        else if (lastFacebookState != STATE_DOWN) lastFacebookState = state;
    }
    else {
        states[6] = STATE_NORMAL;
        if (state == STATE_UP) lastFacebookState = STATE_NORMAL;
    }
    if (RegisterButtonTexture && x >= RegisterButtonPosition.x && x <= RegisterButtonPosition.x + RegisterButtonSurface.Width && y >= RegisterButtonPosition.y && y <= RegisterButtonPosition.y + RegisterButtonSurface.Height)
    {
        if (lastRegisterState != STATE_DOWN) states[8] = state;
        if (state == STATE_UP) RegisterClick();
        else if (lastRegisterState != STATE_DOWN) lastRegisterState = state;
    }
    else {
        states[8] = STATE_NORMAL;
        if (state == STATE_UP) lastRegisterState = STATE_NORMAL;
    }
    if (RepairButtonTexture && inHit(x, y, g_RepairHitX, g_RepairHitY, g_RepairHitW, g_RepairHitH, RepairButtonPosition, RepairButtonSurface))
    {
        if (lastRepairState != STATE_DOWN) states[9] = state;
        if (state == STATE_UP) RepairClick();
        else if (lastRepairState != STATE_DOWN) lastRepairState = state;
    }
    else {
        states[9] = STATE_NORMAL;
        if (state == STATE_UP) lastRepairState = STATE_NORMAL;
    }
}

static int xClick;
static int yClick;

static bool leftMouse = false;
static bool dragWindow = false;

DWORD g_lastMouseX = 0, g_lastMouseY = 0;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SOCKETMSG: {
        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_CONNECT: {
            //TRACE("Socket connected..\n");
        } break;
        case FD_CLOSE: {
            Engine->ready = false;
            Engine->SetState("Disconnected.");
            Engine->SetPercent(0);
            Engine->mSocket->Release();
        }  break;
        case FD_READ: {
            Engine->mSocket->Receive();
        } break;
        default: {
            __ASSERT(0, "WM_SOCKETMSG: unknown socket flag.");
        } break;
        }
    } break;
    case WM_LBUTTONDOWN:
        dragWindow = true;
        leftMouse = true;
        RECT prc;
        GetWindowRect(hWnd, &prc);
        g_lastMouseX = LOWORD(lParam);
        g_lastMouseY = HIWORD(lParam);
        SetCapture(hWnd);
        xClick = LOWORD(lParam);
        yClick = HIWORD(lParam);
        HandleMouse(STATE_DOWN, xClick, yClick);
        break;
    case WM_LBUTTONUP:
        leftMouse = false;
        HandleMouse(STATE_UP, LOWORD(lParam), HIWORD(lParam));
        ReleaseCapture();
        dragWindow = false;
        {
            int windowWidth, windowHeight;
            RECT mainWindowRect, desktop;
            GetWindowRect(hWnd, &mainWindowRect);
            const HWND hDesktop = GetDesktopWindow();
            GetWindowRect(hDesktop, &desktop);
            windowHeight = mainWindowRect.bottom - mainWindowRect.top;
            windowWidth = mainWindowRect.right - mainWindowRect.left;
            POINT realPos;
            realPos.x = mainWindowRect.left;
            realPos.y = mainWindowRect.top;
            if (mainWindowRect.right > desktop.right)
                realPos.x = desktop.right - windowWidth;
            else if (mainWindowRect.left < desktop.left)
                realPos.x = desktop.left;

            if (mainWindowRect.bottom > desktop.bottom)
                realPos.y = desktop.bottom - windowHeight;
            else if (mainWindowRect.top < desktop.top)
                realPos.y = desktop.top;
            MoveWindow(hWnd, realPos.x, realPos.y, windowWidth, windowHeight, TRUE);
        }
        break;
    case WM_MOUSEMOVE:
    {
        HandleMouse(STATE_HOVER, LOWORD(lParam), HIWORD(lParam));
        if (GetCapture() == hWnd && dragWindow && leftMouse && !isInArea(LOWORD(lParam), HIWORD(lParam)) && lastCloseState != STATE_DOWN && /*lastDiscordState  != STATE_DOWN &&*/ lastSettingsState != STATE_DOWN && /*lastHomepageState != STATE_DOWN &&*/ lastStartState != STATE_DOWN /*&& lastForumState != STATE_DOWN &&*/ /*lastFacebookState != STATE_DOWN*/)
        {
            RECT mainWindowRect;
            POINT pos;
            int windowWidth, windowHeight;
            pos.x = (int)(short)LOWORD(lParam);
            pos.y = (int)(short)HIWORD(lParam);
            GetWindowRect(hWnd, &mainWindowRect);

            windowHeight = mainWindowRect.bottom - mainWindowRect.top;
            windowWidth = mainWindowRect.right - mainWindowRect.left;

            ClientToScreen(hWnd, &pos);

            MoveWindow(hWnd, pos.x - g_lastMouseX, pos.y - g_lastMouseY, windowWidth, windowHeight, TRUE);
        }
        break;
    }
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SETCURSOR:
        SetCursor(hCursorHand);
        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0; 
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
