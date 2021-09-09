

#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <stdint.h>
#include <thread>
#include <list>

#include "classes.h"
#include "overlay.h"
#include "cfg.h"
#include "xorstr.h"

using X2295::D3DOverlay;


UEngine CurrentUEngine;
ULocalPlayer CurrentLocalPlayer;
APlayerController CurrentController;
APawn CurrentAcknowledgedPawn;
ACamera CurrentCamera;
AWorldInfo CurrentWorldInfo;
APawn CurrentPawnList;
APawn LockedPawn;

bool IsValid(uint64_t adress) 
{
    if (adress == 0 || adress == 0xCCCCCCCCCCCCCCCC) {
        return false;
    }
    return true;
}

void CallAimbot()
{
    if (!IsValid(LockedPawn.data)) return;
    int Health = LockedPawn.GetHealth();
    if (Health > 1)
    {
        FRotator AimRotation = FRotator{ 0,0,0 };
        bool isVisible = LockedPawn.GetMesh().IsVisible(CurrentWorldInfo.GetTimeSeconds());
        if (isVisible)
        {
            FVector TargetLocation = LockedPawn.GetLocation();
            if (CFG.b_isPredictionAim)
            {
                auto currProjectiles = CurrentAcknowledgedPawn.GetWeapon().GetProjectiles();
                if (currProjectiles.Length() > 0)
                {
                    CFG.fl_Speed = currProjectiles.GetById(0).GetSpeed();
                }
                FVector TargetVelocity = LockedPawn.GetVelocity();
                float TravelTime = math::GetDistance(CurrentAcknowledgedPawn.GetLocation(), TargetLocation) / CFG.fl_Speed;
                TargetLocation = {
                    (TargetLocation.X + TargetVelocity.X * TravelTime),
                    (TargetLocation.Y + TargetVelocity.Y * TravelTime),
                     TargetLocation.Z
                };
            }
            FVector RealLocation = CurrentCamera.GetRealLocation();
            math::AimAtVector(TargetLocation + FVector(0, 0, LockedPawn.GetEyeHeight()), RealLocation, AimRotation);

            AimRotation.Yaw = math::ClampYaw(AimRotation.Yaw);
            AimRotation.Pitch = math::ClampPitch(AimRotation.Pitch);

            if (CFG.b_Smoothing)
            {
                FRotator currentRotation = CurrentController.GetRotation();
                currentRotation.Roll = 0;

                auto diff = currentRotation - AimRotation;

                auto realDiff = diff;
                auto a = math::ClampYaw(currentRotation.Yaw);
                auto b = math::ClampYaw(AimRotation.Yaw);
                const auto Full360 = Const_URotation180 * 2;

                auto dist1 = -(a - b + Full360) % Full360;
                auto dist2 = (b - a + Full360) % Full360;

                auto dist = dist1;
                if (abs(dist2) < abs(dist1)) {
                    dist = dist2;
                }

                auto smoothAmount = CFG.fl_SmoothingValue;

                if (CFG.b_LockWhenClose && abs(dist) + abs(diff.Pitch) < Const_URotation180 / 100) {
                    smoothAmount = 1;
                }

                diff.Yaw = (int)(dist * smoothAmount);
                diff.Pitch = (int)(diff.Pitch * smoothAmount);
                AimRotation = currentRotation + diff;
            }
            AimRotation.Pitch = AimRotation.Pitch;
            CurrentController.SetRotation(AimRotation);
        }
    }
    else
    {
        CFG.b_Locked = false;
        LockedPawn = APawn{};
        return;
    }
}
void RenderVisual()
{
	auto width = D3DOverlay::getWidth();
	auto height = D3DOverlay::getHeight();

	CurrentUEngine = GetUEngine(GameVars.dwProcess_Base);
	CurrentLocalPlayer = CurrentUEngine.GetLocalPlayer();
	CurrentController = CurrentLocalPlayer.GetController();
	CurrentAcknowledgedPawn = CurrentController.GetAcknowledgedPawn();
    CurrentCamera = CurrentController.GetCamera();
	CurrentWorldInfo = CurrentController.GetWorldInfo();
	CurrentPawnList = CurrentWorldInfo.GetPawnList();

	CFG.in_CurrentHealth = CurrentAcknowledgedPawn.GetHealth();
	CFG.fl_CurrentFOV = CurrentCamera.GetDeafultFov() * CurrentController.GetFovMultiplier();


	if (CFG.b_Aimbot)
	{
		if (CFG.b_AimFOV)
		{
			D3DOverlay::DrawCircle(width / 2, height / 2, math::GetFOV(CFG.fl_AimFov), CFG.FovColor);
		}
	}

	APawn CurrentPawn = CurrentPawnList;
	int players = 0;
	bool isAimbotActive = CFG.b_Aimbot && GetAsyncKeyState(VK_LMENU) & 1;

	if (!IsValid(LockedPawn.data) || !LockedPawn.GetMesh().IsVisible(CurrentWorldInfo.GetTimeSeconds())) {
		LockedPawn = APawn{};
		CFG.b_Locked = false;
	}

	float ClosestDistance = 999999.0f;

	auto currentTeamIndex = CurrentAcknowledgedPawn.GetPlayerReplicationInfo().GetTeamInfo().GetTeamIndex();

	while (IsValid(CurrentPawn.data))
	{
		APawn nextPawn = CurrentPawn.GetNextPawn();
		if (!IsValid(CurrentPawn.data))
		{
			CurrentPawn = nextPawn;
			continue;
		}

		int Hp = CurrentPawn.GetHealth();
		players++;
		auto repInfo = CurrentPawn.GetPlayerReplicationInfo();
		if (!IsValid(repInfo.data)) {
			CurrentPawn = nextPawn;
			continue;
		}

		auto teamInfo = repInfo.GetTeamInfo();
		if (!IsValid(teamInfo.data)) {
			CurrentPawn = nextPawn;
			continue;
		}

		auto teamIndex = teamInfo.GetTeamIndex();

		if (teamIndex == currentTeamIndex ||
			CurrentPawn.data == CurrentAcknowledgedPawn.data ||
			Hp < 1 || Hp > 10000)
		{
			CurrentPawn = nextPawn;
			continue;
		}

		auto mesh = CurrentPawn.GetMesh();
		if (!IsValid(mesh.data)) {
			CurrentPawn = nextPawn;
			continue;
		}

		FBoxSphereBounds PlayerHitbox = mesh.GetBounds();

		FVector bottom, head, pawnPos;
		Vec2 bottompos, headpos, pos;

		bottom = math::VectorSubtract(PlayerHitbox.Origin, PlayerHitbox.BoxExtent);
		head = math::VectorAdd(PlayerHitbox.Origin, PlayerHitbox.BoxExtent);

		pawnPos = CurrentPawn.GetLocation();

		FVector posHead = pawnPos;
		FRotator Rotation = CurrentController.GetRotation();
		FVector RealLocation = CurrentCamera.GetRealLocation();

		bool isPawnVisible = mesh.IsVisible(CurrentWorldInfo.GetTimeSeconds());   
		auto tracerDistance = math::VectorMagnitude(math::VectorSubtract(RealLocation, pawnPos)) / 50.f;

		try
		{
			if (math::W2S(bottom, bottompos, Rotation, RealLocation, CFG.fl_CurrentFOV) &&
				math::W2S(head, headpos, Rotation, RealLocation, CFG.fl_CurrentFOV))
			{

				float box_height = abs(headpos.y - bottompos.y);
				float box_width = box_height * 0.75;

				if (CFG.b_Visual)
				{
					//BOX

					if (CFG.b_EspBox)
					{
						if (CFG.in_BoxType == 0)
						{
							D3DOverlay::DrawBox(CFG.BoxVisColor, headpos.x - (box_width / 2), headpos.y, box_width, box_height);
						}
						else if (CFG.in_BoxType == 1)
						{
							D3DOverlay::DrawCorneredBox(headpos.x - (box_width / 2), headpos.y, box_width, box_height, CFG.BoxVisColor, 1.5f);

						}
					}

					//HP
					if (CFG.b_EspHealth)
					{

						auto maxHP = CurrentPawn.GetMaxHealth();
						auto procentage = Hp * 100 / maxHP;

						float width = box_width / 10;
						if (width < 2.f) width = 2.;
						if (width > 3) width = 3.;

						D3DOverlay::ProgressBar(headpos.x - (box_width / 2), headpos.y, width, bottompos.y - headpos.y, procentage, 100, CFG.HealthBarColor, true);
					}
					//Name
					if (CFG.b_EspName)
					{
						D3DOverlay::DrawOutlinedText(X2295::Century_Gothic, repInfo.GetName().ToString(), ImVec2(headpos.x, headpos.y - 20), 15, ImColor(255, 255, 255), true);
					}
					//Line
					if (CFG.b_EspLine)
					{

						if (CFG.in_LineType == 0)
						{
							D3DOverlay::DrawLine(ImVec2(static_cast<float>(width/ 2), static_cast<float>(height)), ImVec2(bottompos.x, bottompos.y), CFG.LineColor, 1.3f); //LINE FROM BOTTOM SCREEN
						}
						if (CFG.in_LineType == 1)
						{
							D3DOverlay::DrawLine(ImVec2(static_cast<float>(width/ 2), 0.f), ImVec2(bottompos.x, bottompos.y), CFG.LineColor, 1); //LINE FROM TOP SCREEN
						}
						if (CFG.in_LineType == 2)
						{
							D3DOverlay::DrawLine(ImVec2(static_cast<float>(width / 2), static_cast<float>(height / 2)), ImVec2(bottompos.x, bottompos.y), CFG.LineColor, 1.3f); //LINE FROM BOTTOM SCREEN
						}

					}
				}
				if (isAimbotActive && isPawnVisible && !CFG.b_Locked)
				{
					Vec2 headPos;
					if (math::W2S(posHead, headPos, Rotation, RealLocation, CFG.fl_CurrentFOV))
					{
						//TODO SETTING
						float ScreenCX = GameVars.ScreenWidth / 2;
						float ScreenCY = GameVars.ScreenHeight / 2;
						float cx = headPos.x;
						float cy = headPos.y;
						float radiusx_ = CFG.fl_AimFov * (ScreenCX / CFG.fl_CurrentFOV);
						float radiusy_ = CFG.fl_AimFov * (ScreenCY / CFG.fl_CurrentFOV);
						float crosshairDistance = math::GetCrosshairDistance(cx, cy, ScreenCX, ScreenCY);
						if (tracerDistance < 30.f || cx >= ScreenCX - radiusx_ && cx <= ScreenCX + radiusx_ && cy >= ScreenCY - radiusy_ && cy <= ScreenCY + radiusy_)
						{
							if (crosshairDistance < ClosestDistance)
							{
								ClosestDistance = crosshairDistance;
								LockedPawn = CurrentPawn;
							}
						}
						CurrentPawn = nextPawn;
						continue;
					}
				}
			}
			CurrentPawn = nextPawn;
		}
		catch (const std::exception&) {
			CurrentPawn = nextPawn;
			continue;
		}
	}

	if (IsValid(LockedPawn.data))
	{
		CFG.b_Locked = true;
		CallAimbot();
	}
}

auto Render(FLOAT aWidth, FLOAT aHeight)->VOID
{
	if (GetAsyncKeyState(VK_INSERT) & 1)
		CFG.b_MenuShow = !CFG.b_MenuShow;

	if (CFG.b_MenuShow)
	{
		ImGui::SetNextWindowSize(ImVec2(675, 410)); // 450,426

		ImGui::PushFont(X2295::DefaultFont);
		ImGui::Begin(enc("Jenrix Paladins"),0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);


		//Visual Function

		D3DOverlay::TabButton(enc("Visual"), &CFG.in_tab_index, 0, true);
		D3DOverlay::TabButton(enc("Aimbot"), &CFG.in_tab_index, 1, false);

		if (CFG.in_tab_index == 0)
		{
			ImGui::Checkbox(enc("Enabled Visual"), &CFG.b_Visual);
			if (CFG.b_Visual)
			{
				ImGui::Checkbox(enc("ESP Box"), &CFG.b_EspBox);
				if (CFG.b_EspBox)
				{
					ImGui::Combo(enc("ESP Box Type"), &CFG.in_BoxType, CFG.BoxTypes, 2);
					if (ImGui::ColorEdit4(enc("Box Color"), CFG.fl_BoxVisColor))
					{
						CFG.BoxVisColor = ImColor(CFG.fl_BoxVisColor[0], CFG.fl_BoxVisColor[1], CFG.fl_BoxVisColor[2], CFG.fl_BoxVisColor[3]);
					}
				}
				ImGui::Checkbox(enc("ESP Name"), &CFG.b_EspName);
				ImGui::Checkbox(enc("ESP HealthBar"), &CFG.b_EspHealth);
				if (CFG.b_EspHealth)
				{
					if (ImGui::ColorEdit4(enc("Health Bar Color"), CFG.fl_HealthBarColor))
					{
						CFG.HealthBarColor = ImColor(CFG.fl_HealthBarColor[0], CFG.fl_HealthBarColor[1], CFG.fl_HealthBarColor[2], CFG.fl_HealthBarColor[3]);
					}
				}
				ImGui::Checkbox(enc("ESP Line"), &CFG.b_EspLine);
				if (CFG.b_EspLine)
				{
					ImGui::Combo(enc("Line ESP Type"), &CFG.in_LineType, CFG.LineTypes, 3);
					if (ImGui::ColorEdit4(enc("Line ESP Color"), CFG.fl_LineColor))
					{
						CFG.LineColor = ImColor(CFG.fl_LineColor[0], CFG.fl_LineColor[1], CFG.fl_LineColor[2], CFG.fl_LineColor[3]);
					}
				}
			}
		}
		else if (CFG.in_tab_index == 1)
		{
			ImGui::Checkbox(enc("Enabled Aimbot"), &CFG.b_Aimbot);
			if (CFG.b_Aimbot)
			{
				ImGui::Text(enc("Press Alt to Activate"));
				ImGui::Checkbox(enc("Draw FOV"), &CFG.b_AimFOV);
				if (CFG.b_AimFOV)
				{
					if (ImGui::ColorEdit4(enc("FOV Color"), CFG.fl_FovColor))
					{
						CFG.FovColor = ImColor(CFG.fl_FovColor[0], CFG.fl_FovColor[1], CFG.fl_FovColor[2], CFG.fl_FovColor[3]);
					}
				}
				ImGui::Checkbox(enc("LockWhenClose"), &CFG.b_LockWhenClose);
				ImGui::SliderFloat(enc("Smoothing"), &CFG.fl_SmoothingValue, 0.0f, 1.0f);
				if (CFG.b_AimFOV)
				{
					ImGui::SliderFloat(enc("Field Of View"), &CFG.fl_AimFov, 5.f, 90.f);
				}
			}
		}
		ImGui::PopFont();
		ImGui::End();
	}
	RenderVisual();
}


int main()
{

    GameVars.dwProcessId = GetProcessIdByName(GameVars.dwProcessName);
    if (!GameVars.dwProcessId)
    {
        printf("[!] process \"%s\ was not found\n", GameVars.dwProcessName);
    }
	Controller = new DriverController(GameVars.dwProcessId);
	GameVars.dwProcess_Base = GetBaseAddress();
	if (!GameVars.dwProcess_Base) 
	{
		printf("[!] failed to get baseadress\n");
	}
	printf("[+] ProcessName: %s ID: (%d) base: %llx\n", GameVars.dwProcessName, GameVars.dwProcessId, GameVars.dwProcess_Base);

    HWND  gameHWND = GetHwndById(GameVars.dwProcessId);
    if (IsWindow(gameHWND))
    {
        if (D3DOverlay::InitOverlay(_T("xxxxx"), _T("xxxxxxxxxx")))
        {

			GameVars.ScreenWidth = D3DOverlay::getWidth();
			GameVars.ScreenHeight = D3DOverlay::getHeight();

			ImGuiIO& io = ImGui::GetIO();
			X2295::DefaultFont = io.Fonts->AddFontDefault();
			X2295::Century_Gothic = io.Fonts->AddFontFromFileTTF("Century_Gothic.ttf", 16);
			io.Fonts->Build();

            D3DOverlay::SetUserRender(Render);
            while (D3DOverlay::MsgLoop() && D3DOverlay::AttachWindow(gameHWND)) {};
            D3DOverlay::UninitOverlay();
        }
    }
    return 0;
}
