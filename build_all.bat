@echo off
setlocal
python "%~dp0tools\build_all.py" %*
exit /b %ERRORLEVEL%
