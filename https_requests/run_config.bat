cd c:\PlatformIO\dzen\https_requests
del /Q platformio-device-monitor-*.log

copy /Y /V "%1\platformio.ini" c:\PlatformIO\dzen\https_requests\platformio.ini
copy /Y /V "%1\sdkconfig.esp32dev" c:\PlatformIO\dzen\https_requests\sdkconfig.esp32dev
copy /Y /V "%1\CMakeLists.txt" c:\PlatformIO\dzen\https_requests\src\CMakeLists.txt

platformio.exe run --target clean
platformio.exe run --target upload
copy c:\PlatformIO\dzen\https_requests\.pio\build\esp32dev\firmware.bin "%1\"

start platformio.exe device monitor

timeout /T 400 /NOBREAK

taskkill /F /IM platformio.exe

copy platformio-device-monitor-*.log "%1\"

timeout /T 180 /NOBREAK

exit