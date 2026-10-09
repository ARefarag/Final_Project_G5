@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Start-GameAndGo-Windows.ps1"
set "GAMEANDGO_EXIT=%ERRORLEVEL%"
if not "%GAMEANDGO_EXIT%"=="0" (
  echo.
  echo Game^&Go could not start. See the message above for the cause.
  echo Your local database files are preserved in %%LOCALAPPDATA%%\GameAndGo.
  pause
)
exit /b %GAMEANDGO_EXIT%
