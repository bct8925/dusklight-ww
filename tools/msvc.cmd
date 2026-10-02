@echo off
rem Runs a command inside a Visual Studio x64 developer environment, from the repo root.
rem   tools\msvc.cmd cmake --preset windows-msvc-relwithdebinfo
rem   tools\msvc.cmd cmake --build --preset windows-msvc-relwithdebinfo -- -k 0
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (echo vswhere.exe not found; install Visual Studio or its Build Tools & exit /b 1)
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (echo No Visual Studio with the C++ x64 tools found & exit /b 1)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0.."
%*
