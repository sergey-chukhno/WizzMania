@echo off
setlocal

set CLIENT_DIR=build\client
if exist build\client\Release\wizz_client.exe set CLIENT_DIR=build\client\Release
if exist build\client\Debug\wizz_client.exe set CLIENT_DIR=build\client\Debug

echo [WizzMania] Launching Client 1...
start "Wizz Client 1" cmd /c "cd %CLIENT_DIR% && wizz_client.exe"

echo Waiting for client to initialize...
timeout /t 1 /nobreak >nul

echo [WizzMania] Launching Client 2...
start "Wizz Client 2" cmd /c "cd %CLIENT_DIR% && wizz_client.exe"

echo Done! Two instances running. Close the terminal to stop them.
endlocal
