@echo off
if exist config.bat call config.bat
pushd ..
pushd src
call revision.bat
if %errorlevel% neq 0 exit /b %errorlevel%
popd
call configure.bat
if %errorlevel% neq 0 exit /b %errorlevel%
call build.bat
if %errorlevel% neq 0 exit /b %errorlevel%

if not exist dist\temp mkdir dist\temp
if not exist dist\ffxiv mkdir dist\ffxiv
if not exist dist\temp\32 mkdir dist\temp\32
if not exist dist\ffxiv\32 mkdir dist\ffxiv\32
if not exist dist\temp\64 mkdir dist\temp\64
if not exist dist\ffxiv\64 mkdir dist\ffxiv\64

if not exist dist\temp_lua mkdir dist\temp_lua
if not exist dist\ffxiv_lua mkdir dist\ffxiv_lua
if not exist dist\temp_lua\32 mkdir dist\temp_lua\32
if not exist dist\ffxiv_lua\32 mkdir dist\ffxiv_lua\32
if not exist dist\temp_lua\64 mkdir dist\temp_lua\64
if not exist dist\ffxiv_lua\64 mkdir dist\ffxiv_lua\64
popd

xcopy /hrkysd /exclude:exclude_files.txt "..\bin\64\Release\*.dll" "temp\64\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd /exclude:exclude_files.txt "..\bin\32\Release\*.dll" "temp\32\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\64\Release\ActWebSocketImguiOverlay.dll" "temp\64\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\ActWebSocketImguiOverlay.dll" "temp\32\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%


xcopy /hrkysd "..\bin\64\Release\loader.dll" "temp\ACTWebSocketOverlay64.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\loader.dll" "temp\ACTWebSocketOverlay32.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\x64\Release\ReShade64.dll" "temp\ReShade64.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\Win32\Release\ReShade32.dll" "temp\ReShade32.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\resource\overlay_atlas.*" "temp\overlay_atlas.*"
if %errorlevel% neq 0 exit /b %errorlevel%

xcopy /hrkysd "..\bin\64\Release\*.dll" /exclude:exclude_files.txt "ffxiv\64\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\*.dll" /exclude:exclude_files.txt "ffxiv\32\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\64\Release\ActWebSocketImguiOverlay.dll" "ffxiv\64\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\ActWebSocketImguiOverlay.dll" "ffxiv\32\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%

xcopy /hrkysd "..\bin\64\Release\loader.dll" "ffxiv\ffxiv_dx11_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\loader.dll" "ffxiv\ffxiv_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\x64\Release\ReShade64.dll" "ffxiv\dxgi.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\Win32\Release\ReShade32.dll" "ffxiv\d3d9.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\resource\overlay_atlas.*" "ffxiv\overlay_atlas.*"
if %errorlevel% neq 0 exit /b %errorlevel%

if exist ACTWebSocketOverlay_latest.zip del ACTWebSocketOverlay_latest.zip
pushd temp
"c:\Program Files\7-Zip\7z.exe" a ..\ACTWebSocketOverlay_latest.zip *
if %errorlevel% neq 0 exit /b %errorlevel%
popd temp

if exist ACTWebSocketOverlay_ffxiv_latest.zip del ACTWebSocketOverlay_ffxiv_latest.zip
pushd ffxiv
"c:\Program Files\7-Zip\7z.exe" a ..\ACTWebSocketOverlay_ffxiv_latest.zip *
if %errorlevel% neq 0 exit /b %errorlevel%
popd ffxiv

REM ==========


xcopy /hrkysd /exclude:exclude_files.txt "..\bin\64\Release\*.dll" "temp\64\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd /exclude:exclude_files.txt "..\bin\32\Release\*.dll" "temp\32\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\64\Release\ActWebSocketImguiOverlayWithLua.dll" "temp\64\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\ActWebSocketImguiOverlayWithLua.dll" "temp\32\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%


xcopy /hrkysd "..\bin\64\Release\loader.dll" "temp_lua\ACTWebSocketOverlay64.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\loader.dll" "temp_lua\ACTWebSocketOverlay32.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\x64\Release\ReShade64.dll" "temp_lua\ReShade64.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\Win32\Release\ReShade32.dll" "temp_lua\ReShade32.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\resource\overlay_atlas.*" "temp_lua\overlay_atlas.*"
if %errorlevel% neq 0 exit /b %errorlevel%

xcopy /hrkysd "..\bin\64\Release\*.dll" /exclude:exclude_files.txt "ffxiv_lua\64\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\*.dll" /exclude:exclude_files.txt "ffxiv_lua\32\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\64\Release\ActWebSocketImguiOverlayWithLua.dll" "ffxiv_lua\64\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\ActWebSocketImguiOverlayWithLua.dll" "ffxiv_lua\32\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%

xcopy /hrkysd "..\bin\64\Release\loader.dll" "ffxiv_lua\ffxiv_dx11_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\loader.dll" "ffxiv_lua\ffxiv_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\x64\Release\ReShade64.dll" "ffxiv_lua\dxgi.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\Win32\Release\ReShade32.dll" "ffxiv_lua\d3d9.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\resource\overlay_atlas.*" "ffxiv_lua\overlay_atlas.*"
if %errorlevel% neq 0 exit /b %errorlevel%

if exist ACTWebSocketOverlay_latest.zip del ACTWebSocketOverlay_lua_latest.zip
pushd temp_lua
"c:\Program Files\7-Zip\7z.exe" a ..\ACTWebSocketOverlay_lua_latest.zip *
if %errorlevel% neq 0 exit /b %errorlevel%
popd temp

if exist ACTWebSocketOverlay_ffxiv_latest.zip del ACTWebSocketOverlay_lua_ffxiv_latest.zip
pushd ffxiv_lua
"c:\Program Files\7-Zip\7z.exe" a ..\ACTWebSocketOverlay_ffxiv_lua_latest.zip *
if %errorlevel% neq 0 exit /b %errorlevel%
popd ffxiv

REM =======
set /p version=<..\src\version
set /p tag=<..\src\tag
SET OWNER=ZCube
SET REPO=ACTWebSocketOverlay

xcopy /hrkyd ACTWebSocketOverlay_latest.zip ACTWebSocketOverlay_%version%.*
xcopy /hrkyd ACTWebSocketOverlay_ffxiv_latest.zip ACTWebSocketOverlay_ffxiv_%version%.*

echo ==========================================================================================
@py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_%version%.zip
@py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_ffxiv_%version%.zip
echo ==========================================================================================


:SetVariables
FOR /F "tokens=3 delims= " %%G IN ('REG QUERY "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v "Personal"') DO (SET DocumentDir=%%G)
if exist "%APPDATA%\Dropbox\info.json" SET DROP_INFO=%APPDATA%\Dropbox\info.json
if exist "%LOCALAPPDATA%\Dropbox\info.json" SET DROP_INFO=%LOCALAPPDATA%\Dropbox\info.json
for /f "tokens=*" %%a in ( 'powershell -NoProfile -ExecutionPolicy Bypass -Command "( ( Get-Content -Raw -Path %DROP_INFO% | ConvertFrom-Json ).personal.path)" ' ) do set dropBoxRoot=%%a
echo %dropBoxRoot%

if not exist "%dropBoxRoot%\share" mkdir "%dropBoxRoot%\share"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd  "%CD%\ACTWebSocketOverlay_latest.zip" /exclude:exclude_files.txt "%dropBoxRoot%\share"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd  "%CD%\ACTWebSocketOverlay_ffxiv_latest.zip" /exclude:exclude_files.txt "%dropBoxRoot%\share"
if %errorlevel% neq 0 exit /b %errorlevel%
copy /y "%CD%\..\src\version" "%dropBoxRoot%\share\ACTWebSocketOverlay_version"
if %errorlevel% neq 0 exit /b %errorlevel%
