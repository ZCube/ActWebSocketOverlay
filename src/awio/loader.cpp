/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include "version.h"
#include <stdio.h>
#include <Windows.h>
#include <filesystem>
#include <thread>
#include <mutex>
#include <iostream>

struct ImGuiContext;
/////////////////////////////////////////////////////////////////////////////////
#include <Windows.h>
#include "../mod.h"

typedef ModInterface* (*TModCreate)();
typedef int(*TModFree)(ModInterface* context);

typedef int(*TModUnInit)(ImGuiContext* context);
typedef int(*TModRender)(ImGuiContext* context);
typedef int(*TModInit)(ImGuiContext* context);
typedef void(*TModTextureData)(int index, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel);
typedef bool(*TModGetTextureDirtyRect)(int index, int dindex, RECT* rect);
typedef void(*TModSetTexture)(int index, void* texture);
typedef int(*TModTextureBegin)();
typedef void(*TModTextureEnd)();
typedef bool(*TModUpdateFont)(ImGuiContext* context);
typedef bool(*TModMenu)(bool* show);

TModCreate modCreate = nullptr;
TModFree  modFree = nullptr;
TModUnInit modUnInit = nullptr;
TModRender modRender = nullptr;
TModInit modInit = nullptr;
TModTextureData modTextureData = nullptr;
TModGetTextureDirtyRect modGetTextureDirtyRect = nullptr;
TModSetTexture modSetTexture = nullptr;
TModTextureBegin modTextureBegin = nullptr;
TModTextureEnd modTextureEnd = nullptr;
TModUpdateFont modUpdateFont = nullptr;
TModMenu modMenu = nullptr;
HMODULE mod = nullptr;
std::mutex m;
/////////////////////////////////////////////////////////////////////////////////


static void LoadModule()
{
	m.lock();
	if (!mod)
	{
		using namespace std::experimental;
		wchar_t result[MAX_PATH] = { 0, };
		GetModuleFileNameW(NULL, result, MAX_PATH);
		filesystem::path m = result;
		m.replace_extension("");

		filesystem::path paths[] =
		{
#ifdef _WIN64
			m.parent_path() / L"64" / L"ActWebSocketImguiOverlay.dll",
#else
			m.parent_path() / L"32" / L"ActWebSocketImguiOverlay.dll",
#endif
			m.parent_path() / L"ActWebSocketImguiOverlay.dll",
#ifdef _WIN64
			m.parent_path() / L"64" / L"ActWebSocketImguiOverlayWithLua.dll",
#else
			m.parent_path() / L"32" / L"ActWebSocketImguiOverlayWithLua.dll",
#endif
			m.parent_path() / L"ActWebSocketImguiOverlayWithLua.dll",
#ifdef _WIN64
			m.parent_path() / L"64" / L"overlay_mod.dll",
#else
			m.parent_path() / L"32" / L"overlay_mod.dll",
#endif
			m.parent_path() / L"overlay_mod.dll",
		};

		for (auto path : paths)
		{
			if (filesystem::exists(path))
			{
				char* names[] = {
					"libeay32.dll",
					"ssleay32.dll",
					nullptr
				};
				for (int i = 0; names[i]; ++i)
				{
					HMODULE ha = LoadLibraryW((path.parent_path() / names[i]).c_str());
					if (!ha) {
						HMODULE ha2 = LoadLibraryW(filesystem::path(names[i]).c_str());
					}
				}
				mod = LoadLibraryW(path.wstring().c_str());

				if (mod)
					break;
			}
		}
		if (mod)
		{
			modCreate = (TModCreate)GetProcAddress(mod, "ModCreate");
			modFree = (TModFree)GetProcAddress(mod, "ModFree");
			modInit = (TModInit)GetProcAddress(mod, "ModInit");
			modRender = (TModRender)GetProcAddress(mod, "ModRender");
			modUnInit = (TModUnInit)GetProcAddress(mod, "ModUnInit");
			modTextureData = (TModTextureData)GetProcAddress(mod, "ModTextureData");
			modSetTexture = (TModSetTexture)GetProcAddress(mod, "ModSetTexture");
			modGetTextureDirtyRect = (TModGetTextureDirtyRect)GetProcAddress(mod, "ModGetTextureDirtyRect");
			modTextureBegin = (TModTextureBegin)GetProcAddress(mod, "ModTextureBegin");
			modTextureEnd = (TModTextureEnd)GetProcAddress(mod, "ModTextureEnd");
			modUpdateFont = (TModUpdateFont)GetProcAddress(mod, "ModUpdateFont");
			modMenu = (TModMenu)GetProcAddress(mod, "ModMenu");
		}
	}
	m.unlock();
}

extern "C" ModInterface* ModCreate()
{
	if (!mod)
		LoadModule();
	if (modCreate)
		return modCreate();
}

extern "C" int ModFree(ModInterface* context)
{
	if (modFree)
		return modFree(context);
	return 0;
}

extern "C" int ModInit(ImGuiContext* context)
{
	if (modInit)
		return modInit(context);
	return 0;
}

extern "C" int ModUnInit(ImGuiContext* context)
{
	if (modUnInit)
		return modUnInit(context);
}


extern "C" void ModTextureData(int index, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
	if (modTextureData)
		modTextureData(index, out_pixels, out_width, out_height, out_bytes_per_pixel);
}

extern "C" void ModSetTexture(int index, void* texture)
{
	if (modSetTexture)
	{
		modSetTexture(index, texture);
	}
}

extern "C" bool ModGetTextureDirtyRect(int index, int dindex, RECT* rect)
{
	if (modGetTextureDirtyRect)
		return modGetTextureDirtyRect(index, dindex, rect);
	return false;
}

extern "C" int ModTextureBegin()
{
	if (modTextureBegin)
		return modTextureBegin();
	return 0;
}

extern "C" void ModTextureEnd()
{
	if (modTextureEnd)
		modTextureEnd();
}

extern "C" int ModRender(ImGuiContext* context)
{
	if (modRender)
		return modRender(context);
	return 0;
}

extern "C" bool ModMenu(bool* show)
{
	if (modMenu)
		return modMenu(show);
	return false;
}

extern "C" bool ModUpdateFont(ImGuiContext* context)
{
	if (modUpdateFont)
		return modUpdateFont(context);
	return false;
}

