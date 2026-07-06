@echo off
setlocal EnableExtensions EnableDelayedExpansion
REM Grid OS — standalone launcher for Windows x64.
REM Double-click GridOS.bat or run from cmd.exe.
REM
REM Requires: QEMU for Windows (winget install SoftwareFreedomConservancy.QEMU)
REM Optional: set GRIDOS_MODE=hd|4k|default   set GRIDOS_HEADLESS=1   set QEMU=path\to\qemu-system-x86_64.exe

set "VERSION=__GRIDOS_VERSION__"
set "SCRIPT_DIR=%~dp0"
set "CACHE=%LOCALAPPDATA%\GridOS\standalone-%VERSION%"
if defined GRIDOS_CACHE set "CACHE=%GRIDOS_CACHE%"

set "KERNEL=%CACHE%\grid-os.elf"
set "DISK=%CACHE%\grid.img"
set "QEMU_BIN=qemu-system-x86_64.exe"
if defined QEMU set "QEMU_BIN=%QEMU%"

call :find_qemu
if errorlevel 1 goto :no_qemu

if not exist "%CACHE%" mkdir "%CACHE%" >nul 2>&1
if not exist "%KERNEL%" copy /Y "%SCRIPT_DIR%grid-os.elf" "%KERNEL%" >nul
if not exist "%DISK%" copy /Y "%SCRIPT_DIR%grid.img" "%DISK%" >nul

set "MODE=%GRIDOS_MODE%"
if /I "%MODE%"=="hd" goto :mode_hd
if /I "%MODE%"=="4k" goto :mode_4k
set "VGA_ARGS="
set "QEMU_NAME=Grid OS %VERSION%"
goto :launch

:mode_hd
set "VGA_ARGS=-device VGA,xres=1920,yres=1080,edid=on"
set "QEMU_NAME=Grid OS - HDMI HD (1920x1080)"
goto :launch

:mode_4k
set "VGA_ARGS=-device VGA,xres=3840,yres=2160,edid=on"
set "QEMU_NAME=Grid OS - HDMI 4K (3840x2160)"
goto :launch

:launch
echo Starting Grid OS %VERSION% (Flynn's Grid)...
echo   Esc -^> grid^> shell in GridBASIC IDE
echo   grid^> poweroff to quit
echo.

set "DISPLAY_ARGS=-display sdl,zoom-to-fit=on"
if "%GRIDOS_HEADLESS%"=="1" set "DISPLAY_ARGS=-display none"

"%QEMU_BIN%" -machine q35,acpi=off -cpu qemu64 -m 128M ^
  -kernel "%KERNEL%" -drive if=none,id=grid0,file=%DISK%,format=raw ^
  -device virtio-blk-pci,drive=grid0 -netdev user,id=net0 ^
  -device virtio-net-pci,netdev=net0 -serial stdio -no-reboot ^
  -device isa-debug-exit,iobase=0xf4,iosize=0x04 -name "%QEMU_NAME%" ^
  %DISPLAY_ARGS% %VGA_ARGS%

if errorlevel 1 (
  echo.
  echo Grid OS exited with error %ERRORLEVEL%.
  pause
)
exit /b %ERRORLEVEL%

:find_qemu
if exist "%QEMU_BIN%" exit /b 0
where "%QEMU_BIN%" >nul 2>&1 && exit /b 0
if exist "%ProgramFiles%\qemu\qemu-system-x86_64.exe" (
  set "QEMU_BIN=%ProgramFiles%\qemu\qemu-system-x86_64.exe"
  exit /b 0
)
if exist "%ProgramFiles(x86)%\qemu\qemu-system-x86_64.exe" (
  set "QEMU_BIN=%ProgramFiles(x86)%\qemu\qemu-system-x86_64.exe"
  exit /b 0
)
if exist "%USERPROFILE%\scoop\apps\qemu\current\qemu-system-x86_64.exe" (
  set "QEMU_BIN=%USERPROFILE%\scoop\apps\qemu\current\qemu-system-x86_64.exe"
  exit /b 0
)
if exist "C:\ProgramData\chocolatey\bin\qemu-system-x86_64.exe" (
  set "QEMU_BIN=C:\ProgramData\chocolatey\bin\qemu-system-x86_64.exe"
  exit /b 0
)
exit /b 1

:no_qemu
echo QEMU not found.
echo Install with:  winget install SoftwareFreedomConservancy.QEMU
echo Or download from: https://www.qemu.org/download/#windows
echo Add qemu-system-x86_64.exe to PATH, then run this file again.
pause
exit /b 1
