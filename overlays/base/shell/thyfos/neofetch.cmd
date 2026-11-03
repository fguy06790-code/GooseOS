@echo off
if "%~1"=="" goto use_path
"%~1" --logo-type file --logo "%~dp0thyfos-logo.txt"
exit /b %errorlevel%
:use_path
fastfetch.exe --logo-type file --logo "%~dp0thyfos-logo.txt"
exit /b %errorlevel%
