#include "rar.hpp"

#ifdef _WIN_ALL

DWORD WinNT()
{
  static int dwPlatformId=-1;
  static DWORD dwMajorVersion,dwMinorVersion;
  if (dwPlatformId==-1)
  {
    OSVERSIONINFO WinVer;
    WinVer.dwOSVersionInfoSize=sizeof(WinVer);
    GetVersionEx(&WinVer);
    dwPlatformId=WinVer.dwPlatformId;
    dwMajorVersion=WinVer.dwMajorVersion;
    dwMinorVersion=WinVer.dwMinorVersion;
  }
  DWORD Result=0;
  if (dwPlatformId==VER_PLATFORM_WIN32_NT)
    Result=dwMajorVersion*0x100+dwMinorVersion;

  return Result;
}


bool IsWindows11OrGreater()
{
  static bool IsSet=false;
  static bool IsWin11=false;
  if (!IsSet)
  {
    OSVERSIONINFOEX WinVer = {};
    WinVer.dwOSVersionInfoSize=sizeof(WinVer);
    GetVersionEx((OSVERSIONINFO*)&WinVer);
    IsWin11=WinVer.dwMajorVersion>10 ||
            WinVer.dwMajorVersion==10 && WinVer.dwBuildNumber>=22000;
    IsSet=true;
  }
  return IsWin11;
}

#endif
