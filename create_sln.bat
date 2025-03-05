@echo off
cd /d %~dp0

if "%1" equ "" (
    echo You must pass the path to your Qt installation as the first argument to this batch file.
    echo For example: .\create_sln.bat "C:\Qt\6.6.3\msvc2019_64\"
    echo If you are opposed to opening a command prompt, drag this folder on top of the batch file.
    echo/
    pause
) else (
    echo Running CMake with Qt base directory set to "%1"...
    echo/
    cmake -B build -G "Visual Studio 17 2022" -DQT_BASEDIR="%1"
    echo/
    echo Assuming CMake setup went well, the project sln is located at "build/vpkedit.sln".
    echo/
    pause
)
