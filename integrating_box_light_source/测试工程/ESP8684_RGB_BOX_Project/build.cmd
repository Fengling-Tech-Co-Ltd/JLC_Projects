@echo off
rem ESP-IDF v5.3.3 build script (ESP8684 / esp32c2)
set MSYSTEM=
set MINGW_PREFIX=
set MINGW_CHOST=
set MSYSTEM_PREFIX=
set IDF_TOOLS_PATH=D:\esp\.espressif
call D:\esp\v5.3.3\esp-idf\export.bat
cd /d %~dp0
python.exe "%IDF_PATH%\tools\idf.py" build
echo.
echo Build finished with exit code %ERRORLEVEL%
pause
