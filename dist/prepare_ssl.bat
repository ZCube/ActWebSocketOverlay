@echo off
pushd ..
pushd src
call revision.bat
if %errorlevel% neq 0 exit /b %errorlevel%
popd
call configure.bat
if %errorlevel% neq 0 exit /b %errorlevel%
call build.bat
if %errorlevel% neq 0 exit /b %errorlevel%
popd

set postfix=_ssl
set use_ssl=TRUE
set use_atlas=TRUE
set dest=ACTWebSocketOverlay%postfix%
set reshade64dst=ReShade64.*
set reshade32dst=ReShade32.*
set overlay64dst=mod_loader_64.*
set overlay32dst=mod_loader_32.*
set overlay64src=ActWebSocketImguiOverlay.dll
set overlay32src=ActWebSocketImguiOverlay.dll
call prepare_dist.bat

set postfix=_lua_ssl
set use_ssl=TRUE
set use_atlas=FALSE
set dest=ACTWebSocketOverlay%postfix%
set reshade64dst=ReShade64.*
set reshade32dst=ReShade32.*
set overlay64dst=mod_loader_64.*
set overlay32dst=mod_loader_32.*
set overlay64src=ActWebSocketImguiOverlayWithLua.dll
set overlay32src=ActWebSocketImguiOverlayWithLua.dll
call prepare_dist.bat

set postfix=_ffxiv_ssl
set use_ssl=TRUE
set use_atlas=TRUE
set dest=ACTWebSocketOverlay%postfix%
set reshade64dst=dxgi.*
set reshade32dst=d3d9.*
set overlay64dst=mod_loader_64.*
set overlay32dst=mod_loader_32.*
set overlay64src=ActWebSocketImguiOverlay.dll
set overlay32src=ActWebSocketImguiOverlay.dll
call prepare_dist.bat

set postfix=_ffxiv_lua_ssl
set use_ssl=TRUE
set use_atlas=FALSE
set dest=ACTWebSocketOverlay%postfix%
set reshade64dst=dxgi.*
set reshade32dst=d3d9.*
set overlay64dst=mod_loader_64.*
set overlay32dst=mod_loader_32.*
set overlay64src=ActWebSocketImguiOverlayWithLua.dll
set overlay32src=ActWebSocketImguiOverlayWithLua.dll
call prepare_dist.bat
