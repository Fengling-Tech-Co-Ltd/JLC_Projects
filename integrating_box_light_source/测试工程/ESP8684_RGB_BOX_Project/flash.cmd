@echo off
rem ESP-IDF v5.3.3 flash script (ESP8684 / esp32c2, normal non-encrypted flash)
rem Usage: double-click, or run: flash.cmd [COMx]
set MSYSTEM=
set MINGW_PREFIX=
set MINGW_CHOST=
set MSYSTEM_PREFIX=
set IDF_TOOLS_PATH=D:\esp\.espressif
set IDF_PATH=D:\esp\v5.3.3\esp-idf
call D:\esp\v5.3.3\esp-idf\export.bat
cd /d %~dp0
if "%1"=="" (set PORT=COM6) else (set PORT=%1)
"D:\esp\.espressif\python_env\idf5.3_py3.12_env\Scripts\python.exe" "%IDF_PATH%\tools\idf.py" -p %PORT% flash
echo.
echo Flash finished with exit code %ERRORLEVEL%
pause
