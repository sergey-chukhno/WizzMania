@echo off
setlocal

echo [WizzMania] Launching Client 1...
start "Wizz Client 1" cmd /c "cd build\client && wizz_client.exe"

echo Waiting for client to initialize...
timeout /t 1 /nobreak >nul

echo [WizzMania] Launching Client 2...
start "Wizz Client 2" cmd /c "cd build\client && wizz_client.exe"

echo Done! Two instances running. Close the terminal to stop them.
endlocal
