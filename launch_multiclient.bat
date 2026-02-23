@echo off
setlocal

echo [WizzMania] Launching Server...
start "Wizz Server" cmd /c "cd build\server && wizz_server.exe"

echo Waiting for Server to initialize...
timeout /t 2 /nobreak >nul

echo [WizzMania] Launching Client 1...
start "Wizz Client 1" cmd /c "cd build\client && wizz_client.exe"

echo [WizzMania] Launching Client 2...
start "Wizz Client 2" cmd /c "cd build\client && wizz_client.exe"

echo Done! Let the chatting begin.
endlocal
