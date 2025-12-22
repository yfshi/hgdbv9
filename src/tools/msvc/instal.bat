@echo off
rem install script for x64
call install.bat %* || goto exit
if exist ..\..\..\configure cd ..\..\..
if exist ..\..\..\..\configure cd ..\..\..\..
cd %~dp0 
:exit
exit /b %errorlevel%

