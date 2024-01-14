@echo off
setlocal enabledelayedexpansion

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (
  REM Who the fuck thought this was a good idea?
  call "%InstallDir%\Common7\Tools\vsdevcmd.bat" -no_logo -arch=x64 -host_arch=x64 && cl /nologo /EHsc /O2 /std:c++20 /utf-8 win64.cc /LD /MD  
) else (
  echo "[WARN] Failed to find Visual Studio 15.2+ (2017 and later)! Build win64.cc manually."
)

echo:
dotnet build --nologo ./Matsuoka/Matsuoka.csproj
copy /Y "Matsuoka\bin\Debug\net8.0\Matsuoka.dll" "%TEMP%\Matsuoka.dll"
pause
