@echo off
rem Atalho para o package.ps1 sem precisar mudar a ExecutionPolicy do PowerShell
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0package.ps1" %*
exit /b %ERRORLEVEL%
