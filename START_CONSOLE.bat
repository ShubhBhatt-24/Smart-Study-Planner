@echo off
setlocal
cd /d "%~dp0"
g++ -std=c++17 -Wall -Wextra backend\console_main.cpp -o planner_console.exe
if errorlevel 1 (pause & exit /b 1)
planner_console.exe
pause
