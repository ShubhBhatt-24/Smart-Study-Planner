@echo off
setlocal
cd /d "%~dp0"
where g++ >nul 2>&1
if errorlevel 1 (
  echo ERROR: g++ not found. Install MinGW-w64 and add it to PATH.
  pause
  exit /b 1
)
echo Compiling C++ web server...
g++ -std=c++17 -Wall -Wextra backend\server.cpp -o planner_server.exe -lws2_32
if errorlevel 1 (
  echo Compilation failed. See error details above.
  pause
  exit /b 1
)
echo Starting local app at http://127.0.0.1:8080
start "" "http://127.0.0.1:8080"
planner_server.exe
pause
