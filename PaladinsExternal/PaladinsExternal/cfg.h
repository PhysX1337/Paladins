#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include "singleton.h"
#include "imgui/imgui.h"


inline namespace Configuration
{
	class Settings : public Singleton<Settings>
	{
	public:

		const char* BoxTypes[2] = { "Full Box","Cornered Box" };
		const char* LineTypes[3] = { "Bottom To Enemy","Top To Enemy","Crosshair To Enemy" };


		bool b_MenuShow = false;
		bool b_Aimbot = false;
		bool b_AimFOV = false;
		bool b_isPredictionAim = false;
		bool b_Smoothing = false;
		bool b_LockWhenClose = false;
		bool b_Locked;

		bool b_Visual = false;
		bool b_EspBox = false;
		bool b_EspName = false;
		bool b_EspHealth = false;
		bool b_EspLine = false;

		ImColor BoxVisColor = ImColor(255.f / 255, 0.f, 0.f, 1.f);
		ImColor HealthBarColor = ImColor(0.f, 255.f / 255.f, 0.f, 1.f);
		ImColor LineColor = ImColor(0.f, 0.f, 255.f / 255, 1.f);
		ImColor FovColor = ImColor(255.f / 255, 0.f, 0.f, 1.f);

		float fl_BoxVisColor[4] = { 255.f / 255,0.f,0.f,1.f };//
		float fl_HealthBarColor[4] = { 0.f,255.f / 255,0.f,1.f };//
		float fl_LineColor[4] = { 0,0,255.f / 255,1 };  //
		float fl_FovColor[4] = { 255.f / 255,0.f,0.f,1.f };  //

		float fl_CurrentFOV;
		float fl_SmoothingValue = 0.1f; // from 0-1
		float fl_Speed = 7000.0f;
		float fl_AimFov = 30.f;

		int in_BoxType = 0;
		int in_LineType = 0;
		int in_CurrentHealth;
		int in_CurrentLoopFrame;
		int in_tab_index = 0;
	};
#define CFG Configuration::Settings::Get()
}