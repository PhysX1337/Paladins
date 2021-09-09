//BLUEFIRE1337 PALADINS SDK - UPDATED 2021-08-19
#pragma once

#ifndef PALADINS_H

#include <iostream>
#include <Windows.h>
#include "singleton.h"

inline namespace Paladins
{
	class Variables : public Singleton<Variables>
	{
	public:
		const char* dwProcessName = "Paladins.exe";
		DWORD dwProcessId;
		uint64_t dwProcess_Base;

		int ScreenHeight;
		int ScreenWidth;
		float ScreenCenterX = ScreenWidth / 2;
		float ScreenCenterY = ScreenHeight / 2;

	};
#define GameVars Paladins::Variables::Get()

		class Offsets : public Singleton<Offsets>
		{
		public:
			DWORD ENGINE = 0x371E880; // Paladins.exe + ENGINE -> UEngine 

			DWORD LOCALPLAYER = 0x6D8; // UEngine -> ULocalPlayer
			DWORD CONTROLLER = 0x68; // ULocalPlayer -> APlayerController

			DWORD ROTATION = 0x008C; // APlayerController -> FRotator
			DWORD WORLD_INFO = 0x0120; // APlayerController -> AWorldInfo

			DWORD ACKNOWLEDGEDPAWN = 0x0498; // APlayerController -> acknowledgedPawn | APawn

			DWORD PLAYER_FOV_MULTIPLIER = 0x04CC; // APlayerController -> FOVMultiplier, aka LODDistanceFactor | float
			DWORD CAMERA = 0x0478; // APlayerController -> ACamera

			DWORD REAL_LOCATION = 0x0494; // ACamera -> FVector
			DWORD DEAFULT_FOV = 0x0290; // ACamera -> float

			DWORD PAWN_LIST = 0x05B4; // AWorldInfo -> pawnList
			DWORD TIMESECONDS = 0x04EC; // AWorldInfo -> float

			DWORD TEAM = 0x4DC; // APlayerReplicationInfo -> Team 
			DWORD TEAMINDEX = 0x298; // Team -> int

			DWORD AMMO_COUNT = 0x4A4; // ATgDevice -> int
			DWORD AMMO_MAX_COUNT = 0x5F0; // ATgDevice -> int

			DWORD GLOW = 0x674; // acknowledgedPawn -> glow
			DWORD THIRDPERSON = 0x0624; // acknowledgedPawn -> write float
			DWORD WEAPON = 0x04E4; // APawn -> ATgDevice
			DWORD BASE_EYE_HEIGHT = 0x03A4; // APawn -> float
			DWORD HEALTH = 0xA90; // APawn -> float
			DWORD GET_HEALTH = 0x3C4; // APawn -> float
			DWORD MAX_HEALTH = 0x111C; // APawn -> float
			DWORD LOCATION = 0x0080; // APawn -> FVector
			DWORD VELOCITY = 0x0190; // APawn -> FVector
			DWORD PLAYER_REPLICATION_INFO = 0x0440; // APawn -> APlayerReplicationInfo
			DWORD NEXT_PAWN = 0x02AC; // APawn -> next APawn
			DWORD MESH = 0x048C; // APawn -> USkeletalMeshComponent

			DWORD BOUNDS = 0x00A0; // USkeletalMeshComponent -> FBoxSphereBounds
			DWORD LAST_RENDER_TIME = 0x0244; // USkeletalMeshComponent -> float

			DWORD PLAYER_NAME = 0x0290; // APlayerReplicationInfo -> FString
			DWORD RECOIL_SETTINGS = 0x0960; // ATgDevice -> FRecoilSettings
			DWORD ACCURACY_SETTINGS = 0x08C0; // ATgDevice -> FAccuracySettings

			DWORD CURRENT_PROJECTILES = 0x0610; // ATgDevice -> TArray<ATgProjectile>
			DWORD PROJECTILE_SPEED = 0x0280; // ATgProjectile -> float
		};
#define GameOffset Paladins::Offsets::Get()
}
#endif  !PALADINS_H

