﻿/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include "version.h"
#include <imgui.h>
#include "imgui_lua.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <Windows.h>
#include <unordered_map>
#include <atlstr.h>
#include <boost/algorithm/string.hpp>
#define STR2UTF8(s) (CW2A(CA2W(s), CP_UTF8))
#define UTF82WSTR(s) (CA2W(s), CP_UTF8)

#include "Serializable.h"
#include "Preference.h"
#include "OverlayContext.h"
#include "AWIOOverlay.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

void ClearUniqueValues();

#include <imgui_internal.h>
#include "OverlayMain.h"

#include <boost/thread/recursive_mutex.hpp>

class ModInstance : public ModInterface
{
public:
	boost::recursive_mutex instanceLock;
	OverlayInstance instance;
public:
	virtual ~ModInstance() {
	}
	virtual int Init(ImGuiContext* context);
	virtual int UnInit(ImGuiContext* context);
	virtual int Render(ImGuiContext* context);
	virtual int TextureBegin();
	virtual void TextureEnd();
	virtual void TextureData(int index, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel);
	virtual bool GetTextureDirtyRect(int index, int dindex, RECT* rect);
	virtual void SetTexture(int index, void* texture);
	virtual bool UpdateFont(ImGuiContext* context);
	virtual bool Menu(bool* show);
};

ModInstance* getModInstance(lua_State* L)
{
	lua_getglobal(L, "__instance");
	return lua_islightuserdata(L, 1) ? reinterpret_cast<ModInstance*>(lua_touserdata(L, 1)) : nullptr;
}

extern "C" int saveWindowPos(lua_State* L)
{
	auto mod = getModInstance(L);
	if (!mod)
	{
		return 0;
	}
	boost::recursive_mutex::scoped_lock l(mod->instanceLock);
	{
		mod->instance.options.SaveWindowPos();
	}
	return 0;
}

extern "C" int getWindowSize(lua_State* L)
{
	auto mod = getModInstance(L);
	if (!mod)
	{
		lua_pushnumber(L, 300);
		lua_pushnumber(L, 300);
		return 2;
	}
	boost::recursive_mutex::scoped_lock l(mod->instanceLock);
	const char* str = luaL_checkstring(L, 1);
	if (str)
	{
		if (lua_isnumber(L, 2) && lua_isnumber(L, 3))
		{
			ImVec2& v = mod->instance.options.GetDefaultSize(str, ImVec2(lua_tonumber(L, 2), lua_tonumber(L, 3)));
			lua_pushnumber(L, v.x);
			lua_pushnumber(L, v.y);
		}
		else
		{
			ImVec2& v = mod->instance.options.GetDefaultSize(str, ImVec2(300,300));
			lua_pushnumber(L, v.x);
			lua_pushnumber(L, v.y);
		}
	}
	else
	{
		lua_pushnumber(L, 300);
		lua_pushnumber(L, 300);
	}
	return 2;
}

extern "C" int setWindowSize(lua_State* L)
{
	auto mod = getModInstance(L);
	if (!mod)
	{
		return 0;
	}
	boost::recursive_mutex::scoped_lock l(mod->instanceLock);
	const char* str = luaL_checkstring(L, 1);
	if (str)
	{
		if (lua_isnumber(L, 2) && lua_isnumber(L, 3))
		{
			float x = luaL_checknumber(L, 2);
			float y = luaL_checknumber(L, 3);

			auto i = mod->instance.options.windows_default_sizes.find(str);
			if (i != mod->instance.options.windows_default_sizes.end())
			{
				i->second.x = x;
				i->second.y = y;
			}
		}
	}
	return 0;
}

extern "C" int getImage(lua_State* L)
{
	auto mod = getModInstance(L);
	if (!mod)
	{
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		return 6;
	}
	boost::recursive_mutex::scoped_lock l(mod->instanceLock);
	const char* str = luaL_checkstring(L, 1);
	if (str)
	{
		auto i = mod->instance.overlay_images.find(str);
		if (i != mod->instance.overlay_images.end())
		{
			lua_pushnumber(L, i->second.width);
			lua_pushnumber(L, i->second.height);
			lua_pushnumber(L, i->second.uv0.x);
			lua_pushnumber(L, i->second.uv0.y);
			lua_pushnumber(L, i->second.uv1.x);
			lua_pushnumber(L, i->second.uv1.y);
		}
		else
		{
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
		}
	}
	return 6;
}

extern "C" int getStyleColor4(lua_State* L)
{
	int idx = luaL_checkint(L, 1);
	if (ImGui::GetCurrentContext())
	{
		if (idx < ImGuiCol_COUNT)
		{
			ImVec4& v = ImGui::GetCurrentContext()->Style.Colors[idx];
			lua_pushnumber(L, v.x);
			lua_pushnumber(L, v.y);
			lua_pushnumber(L, v.z);
			lua_pushnumber(L, v.w);
			return 4;
		}
	}
	ImVec4 v(0, 0, 0, 1);
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	lua_pushnumber(L, v.z);
	lua_pushnumber(L, v.w);
	return 4;
}
extern "C" int setStyleColor4(lua_State* L)
{
	int idx = luaL_checkint(L, 1);
	if (idx < ImGuiCol_COUNT &&
		lua_isnumber(L, 2) &&
		lua_isnumber(L, 3) &&
		lua_isnumber(L, 4) &&
		lua_isnumber(L, 5))
	{
		if (ImGui::GetCurrentContext())
		{
			ImVec4& v = ImGui::GetCurrentContext()->Style.Colors[idx];
			const float v0 = luaL_checknumber(L, 2);
			const float v1 = luaL_checknumber(L, 3);
			const float v2 = luaL_checknumber(L, 4);
			const float v3 = luaL_checknumber(L, 5);

			v.x = v0;
			v.y = v1;
			v.z = v2;
			v.w = v3;
		}
		return 0;
	}
	return 0;
}
extern "C" int getColor4(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->color_map.find(str);
		if (i != storage->color_map.end())
		{
			ImVec4& v = i->second;
			lua_pushnumber(L, v.x);
			lua_pushnumber(L, v.y);
			lua_pushnumber(L, v.z);
			lua_pushnumber(L, v.w);
			return 4;
		}
	}
	ImVec4 v(0,0,0,1);
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	lua_pushnumber(L, v.z);
	lua_pushnumber(L, v.w);
	return 4;
}
extern "C" int setColor4(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str &&
		lua_isnumber(L, 3) &&
		lua_isnumber(L, 4) &&
		lua_isnumber(L, 5) &&
		lua_isnumber(L, 6) &&
		storage)
	{
		auto i = storage->color_map.find(str);
		if (i != storage->color_map.end())
		{
			const float v0 = luaL_checknumber(L, 3);
			const float v1 = luaL_checknumber(L, 4);
			const float v2 = luaL_checknumber(L, 5);
			const float v3 = luaL_checknumber(L, 6);
			i->second.x = v0;
			i->second.y = v1;
			i->second.z = v2;
			i->second.w = v3;
			return 0;
		}
	}
	return 0;
}
extern "C" int getColor3(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->color_map.find(str);
		if (i != storage->color_map.end())
		{
			ImVec4& v = i->second;
			lua_pushnumber(L, v.x);
			lua_pushnumber(L, v.y);
			lua_pushnumber(L, v.z);
			return 3;
		}
	}
	ImVec4 v(0, 0, 0, 1);
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	lua_pushnumber(L, v.z);
	return 3;
}
extern "C" int setColor3(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str &&
		lua_isnumber(L, 3) &&
		lua_isnumber(L, 4) &&
		lua_isnumber(L, 5) &&
		storage)
	{
		auto i = storage->color_map.find(str);
		if (i != storage->color_map.end())
		{
			const float v0 = luaL_checknumber(L, 3);
			const float v1 = luaL_checknumber(L, 4);
			const float v2 = luaL_checknumber(L, 5);
			i->second.x = v0;
			i->second.y = v1;
			i->second.z = v2;
			return 0;
		}
	}
	return 0;
}
extern "C" int getString(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->string_map.find(str);
		if (i != storage->string_map.end())
		{
			std::string& v = i->second;
			lua_pushlstring(L, v.c_str(), v.size());
			return 1;
		}
	}
	std::string v;
	lua_pushlstring(L, v.c_str(), v.size());
	return 1;
}
extern "C" int setString(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	const char* v = luaL_checkstring(L, 3);
	if (v && str && storage)
	{
		auto i = storage->string_map.find(str);
		if (i != storage->string_map.end())
		{
			i->second = v;
			return 0;
		}
	}
	return 0;
}
extern "C" int getInt(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->int_map.find(str);
		if (i != storage->int_map.end())
		{
			int& v = i->second;
			lua_pushnumber(L, v);
			return 1;
		}
	}
	int v = 0;
	lua_pushnumber(L, v);
	return 1;
}
extern "C" int setInt(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (lua_isnumber(L, 3) && str && storage)
	{
		const int v = lua_tonumber(L, 3);
		auto i = storage->int_map.find(str);
		if (i != storage->int_map.end())
		{
			i->second = v;
			return 0;
		}
	}
	return 0;
}
extern "C" int getFloat(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->float_map.find(str);
		if (i != storage->float_map.end())
		{
			float& v = i->second;
			lua_pushnumber(L, v);
			return 1;
		}
	}
	float v = 0.0f;
	lua_pushnumber(L, v);
	return 1;
}
extern "C" int setFloat(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (lua_isnumber(L, 3) && str && storage)
	{
		const float v = lua_tonumber(L, 3);
		auto i = storage->float_map.find(str);
		if (i != storage->float_map.end())
		{
			i->second = v;
			return 0;
		}
	}
	return 0;
}
extern "C" int getBoolean(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->boolean_map.find(str);
		if (i != storage->boolean_map.end())
		{
			bool& v = i->second;
			lua_pushboolean(L, v);
			return 1;
		}
	}
	bool v = false;
	lua_pushboolean(L, v);
	return 1;
}
extern "C" int setBoolean(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (lua_isboolean(L, 3) && str && storage)
	{
		const bool v = lua_toboolean(L, 3);
		auto i = storage->boolean_map.find(str);
		if (i != storage->boolean_map.end())
		{
			i->second = v;
			return 0;
		}
	}
	return 0;
}


extern "C" ModInterface* ModCreate()
{
	return new ModInstance();
}

extern "C" int ModFree(ModInterface* context)
{
	if (context)
		delete context;
	return 0;
}

int ModInstance::Init(ImGuiContext* context)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	ImGui::SetCurrentContext(context);
	if (context)
	{
		context->IO.Fonts->AddFontDefault();

	}
	WCHAR result[MAX_PATH] = {};
	GetModuleFileNameW(NULL, result, MAX_PATH);
	boost::filesystem::path module_path = result;
	boost::filesystem::path module_dir = module_path.parent_path();
	instance.Init(context, module_dir);
	instance.instance = this;
	return 0;
}

int ModInstance::UnInit(ImGuiContext* context)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	ImGui::SetCurrentContext(context);
	try {
		//instance.UnloadScripts();
	}
	catch (...)
	{

	}
	return 0;
}

void ModInstance::TextureData(int index, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	assert(out_pixels != nullptr && out_width != nullptr && out_height != nullptr);
	//*out_pixels = nullptr;
	//*out_width = 0;
	//*out_height = 0;
	//return;
	*out_pixels = instance.overlay_texture_filedata;
	*out_width = instance.overlay_texture_width;
	*out_height = instance.overlay_texture_height;
	if (out_bytes_per_pixel)
		*out_bytes_per_pixel = instance.overlay_texture_channels;
}

void ModInstance::SetTexture(int index, void* texture)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	instance.SetTexture(texture);
	if (texture == nullptr)
	{
	}
}

bool ModInstance::GetTextureDirtyRect(int index, int dindex, RECT* rect)
{
	return false;
}

int ModInstance::TextureBegin()
{
	return 1; // texture Size and lock
}

void ModInstance::TextureEnd()
{
	return;
}

int ModInstance::Render(ImGuiContext* context)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	ImGui::SetCurrentContext(context);
	instance.Render(context);
	ImGuiContext& g = *context;
	if (g.CurrentWindow && g.CurrentWindow->WriteAccessed)
	{
		// debug
		if (ImGui::Button("Reload Overlays"))
		{
			instance.ReloadScripts();
		}
	}


	return 0;
}

bool ModInstance::Menu(bool* show)
{
	if (show)
		instance.options.show_preferences = *show;
	else
		instance.options.show_preferences = !instance.options.show_preferences;
	return instance.options.show_preferences;
}


bool ModInstance::UpdateFont(ImGuiContext* context)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	if (instance.font_setting_dirty)
	{
		ImGui::SetCurrentContext(context);
		instance.context = context;
		instance.LoadFonts();
		return true;
	}
	//if (instance.texture_setting_dirty)
	//{
	//	instance.context = context;
	//	instance.LoadTexture();
	//}
	return false;
}

void OverlayInstance::SetTexture(ImTextureID texture)
{
	overlay_texture = texture;
	for (auto i = overlays.begin(); i != overlays.end(); ++i) {
		if (i->second->IsLoaded())
		{
			i->second->SetTexture(texture, &overlay_images);
		}
	}
}

OverlayInstance::OverlayInstance()
{
	options.show_preferences = false;
	style_colors = {
		{ "Text",                  &imgui_style.Colors[ImGuiCol_Text] },
		{ "TextDisabled",          &imgui_style.Colors[ImGuiCol_TextDisabled] },
		{ "WindowBg",              &imgui_style.Colors[ImGuiCol_WindowBg] },
		{ "ChildWindowBg",         &imgui_style.Colors[ImGuiCol_ChildWindowBg] },
		{ "PopupBg",               &imgui_style.Colors[ImGuiCol_PopupBg] },
		{ "Border",                &imgui_style.Colors[ImGuiCol_Border] },
		{ "BorderShadow",          &imgui_style.Colors[ImGuiCol_BorderShadow] },
		{ "FrameBg",               &imgui_style.Colors[ImGuiCol_FrameBg] },
		{ "FrameBgHovered",        &imgui_style.Colors[ImGuiCol_FrameBgHovered] },
		{ "FrameBgActive",         &imgui_style.Colors[ImGuiCol_FrameBgActive] },
		{ "TitleBg",               &imgui_style.Colors[ImGuiCol_TitleBg] },
		{ "TitleBgCollapsed",      &imgui_style.Colors[ImGuiCol_TitleBgCollapsed] },
		{ "TitleBgActive",         &imgui_style.Colors[ImGuiCol_TitleBgActive] },
		{ "MenuBarBg",             &imgui_style.Colors[ImGuiCol_MenuBarBg] },
		{ "ScrollbarBg",           &imgui_style.Colors[ImGuiCol_ScrollbarBg] },
		{ "ScrollbarGrab",         &imgui_style.Colors[ImGuiCol_ScrollbarGrab] },
		{ "ScrollbarGrabHovered",  &imgui_style.Colors[ImGuiCol_ScrollbarGrabHovered] },
		{ "ScrollbarGrabActive",   &imgui_style.Colors[ImGuiCol_ScrollbarGrabActive] },
		//{ "ComboBg",               &imgui_style.Colors[ImGuiCol_PopupBg] },
		{ "CheckMark",             &imgui_style.Colors[ImGuiCol_CheckMark] },
		{ "SliderGrab",            &imgui_style.Colors[ImGuiCol_SliderGrab] },
		{ "SliderGrabActive",      &imgui_style.Colors[ImGuiCol_SliderGrabActive] },
		{ "Button",                &imgui_style.Colors[ImGuiCol_Button] },
		{ "ButtonHovered",         &imgui_style.Colors[ImGuiCol_ButtonHovered] },
		{ "ButtonActive",          &imgui_style.Colors[ImGuiCol_ButtonActive] },
		{ "Header",                &imgui_style.Colors[ImGuiCol_Header] },
		{ "HeaderHovered",         &imgui_style.Colors[ImGuiCol_HeaderHovered] },
		{ "HeaderActive",          &imgui_style.Colors[ImGuiCol_HeaderActive] },
		{ "Column",                &imgui_style.Colors[ImGuiCol_Column] },
		{ "ColumnHovered",         &imgui_style.Colors[ImGuiCol_ColumnHovered] },
		{ "ColumnActive",          &imgui_style.Colors[ImGuiCol_ColumnActive] },
		{ "ResizeGrip",            &imgui_style.Colors[ImGuiCol_ResizeGrip] },
		{ "ResizeGripHovered",     &imgui_style.Colors[ImGuiCol_ResizeGripHovered] },
		{ "ResizeGripActive",      &imgui_style.Colors[ImGuiCol_ResizeGripActive] },
		{ "CloseButton",           &imgui_style.Colors[ImGuiCol_Button] },
		{ "CloseButtonHovered",    &imgui_style.Colors[ImGuiCol_ButtonHovered] },
		{ "CloseButtonActive",     &imgui_style.Colors[ImGuiCol_ButtonActive] },
		{ "PlotLines",             &imgui_style.Colors[ImGuiCol_PlotLines] },
		{ "PlotLinesHovered",      &imgui_style.Colors[ImGuiCol_PlotLinesHovered] },
		{ "PlotHistogram",         &imgui_style.Colors[ImGuiCol_PlotHistogram] },
		{ "PlotHistogramHovered",  &imgui_style.Colors[ImGuiCol_PlotHistogramHovered] },
		{ "TextSelectedBg",        &imgui_style.Colors[ImGuiCol_TextSelectedBg] },
		{ "ModalWindowDarkening",  &imgui_style.Colors[ImGuiCol_ModalWindowDarkening] },
	};
	preference_nodes = {
		//{ "Styles", {
		//	{ "GrabMinSize",               PreferenceNode::Type::Float, &imgui_style.GrabMinSize },
		//	{ "GrabRounding",              PreferenceNode::Type::Float, &imgui_style.GrabRounding },
		//	{ "WindowRounding",            PreferenceNode::Type::Float, &imgui_style.WindowRounding },
		//}
		//},
		{ "Colors",{
			{ "Text",                  PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Text] },
			{ "TextDisabled",          PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_TextDisabled] },
			{ "WindowBg",              PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_WindowBg] },
			{ "ChildWindowBg",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ChildWindowBg] },
			{ "PopupBg",               PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PopupBg] },
			{ "Border",                PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Border] },
			{ "BorderShadow",          PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_BorderShadow] },
			{ "FrameBg",               PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_FrameBg] },
			{ "FrameBgHovered",        PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_FrameBgHovered] },
			{ "FrameBgActive",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_FrameBgActive] },
			{ "TitleBg",               PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_TitleBg] },
			{ "TitleBgCollapsed",      PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_TitleBgCollapsed] },
			{ "TitleBgActive",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_TitleBgActive] },
			{ "MenuBarBg",             PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_MenuBarBg] },
			{ "ScrollbarBg",           PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ScrollbarBg] },
			{ "ScrollbarGrab",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ScrollbarGrab] },
			{ "ScrollbarGrabHovered",  PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ScrollbarGrabHovered] },
			{ "ScrollbarGrabActive",   PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ScrollbarGrabActive] },
			//{ "ComboBg",               PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PopupBg] },
			{ "CheckMark",             PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_CheckMark] },
			{ "SliderGrab",            PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_SliderGrab] },
			{ "SliderGrabActive",      PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_SliderGrabActive] },
			{ "Button",                PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Button] },
			{ "ButtonHovered",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ButtonHovered] },
			{ "ButtonActive",          PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ButtonActive] },
			{ "Header",                PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Header] },
			{ "HeaderHovered",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_HeaderHovered] },
			{ "HeaderActive",          PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_HeaderActive] },
			{ "Column",                PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Column] },
			{ "ColumnHovered",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ColumnHovered] },
			{ "ColumnActive",          PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ColumnActive] },
			{ "ResizeGrip",            PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ResizeGrip] },
			{ "ResizeGripHovered",     PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ResizeGripHovered] },
			{ "ResizeGripActive",      PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ResizeGripActive] },
			{ "CloseButton",           PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Button] },
			{ "CloseButtonHovered",    PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ButtonHovered] },
			{ "CloseButtonActive",     PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ButtonActive] },
			{ "PlotLines",             PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PlotLines] },
			{ "PlotLinesHovered",      PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PlotLinesHovered] },
			{ "PlotHistogram",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PlotHistogram] },
			{ "PlotHistogramHovered",  PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PlotHistogramHovered] },
			{ "TextSelectedBg",        PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_TextSelectedBg] },
			{ "ModalWindowDarkening",  PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ModalWindowDarkening] },
		},
		}
	};


}

OverlayInstance::~OverlayInstance()
{
	overlays.clear();
}

void OverlayInstance::FontsPreferences() {
	if (ImGui::TreeNode("Fonts"))
	{
		ImGui::Text("Default font is \'Default\' with fixed font size 13.0");
		{
			std::vector<const char*> data;
			data.reserve(fonts.size());
			for (int i = 0; i < fonts.size(); ++i)
			{
				data.push_back(fonts[i].fontname.c_str());
			}

			static int index_ = -1;
			static char buf[100] = { 0, };
			static int current_item = -1;
			static float font_size = 12.0f;
			static int glyph_range = 0;
			static int fontname_idx = -1;
			static int index = -1;
			bool decIndex = false;
			bool incIndex = false;
			static std::vector<const char *> font_cstr_filenames_prefix = font_cstr_filenames;
			if (ImGui::ListBox("Fonts", &index_, data.data(), data.size()))
			{
				index = index_;
				strcpy(buf, fonts[index].fontname.c_str());
				font_size = fonts[index].font_size;
				auto ri = std::find(glyph_range_key.begin(), glyph_range_key.end(), fonts[index].glyph_range);
				if (ri != glyph_range_key.end())
				{
					glyph_range = ri - glyph_range_key.begin();
				}
				const std::string val = boost::to_lower_copy(fonts[index].fontname);
				const std::string val2 = val + ".ttc";
				const std::string val3 = val + ".ttf";
				font_cstr_filenames_prefix.clear();
				for (auto k = font_cstr_filenames.begin(); k != font_cstr_filenames.end(); ++k)
				{
					if (boost::starts_with(boost::to_lower_copy(std::string(*k)), val))
						font_cstr_filenames_prefix.push_back(*k);
				}
				auto fi = std::find_if(font_cstr_filenames.begin(), font_cstr_filenames.end(), [&val, &val2, &val3](const std::string& a) {
					return val == boost::to_lower_copy(a) || val2 == boost::to_lower_copy(a) || val3 == boost::to_lower_copy(a);
				});
				if (fi != font_cstr_filenames.end())
				{
					fontname_idx = fi - font_cstr_filenames.begin();
				}
				else
				{
					fontname_idx = -1;
				}
			}
			if (ImGui::Button("Up"))
			{
				if (index > 0)
				{
					std::swap(fonts[index], fonts[index - 1]);
					Save();
					decIndex = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Down"))
			{
				if (index + 1 < fonts.size())
				{
					std::swap(fonts[index], fonts[index + 1]);
					Save();
					incIndex = true;
				}
			}
			ImGui::SameLine();
			//if (ImGui::Button("Edit"))
			//{
			//	if(index_ >= 0)
			//		ImGui::OpenPopup("Edit Column");
			//}
			//ImGui::SameLine();

			if (ImGui::Button("Remove"))
			{
				if (index >= 0)
				{
					fonts.erase(fonts.begin() + index);
					Save();
				}
				if (index >= fonts.size())
				{
					decIndex = true;
				}
			}
			if (decIndex)
			{
				--index_;
				index = index_;
				if (index >= 0)
				{
					strcpy(buf, fonts[index].fontname.c_str());
					font_size = fonts[index].font_size;
					auto ri = std::find(glyph_range_key.begin(), glyph_range_key.end(), fonts[index].glyph_range);
					if (ri != glyph_range_key.end())
					{
						glyph_range = ri - glyph_range_key.begin();
					}
				}
				else
				{
					strcpy(buf, "");
					font_size = 15.0f;
					glyph_range = 0;
				}
			}
			if (incIndex)
			{
				++index_;
				index = index_;
				if (index < fonts.size())
				{
					strcpy(buf, fonts[index].fontname.c_str());
					font_size = fonts[index].font_size;
					auto ri = std::find(glyph_range_key.begin(), glyph_range_key.end(), fonts[index].glyph_range);
					if (ri != glyph_range_key.end())
					{
						glyph_range = ri - glyph_range_key.begin();
					}
				}
				else
				{
					strcpy(buf, "");
					font_size = 15.0f;
					glyph_range = 0;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Append"))
			{
				font_size = font_sizes;
				ImGui::OpenPopup("Append Column");
			}
			ImGui::SameLine();
			if (ImGui::Button("Apply"))
			{
				font_setting_dirty = true;
			}
			if (ImGui::BeginPopup("Append Column"))
			{
				static char buf[100] = { 0, };
				static int glyph_range = 0;
				static int fontname_idx = -1;
				static std::vector<const char *> font_cstr_filenames_prefix = font_cstr_filenames;
				if (ImGui::InputText("FontName", buf, 99))
				{
					std::string val = boost::to_lower_copy(std::string(buf));
					const std::string val2 = val + ".ttc";
					const std::string val3 = val + ".ttf";
					font_cstr_filenames_prefix.clear();
					for (auto k = font_cstr_filenames.begin(); k != font_cstr_filenames.end(); ++k)
					{
						if (boost::starts_with(boost::to_lower_copy(std::string(*k)), val))
							font_cstr_filenames_prefix.push_back(*k);
					}
					auto fi = std::find_if(font_cstr_filenames.begin(), font_cstr_filenames.end(), [&val, &val2, &val3](const std::string& a) {
						return val == boost::to_lower_copy(a) || val2 == boost::to_lower_copy(a) || val3 == boost::to_lower_copy(a);
					});
					if (fi != font_cstr_filenames.end())
					{
						fontname_idx = fi - font_cstr_filenames.begin();
					}
					else
					{
						fontname_idx = -1;
					}
				}
				if (ImGui::ListBox("FontName", &fontname_idx, font_cstr_filenames_prefix.data(), font_cstr_filenames_prefix.size(), 10))
				{
					if (fontname_idx >= 0)
					{
						strcpy(buf, font_cstr_filenames_prefix[fontname_idx]);
					}
				}
				if (ImGui::Combo("GlyphRange", &glyph_range, glyph_range_key.data(), glyph_range_key.size()))
				{
				}
				if (ImGui::InputFloat("Size", &font_size, 0.5f))
				{
					font_size = std::min(std::max(font_size, 6.0f), 30.0f);
				}
				if (ImGui::Button("Append"))
				{
					if (glyph_range >= 0)
					{
						Font font;
						font.fontname = buf;
						font.font_size = font_size;
						font.glyph_range = glyph_range_key[glyph_range];
						fonts.push_back(font);
						ImGui::CloseCurrentPopup();
						strcpy(buf, "");
						current_item = -1;
						font_size = font_sizes;
						Save();
					}
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine(0, 100);
			if (ImGui::Button("Reset"))
			{
				font_sizes = 13;
				fonts = {
					Font("consolab.ttf", "Default", font_sizes),
					Font("Default", "Default", font_sizes), // Default will ignore font size.
					Font("ArialUni.ttf", "Japanese", font_sizes),
					Font("NanumBarunGothic.ttf", "Korean", font_sizes),
					Font("gulim.ttc", "Korean", font_sizes),
				};
			}

			//if (ImGui::BeginPopup("Edit Column"))
			if (index >= 0)
			{
				ImGui::BeginGroup();
				ImGui::Text("Edit");
				ImGui::Separator();
				//font_cstr_filenames;
				if (ImGui::InputText("FontName", buf, 99))
				{
					fonts[index].fontname = buf;
					std::string val = boost::to_lower_copy(fonts[index].fontname);
					const std::string val2 = val + ".ttc";
					const std::string val3 = val + ".ttf";
					font_cstr_filenames_prefix.clear();
					for (auto k = font_cstr_filenames.begin(); k != font_cstr_filenames.end(); ++k)
					{
						if (boost::starts_with(boost::to_lower_copy(std::string(*k)), val))
							font_cstr_filenames_prefix.push_back(*k);
					}
					auto fi = std::find_if(font_cstr_filenames.begin(), font_cstr_filenames.end(), [&val, &val2, &val3](const std::string& a) {
						return val == boost::to_lower_copy(a) || val2 == boost::to_lower_copy(a) || val3 == boost::to_lower_copy(a);
					});
					if (fi != font_cstr_filenames.end())
					{
						fontname_idx = fi - font_cstr_filenames.begin();
					}
					else
					{
						fontname_idx = -1;
					}
					Save();
				}

				if (ImGui::ListBox("FontName", &fontname_idx, font_cstr_filenames_prefix.data(), font_cstr_filenames_prefix.size(), 10))
				{
					if (fontname_idx >= 0)
					{
						fonts[index].fontname = font_cstr_filenames_prefix[fontname_idx];
						strcpy(buf, font_cstr_filenames_prefix[fontname_idx]);
						Save();
					}
				}
				if (ImGui::Combo("GlyphRange", &glyph_range, glyph_range_key.data(), glyph_range_key.size()))
				{
					if (glyph_range >= 0)
					{
						fonts[index].glyph_range = glyph_range_key[glyph_range];
						Save();
					}
				}
				if (ImGui::InputFloat("Size", &font_size, 0.5f))
				{
					font_size = std::min(std::max(font_size, 6.0f), 30.0f);
					fonts[index].font_size = font_size;
					Save();
				}
				ImGui::Separator();
				ImGui::EndGroup();
			}
			if (ImGui::InputFloat("FontSizes (for all)", &font_sizes, 0.5f))
			{
				font_sizes = std::min(std::max(font_sizes, 6.0f), 30.0f);
				for (auto i = fonts.begin(); i != fonts.end(); ++i)
				{
					i->font_size = font_sizes;
				}
				font_size = font_sizes;
				Save();
			}
		}
		ImGui::TreePop();
	}
}

void OverlayInstance::Preferences() {
	char buf[256] = { 0, };
	sprintf_s(buf, 256, "Preferences");
	options.show_preferences = true;
	ImGui::SetNextWindowPosCenter(ImGuiSetCond_Appearing);
	if (ImGui::Begin(buf, &options.show_preferences, options.GetDefaultSize("Preferences", ImVec2(300,300)), -1, ImGuiWindowFlags_NoCollapse))
	{
		ImGui::Text("ACTWebSocketOverlay - %s", VERSION_LONG_STRING);
		ImGui::Separator();
		options.windows_default_sizes["Preferences"] = ImGui::GetWindowSize();
		if (ImGui::TreeNodeEx("Global", ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen))
		{
			FontsPreferences();
			for (auto i = preference_nodes.begin(); i != preference_nodes.end(); ++i)
			{
				PreferenceBase::Preferences(*i, preference_storage);
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Overlays", ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginGroup();
			ImGui::Text("Visible");
			for (auto i = overlays.begin(); i != overlays.end(); ++i) {
				if (ImGui::Checkbox((std::string("Show ") + i->second->name).c_str(), &i->second->IsVisible()))
				{
					Save();
				}
			}
			ImGui::Separator();
			ImGui::Text("more...");
			for (auto i = overlays.begin(); i != overlays.end(); ++i) {
				if (i->second->IsLoaded())
				{
					ImGui::BeginGroup();
					i->second->Preferences();
					ImGui::EndGroup();
				}
			}
			ImGui::EndGroup();
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Reload"))
		{
			if (ImGui::Button("Reload Overlays"))
			{
				ReloadScripts();
			}
			ImGui::TreePop();
		}
		ImGui::Separator();
		if (ImGui::TreeNode("About"))
		{
			ImGui::Text("Version : %s", VERSION_LONG_STRING);
			ImGui::Text("Github : https://github.com/ZCube/ACTWebSocket");
			ImGui::Text("Github : https://github.com/ZCube/ACTWebSocketOverlay");
			ImGui::Text("");
			ImGui::Text("Path : %s", (root_path).string().c_str());
			ImGui::Text("Script Search : %s", (root_path / "*" / "script.json").string().c_str());
			ImGui::Text("");
			if (ImGui::TreeNode("Libraries"))
			{
#include "licenses.h"
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		ImGui::End();

	}
}

void OverlayInstance::Render(ImGuiContext * context)
{
	if (context != this->context)
	{
		ImGui::SetCurrentContext(context);
		this->context = context;
		if (!initialized)
		{
			Init(context, root_path);
		}
		else
		{
			Load();
		}
	}
	bool move_key_pressed = GetAsyncKeyState(VK_SHIFT) && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_MENU);
	bool use_input = options.movable || options.show_preferences || move_key_pressed;

	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiStyle backup = style;

	bool isVisible = false;
	for (auto i = overlays.begin(); i != overlays.end(); ++i) {
		style = imgui_style;
		current_storage = &i->second->preference_storage;
		if (i->second->IsVisible())
		{
			i->second->Render(use_input, &options);
			isVisible = true;
		}
	}
	current_storage = nullptr;
	style = imgui_style;
	if (options.show_preferences)
	{
		Preferences();
	}

	if(context)
	{
		auto& io = context->IO;
		if (!use_input)
		{
			io.WantCaptureMouse = false;
		}
		if (!options.show_preferences)
		{
			io.WantCaptureKeyboard = false;
		}
	}

	style = backup;
}

void OverlayInstance::Load() {
	Json::Reader r;
	Json::Value setting;
	try {
		std::ifstream fin(setting_path.wstring());
		options.show_preferences = true;
		if (r.parse(fin, setting))
		{
			options.show_preferences = false;
			preference_storage.FromJson(setting);

			{
				auto it = preference_storage.boolean_map.find("Movable");
				if (it != preference_storage.boolean_map.end())
				{
					options.movable = it->second;
				}
			}

			int count = 0;
			int transparent_count = 0;
			for (auto i = style_colors.begin(); i != style_colors.end(); ++i)
			{
				auto j = preference_storage.color_map.find(i->first);
				if (j != preference_storage.color_map.end())
				{
					if (j->second.w == 0)
						++transparent_count;
					++count;
				}
			}
			// load to style

			if (transparent_count != count)
			{
				for (auto i = style_colors.begin(); i != style_colors.end(); ++i)
				{
					auto j = preference_storage.color_map.find(i->first);
					if (j != preference_storage.color_map.end())
					{
						*i->second = j->second;
					}
				}
			}

			// global
			Json::Value fonts = setting.get("fonts", Json::nullValue);
			std::string fonts_str = "fonts";
			if (setting.find(&*fonts_str.begin(), &*fonts_str.begin() + fonts_str.size()) != nullptr)
			{
				if (fonts.size() > 0)
				{
					this->fonts.clear();
					for (auto i = fonts.begin();
						i != fonts.end();
						++i)
					{
						Font font;
						font.FromJson(*i);
						this->fonts.push_back(font);
					}
				}
			}
			// global
			Json::Value windows = setting.get("windows", Json::nullValue);
			std::string windows_str = "windows";

			if (setting.find(&*windows_str.begin(), &*windows_str.begin() + windows_str.size()) != nullptr)
			{
				if (windows.size() > 0)
				{
					Json::Value;
					for (auto i = windows.begin();
						i != windows.end();
						++i)
					{
						Json::Value& win = *i;
						std::string name = win["name"].asString();
						if (name.empty())
							continue;
						ImVec2 pos = ImVec2(win["x"].asFloat(), win["y"].asFloat());
						ImVec2 size = ImVec2(win["width"].asFloat(), win["height"].asFloat());
						options.windows_default_sizes[name] = size;
						size = ImMax(size, context->Style.WindowMinSize);
						size = ImMax(size, ImVec2(100, 50));
						if (pos.x < 0)
							pos.x = 0;
						if (pos.y < 0)
							pos.y = 0;

						ImGuiWindow* window = ImGui::FindWindowByName(name.c_str());
						if (window)
						{
							ImGui::SetWindowPos(name.c_str(), ImVec2(pos.x, pos.y), ImGuiSetCond_Always);
							ImGui::SetWindowSize(name.c_str(), ImVec2(size.x, size.y), ImGuiSetCond_Always);
						}
						else
						{
							if (context)
							{
								ImGuiWindowSettings * settings = nullptr;
								ImGuiContext & g = *context;
								g.Initialized = true;
								ImGuiID id = ImHash(name.c_str(), 0);
								{
									for (int i = 0; i != g.SettingsWindows.Size; ++i)
									{
										ImGuiWindowSettings* ini = &g.SettingsWindows[i];
										if (ini->Id == id)
										{
											settings = ini;
											break;
										}
									}
									if (settings == nullptr)
									{
										g.SettingsWindows.push_back(ImGuiWindowSettings());
										ImGuiWindowSettings* ini = &g.SettingsWindows.back();
										ini->Name = ImStrdup(name.c_str());
										ini->Id = ImHash(name.c_str(), 0);
										ini->Collapsed = false;
										ini->Pos = ImVec2(FLT_MAX, FLT_MAX);
										ini->Size = ImVec2(0, 0);
										settings = ini;
									}
								}
								if (settings)
								{
									if (pos.x < 0)
										pos.x = 0;
									if (pos.y < 0)
										pos.y = 0;
									settings->Pos = pos;
									settings->Size = size;
								}
							}
						}
					}
				}
			}
		}
		for (auto i = overlays.begin(); i != overlays.end(); ++i) {
			i->second->FromJson(setting);
			i->second->WebSocketCheck();
		}

		fin.close();
	}
	catch (...)
	{
	}
}

void OverlayInstance::Save() {
	Json::StyledWriter w;
	Json::Value setting;
	Json::Value color;

	// save from style
	for (auto i = style_colors.begin(); i != style_colors.end(); ++i)
	{
		preference_storage.color_map[i->first] = *i->second;
	}

	preference_storage.boolean_map["Movable"] = options.movable;

	preference_storage.ToJson(setting);

	Json::Value fonts;
	for (int i = 0; i < this->fonts.size(); ++i)
	{
		Json::Value font;
		this->fonts[i].ToJson(font);
		fonts.append(font);
	}
	setting["fonts"] = fonts;

	Json::Value windows;
	//if(context)
	//{
	//	ImGuiContext& g = *context;
	//	{
	//		for (int i = 0; i != g.Windows.Size; ++i)
	//		{
	//			ImGuiWindow* window = g.Windows[i];
	//			if (window->Flags & ImGuiWindowFlags_NoSavedSettings)
	//				continue;
	//			Json::Value win;
	//			win["name"] = window->Name;
	//			win["x"] = window->Pos.x;
	//			win["y"] = window->Pos.y;
	//			win["width"] = window->SizeFull.x;
	//			win["height"] = window->SizeFull.y;
	//			windows[window->Name] = win;
	//		}
	//	}
	//}
	for (auto pos = options.windows_default_pos.begin(); pos != options.windows_default_pos.end();++pos)
	{
		const std::string name = pos->first;
		Json::Value win;
		win["name"] = name;
		win["x"] = pos->second.x;
		win["y"] = pos->second.y;
		ImVec2& size = options.windows_default_sizes[name];
		size = ImMax(size, context->Style.WindowMinSize);
		win["width"] = size.x;
		win["height"] = size.y;
		windows[name] = win;
	}
	setting["windows"] = windows;

	for (auto i = overlays.begin(); i != overlays.end(); ++i) {
		i->second->ToJson(setting);
	}
	std::ofstream fout(setting_path.wstring());
	fout << w.write(setting);
	fout.close();
}

void OverlayInstance::Init(ImGuiContext * context, const boost::filesystem::path & path) {
	root_path = path;
	setting_path = root_path / "mod.json";
	this->context = context;
	if (!initialized)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		imgui_style = style;
		Load();
		LoadFonts();
		boost::mutex::scoped_lock l(m);
		LoadOverlays();
		Load();
		initialized = true;
	}
	else
	{
		font_setting_dirty = true;
		ReloadScripts();
	}
	LoadTexture();
	SetTexture(overlay_texture);
}

void OverlayInstance::ReloadScripts() {
	overlays.clear();
	ClearUniqueValues();
	LoadOverlays();
	SetTexture(overlay_texture);
	Load();
}

void OverlayInstance::UnloadScripts() {
	overlays.clear();
	ClearUniqueValues();
}

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u)  ((void)(u), ImGui::MemAlloc(x))
#define STBTT_free(x,u)    ((void)(u), ImGui::MemFree(x))
#include "stb_truetype.h"

void GetImages(const boost::filesystem::path& image_dir, std::vector<boost::filesystem::path>& image_paths)
{
	try {
		boost::filesystem::directory_iterator itr, end_itr;
		for (boost::filesystem::directory_iterator itr(image_dir); itr != end_itr; ++itr) {
			if (boost::filesystem::exists(*itr) && boost::filesystem::is_regular_file(*itr))
			{
				std::string extension = boost::to_lower_copy(itr->path().extension().string());
				if (extension == ".png")
				{
					image_paths.push_back(itr->path());
				}
			}
		}
	}
	catch (...)
	{
	}
}

void OverlayInstance::LoadTexture()
{
	boost::filesystem::directory_iterator itr, end_itr;

	std::map<std::string, std::vector<boost::filesystem::path>> image_paths;
	std::vector<std::vector<std::string>> image_names;

	for (auto i = overlays.begin(); i != overlays.end(); ++i) {
		GetImages(i->second->GetImagesPath(), image_paths[i->second->name]);
	}
	std::vector<Image> image_infos;
	std::vector<stbrp_rect> image_rects;
	std::vector<uint8_t*> image_bytes;
	for (auto i = overlays.begin(); i != overlays.end(); ++i) {
		auto & paths = image_paths[i->second->name];
		for (auto j = paths.begin(); j != paths.end(); ++j) {
			int width = 0;
			int height = 0;
			int channels = 0;
			{
				FILE *file = nullptr;
				uint8_t* image = nullptr;
				if (_wfopen_s(&file, j->wstring().c_str(), L"rb") == 0)
				{
					image = stbi_load_from_file(file, &width, &height, &channels, STBI_rgb_alpha);
					fclose(file);
					if (image)
					{
						boost::filesystem::path rel = boost::filesystem::relative(*j, i->second->GetImagesPath());
						rel = rel.replace_extension("");
						std::string rel_name = boost::to_lower_copy(std::string(CW2A(rel.wstring().c_str(), CP_UTF8)));

						boost::filesystem::path p = j->filename();
						p = p.replace_extension("");
						std::string name = boost::to_lower_copy(std::string(CW2A(p.wstring().c_str(), CP_UTF8)));
						image_names.push_back({
							name,
							i->second->name + ":" + name,
							rel_name,
							i->second->name + ":" + rel_name
						});

						for (UINT i = 0; i < width*height; i += 4)
							std::swap(image[i], image[i + 2]);

						stbrp_rect rt;
						rt.w = width;
						rt.h = height;
						rt.id = image_names.size() - 1;
						rt.x = 0;
						rt.y = 0;
						image_rects.push_back(rt);
						image_bytes.push_back(image);
					}
				}
			}
		}
	}

	if (image_rects.size())
	{
		stbrp_context context;

		int s = 256;
		bool packed_all = false;
		for (int i = 0; i < image_rects.size(); ++i)
		{
			do
			{
				if (image_rects[i].x < s && image_rects[i].y < s)
					break;
				s = s * 2;
			} while (true);
		}
		do
		{
			std::vector<stbrp_node> nodes;
			nodes.resize(image_rects.size());

			for (int i = 0; i < image_rects.size(); ++i)
			{
				image_rects[i].was_packed = 0;
			}

			stbrp_init_target(&context, s, s, nodes.data(), image_rects.size());
			stbrp_pack_rects(&context, image_rects.data(), image_rects.size());

			packed_all = true;
			for (int i = 0; i < image_rects.size(); ++i)
			{
				if (!image_rects[i].was_packed)
				{
					packed_all = false;
					break;
				}
			}
			if (packed_all)
				break;
			s *= 2;
		} while (true);

		atlas_image.resize(s * s * 4, 0);
		const int channels = 4;
		for (int i = 0; i < image_rects.size(); ++i)
		{
			const int& height = image_rects[i].h;
			const int& width = image_rects[i].w;
			for (int j = 0; j < height; ++j)
			{
				//const int cwidth = width*channels;
				RGBQUAD* src = (RGBQUAD*)(image_bytes[i] + width * (j)* channels);
				RGBQUAD* dst = (RGBQUAD*)(atlas_image.data() + ((s*(image_rects[i].y + j)) + image_rects[i].x)*channels);

				for (int k = 0; k < width; ++k)
				{
					dst->rgbRed = src->rgbRed;
					dst->rgbGreen = src->rgbGreen;
					dst->rgbBlue = src->rgbBlue;
					dst->rgbReserved = src->rgbReserved;
					++dst;
					++src;
				}
			}
		}
		for (int i = 0; i < image_rects.size(); ++i)
		{
			stbi_image_free(image_bytes[i]);
			Image im;
			im.x = image_rects[i].x;
			im.y = image_rects[i].y;
			im.width = image_rects[i].w;
			im.height = image_rects[i].h;
			im.uv0 = ImVec2(((float)im.x + 0.5f) / (float)s,
				((float)im.y + 0.5f) / (float)s);
			im.uv1 = ImVec2((float)(im.x + im.width - 1) / (float)s,
				(float)(im.y + im.height - 1) / (float)s);
			auto & names = image_names[image_rects[i].id];
			for (auto j = names.begin(); j != names.end(); ++j)
			{
				overlay_images[*j] = im;
			}
		}
		image_bytes.clear();

		overlay_texture_filedata = atlas_image.data();
		overlay_texture_width = s;
		overlay_texture_height = s;
		overlay_texture_channels = 4;
	}
	else
	{
		overlay_texture_filedata = nullptr;
		overlay_texture_width = 16;
		overlay_texture_height = 16;
		overlay_texture_channels = 4;
	}
}

void OverlayInstance::LoadOverlays()
{
	{
		auto awio = overlays["AWIO"] = std::make_shared<AWIOOverlay>();
		awio->SetRoot(this);
		if (awio->Init(root_path))
		{
			awio->WebSocketRun();
		}
	}
	for (boost::filesystem::directory_iterator itr(root_path); itr != boost::filesystem::directory_iterator(); ++itr) {
		if (boost::filesystem::is_directory(*itr))
		{
			std::string u8name = STR2UTF8((itr)->path().filename().string().c_str());
			auto script_path = (*itr) / "script.json";

			if (boost::filesystem::exists(script_path))
			{
				Json::Value value;
				Json::Reader reader;
				std::ifstream fin(script_path.wstring().c_str());
				if (reader.parse(fin, value))
				{
					if (value["Type"].asString() == "Lua")
					{
						auto& overlay = overlays[u8name] = std::make_shared<OverlayContextLua>();
						overlay->SetRoot(this);
						if (overlay->Init((*itr)))
						{
							overlay->WebSocketRun();
						}
					}
				}
			}
		}
	}
}

void OverlayInstance::LoadFonts()
{
	// Changing or adding fonts at runtime causes a crash.
	// TODO: is it possible ?...
	if (!context || (context && !context->IO.Fonts))
		return;
	ImGuiIO& io = context->IO;

	WCHAR result[MAX_PATH] = {};
	GetWindowsDirectoryW(result, MAX_PATH);
	boost::filesystem::path windows_dir = result;

	// font
	io.Fonts->Clear();

	std::map<std::string, const ImWchar*> glyph_range_map = {
		{ "Default", io.Fonts->GetGlyphRangesDefault() },
		{ "Chinese", io.Fonts->GetGlyphRangesChinese() },
		{ "Cyrillic", io.Fonts->GetGlyphRangesCyrillic() },
		{ "Japanese", io.Fonts->GetGlyphRangesJapanese() },
		{ "Korean", io.Fonts->GetGlyphRangesKorean() },
		{ "Thai", io.Fonts->GetGlyphRangesThai() },
	};

	// Add fonts in this order.
	glyph_range_key = {
		"Default",
		"Chinese",
		"Cyrillic",
		"Japanese",
		"Korean",
		"Thai",
	};

	std::vector<boost::filesystem::path> font_find_folders = {
		root_path,
		root_path / "Fonts",
		windows_dir / "Fonts", // windows fonts
	};

	ImFontConfig config;
	config.MergeMode = false;


	font_paths.clear();

	// first time
	if (font_filenames.empty())
	{
		font_filenames.clear();
		font_cstr_filenames.clear();
		boost::filesystem::directory_iterator itr, end_itr;

		font_filenames.push_back("Default");
		for (auto i = font_find_folders.begin(); i != font_find_folders.end(); ++i)
		{
			if (boost::filesystem::exists(*i))
			{
				for (boost::filesystem::directory_iterator itr(*i); itr != end_itr; ++itr)
				{
					if (is_regular_file(itr->path())) {
						std::string extension = boost::to_lower_copy(itr->path().extension().string());
						if (extension == ".ttc" || extension == ".ttf")
						{
							font_paths.push_back(itr->path());
							font_filenames.push_back(itr->path().filename().string());
						}
					}
				}
			}
		}
		{
			for (auto i = font_filenames.begin(); i != font_filenames.end(); ++i)
			{
				font_cstr_filenames.push_back(i->c_str());
			}
		}
	}

	for (auto i = glyph_range_key.begin(); i != glyph_range_key.end(); ++i)
	{
		bool is_loaded = false;

		for (auto j = fonts.begin(); j != fonts.end() && !is_loaded; ++j)
		{
			if (j->glyph_range == *i)
			{
				if (j->fontname == "Default")
				{
					io.Fonts->AddFontDefault(&config);
					is_loaded = true;
					config.MergeMode = true;
				}
				else
				{
					for (auto k = font_find_folders.begin(); k != font_find_folders.end(); ++k)
					{
						if (j->fontname.empty())
							continue;
						std::vector<std::string> extensions = {"", ".ttc", ".ttf" };
						for (auto l = extensions.begin(); l != extensions.end(); ++l) {
							// ttf, ttc only
							auto fontpath = (*k) / (j->fontname + *l);
							if (boost::filesystem::exists(fontpath) && boost::filesystem::is_regular_file(fontpath))
							{
								io.Fonts->AddFontFromFileTTF((fontpath).string().c_str(), j->font_size, &config, glyph_range_map[j->glyph_range]);
								is_loaded = true;
								config.MergeMode = true;
							}
						}
					}
				}
			}
		}

		if (*i == "Default" && !is_loaded)
		{
			io.Fonts->AddFontDefault(&config);
			is_loaded = true;
			config.MergeMode = true;
		}
	}
	// do not remove
	io.Fonts->AddFontDefault();
	if (fonts.empty())
	{
		font_sizes = 13;
	}
	else
	{
		font_sizes = fonts[0].font_size;
		for (auto i = fonts.begin(); i != fonts.end(); ++i)
		{
			font_sizes = std::min(font_sizes, i->font_size);
		}
	}
	font_setting_dirty = false;
}
