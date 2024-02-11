@echo off

REM setting variables for compile and linking
cd /D "%~dp0"
set cl_common= /I..\src\ /I..\local\ /nologo /FC /Z7
set cl_debug= call cl /Od %cl_common%

REM prepare build folder
if not exist build mkdir build

REM compile and execute the program
pushd build
%cl_debug% ..\main.c
echo "---Compile Done---"

echo "---Clean Previous Results---"
if exist scanline.png del scanline.png 

echo Executing the compiled program...
call main.exe

echo "---Execution Done---"

call scanline.png
popd