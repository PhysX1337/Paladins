#include <string>
#include <sstream>

#include "overlay.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace X2295
{
	HWND D3DOverlay::m_vWnd;
	WNDCLASSEX D3DOverlay::m_vWndClass;
	FLOAT D3DOverlay::m_vRenderWidth;
	FLOAT D3DOverlay::m_vRenderHeight;
	_D3DOVERLAY_USER_RENDER D3DOverlay::m_vUserRender;
	RECT D3DOverlay::m_vOldRect;

	ID3D11Device* D3DOverlay::ms_vD3dDevice;
	ID3D11DeviceContext* D3DOverlay::ms_vDeviceContext;
	IDXGISwapChain* D3DOverlay::ms_vSwapChain;
	ID3D11RenderTargetView* D3DOverlay::ms_vRenderTargetView;

	auto WINAPI D3DOverlay::WndProc(HWND aHwnd, UINT aMsg, WPARAM aWParam, LPARAM aLParam) -> LRESULT
	{
		switch (aMsg)
		{
		case WM_SIZE:
		{
			if (ms_vD3dDevice != NULL && aWParam != SIZE_MINIMIZED)
			{
				CleanupRenderTarget();
				m_vRenderWidth = (FLOAT)LOWORD(aLParam);
				m_vRenderHeight = (FLOAT)HIWORD(aLParam);
				ms_vSwapChain->ResizeBuffers(0, (UINT)m_vRenderWidth, (UINT)m_vRenderHeight, DXGI_FORMAT_UNKNOWN, 0);
				CreateRenderTarget();
				return S_OK;
			}
			break;
		}
		case WM_SYSCOMMAND:
		{
			if ((aWParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return S_OK;
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
		}
		if (ImGui_ImplWin32_WndProcHandler(aHwnd, aMsg, aWParam, aLParam))
			return S_OK;
		return ::DefWindowProc(aHwnd, aMsg, aWParam, aLParam);
	}

	auto D3DOverlay::Reset() -> VOID
	{
		m_vWnd = nullptr;
		m_vWndClass = {};
		m_vRenderWidth = (FLOAT)GetSystemMetrics(SM_CXSCREEN);
		m_vRenderHeight = (FLOAT)GetSystemMetrics(SM_CYSCREEN);
		m_vUserRender = nullptr;
		m_vOldRect = { 0 };

		ms_vD3dDevice = nullptr;
		ms_vDeviceContext = nullptr;
		ms_vSwapChain = nullptr;
		ms_vRenderTargetView = nullptr;
	}

	auto D3DOverlay::CreateOverlay(LPCTSTR aWndClassName, LPCTSTR aWndName)->BOOLEAN
	{
		Reset();
		MARGINS vMargins{ -1 };
		m_vWndClass = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), NULL, NULL, NULL, NULL, aWndClassName, NULL };
		RegisterClassEx(&m_vWndClass);
		m_vWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, m_vWndClass.lpszClassName, aWndName, WS_POPUP, 1, 1, (INT)m_vRenderWidth - 2, (INT)m_vRenderHeight - 2, 0, 0, 0, 0);
		if (!m_vWnd)
			return FALSE;
		SetLayeredWindowAttributes(m_vWnd, 0, 0, LWA_ALPHA);
		SetLayeredWindowAttributes(m_vWnd, 0, RGB(0, 0, 0), LWA_COLORKEY);
		ShowWindow(m_vWnd, SW_SHOW);
		DwmExtendFrameIntoClientArea(m_vWnd, &vMargins);

		return TRUE;
	}

	auto D3DOverlay::CleanupOverlay() -> VOID
	{
		UnregisterClass(m_vWndClass.lpszClassName, m_vWndClass.hInstance);
		if (m_vWnd)
			CloseWindow(m_vWnd);
	}

	auto D3DOverlay::CreateDeviceD3D() -> BOOLEAN
	{
		DXGI_SWAP_CHAIN_DESC vSwapChainDesc;
		ZeroMemory(&vSwapChainDesc, sizeof(vSwapChainDesc));
		vSwapChainDesc.BufferCount = 2;
		vSwapChainDesc.BufferDesc.Width = 0;
		vSwapChainDesc.BufferDesc.Height = 0;
		vSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		vSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		vSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		vSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		vSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		vSwapChainDesc.OutputWindow = m_vWnd;
		vSwapChainDesc.SampleDesc.Count = 1;
		vSwapChainDesc.SampleDesc.Quality = 0;
		vSwapChainDesc.Windowed = TRUE;
		vSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		D3D_FEATURE_LEVEL vFeatureLevel;
		D3D_FEATURE_LEVEL vFeatureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
		if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, vFeatureLevelArray, 2, D3D11_SDK_VERSION, &vSwapChainDesc, &ms_vSwapChain, &ms_vD3dDevice, &vFeatureLevel, &ms_vDeviceContext) != S_OK)
			return FALSE;
		CreateRenderTarget();
		ShowWindow(m_vWnd, SW_SHOWNORMAL);
		UpdateWindow(m_vWnd);
		return TRUE;
	}

	auto D3DOverlay::CleanupDeviceD3D() -> VOID
	{
		CleanupRenderTarget();
		if (ms_vSwapChain)
		{
			ms_vSwapChain->Release();
			ms_vSwapChain = NULL;
		}
		if (ms_vDeviceContext)
		{
			ms_vDeviceContext->Release();
			ms_vDeviceContext = NULL;
		}
		if (ms_vD3dDevice)
		{
			ms_vD3dDevice->Release();
			ms_vD3dDevice = NULL;
		}
	}

	auto D3DOverlay::CreateRenderTarget() -> VOID
	{
		ID3D11Texture2D* pBackBuffer = nullptr;
		ms_vSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		ms_vD3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &ms_vRenderTargetView);
		pBackBuffer->Release();
	}

	auto D3DOverlay::CleanupRenderTarget() -> VOID
	{
		if (!ms_vRenderTargetView)
			return;
		ms_vRenderTargetView->Release();
		ms_vRenderTargetView = NULL;
	}

	auto D3DOverlay::CreateImGuiContext() -> VOID
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui::StyleColorsClassic();
		auto vStyle = &ImGui::GetStyle();

		vStyle->WindowPadding = ImVec2(15, 15);
		vStyle->WindowRounding = 5.0f;
		vStyle->FramePadding = ImVec2(5, 5);
		vStyle->FrameRounding = 4.0f;
		vStyle->ItemSpacing = ImVec2(12, 8);
		vStyle->ItemInnerSpacing = ImVec2(8, 6);
		vStyle->IndentSpacing = 25.0f;
		vStyle->ScrollbarSize = 15.0f;
		vStyle->ScrollbarRounding = 9.0f;
		vStyle->GrabMinSize = 5.0f;
		vStyle->GrabRounding = 3.0f;

		vStyle->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
		vStyle->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		vStyle->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		vStyle->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		vStyle->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
		vStyle->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		vStyle->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		vStyle->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		vStyle->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
		vStyle->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		vStyle->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		vStyle->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		vStyle->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		vStyle->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		vStyle->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		vStyle->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		vStyle->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		vStyle->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		vStyle->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		vStyle->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);

		vStyle->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		vStyle->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		vStyle->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		vStyle->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		vStyle->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		vStyle->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		vStyle->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		vStyle->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);

		vStyle->WindowTitleAlign.x = 0.50f;
		vStyle->FrameRounding = 2.0f;

		ImGui_ImplWin32_Init(m_vWnd);
		ImGui_ImplDX11_Init(ms_vD3dDevice, ms_vDeviceContext);
	}

	auto D3DOverlay::CleanupImGuiContext() -> VOID
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	auto D3DOverlay::MsgLoop() -> BOOLEAN
	{
		if (m_vWnd)
		{
			MSG vMsg{ 0 };
			ImVec4 vCleanColor{ 0,0,0,0 };

			if (::PeekMessage(&vMsg, NULL, 0U, 0U, PM_REMOVE))
			{
				::TranslateMessage(&vMsg);
				::DispatchMessage(&vMsg);
			}

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			if (m_vUserRender)
				m_vUserRender(m_vRenderWidth, m_vRenderHeight);
			ImGui::Render();
			ms_vDeviceContext->OMSetRenderTargets(1, &ms_vRenderTargetView, NULL);
			ms_vDeviceContext->ClearRenderTargetView(ms_vRenderTargetView, (float*)&vCleanColor);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			ms_vSwapChain->Present(1, 0);

			return vMsg.message != WM_QUIT;
		}
		return FALSE;
	}

	auto D3DOverlay::AttachWindow(const HWND aWnd)->BOOLEAN
	{
		if (!m_vWnd || !IsWindow(aWnd))
			return FALSE;

		RECT vRect{ 0 };
		POINT vPoint{ 0 };
		GetClientRect(aWnd, &vRect);
		ClientToScreen(aWnd, &vPoint);
		vRect.left = vPoint.x + 1;
		vRect.top = vPoint.y + 1;
		vRect.right -= 2;
		vRect.bottom -= 2;

		if (m_vOldRect.left != vRect.left ||
			m_vOldRect.top != vRect.top ||
			m_vOldRect.right != vRect.right ||
			m_vOldRect.bottom != vRect.bottom)
		{
			SetWindowPos(m_vWnd, HWND_TOPMOST, vRect.left, vRect.top, vRect.right, vRect.bottom, SWP_NOREDRAW);
			m_vOldRect = vRect;
		}
		return TRUE;
	}

	int D3DOverlay::getWidth()
	{
		return m_vRenderWidth;
	}

	int D3DOverlay::getHeight()
	{
		return m_vRenderHeight;
	}

	auto D3DOverlay::DrawCrossHair(const FLOAT aSize, ImU32 aColor)-> VOID
	{
		auto vList = ImGui::GetBackgroundDrawList();

		vList->AddLine({ m_vRenderWidth / 2,m_vRenderHeight / 2 - (aSize + 1) }, { m_vRenderWidth / 2 ,m_vRenderHeight / 2 + (aSize + 1) }, aColor, 2);
		vList->AddLine({ m_vRenderWidth / 2 - (aSize + 1),m_vRenderHeight / 2 }, { m_vRenderWidth / 2 + (aSize + 1)  ,m_vRenderHeight / 2 }, aColor, 2);
	}

	auto D3DOverlay::DrawEspBox(const ImVec2& aPos, const FLOAT aWidth, const FLOAT aHeight, ImU32 aColor, const FLOAT aLineWidth) -> VOID
	{
		auto vList = ImGui::GetBackgroundDrawList();

		std::array<ImVec2, 4> vBoxLines{ aPos,ImVec2{aPos.x + aWidth,aPos.y},ImVec2{aPos.x + aWidth,aPos.y + aHeight},ImVec2{aPos.x,aPos.y + aHeight} };
		vList->AddPolyline(vBoxLines.data(), vBoxLines.size(), aColor, true, 2);
	}

	auto D3DOverlay::DrawLine(const ImVec2& aPoint1, const ImVec2 aPoint2, ImU32 aColor, const FLOAT aLineWidth) -> VOID
	{
		auto vList = ImGui::GetBackgroundDrawList();
		vList->AddLine(aPoint1, aPoint2, aColor, aLineWidth);
	}

	auto D3DOverlay::DrawBox(ImColor color, float x, float y, float w, float h)-> VOID
	{
		DrawLine(ImVec2(x, y), ImVec2(x + w, y), color, 1.3f); // top 
		DrawLine(ImVec2(x, y - 1.3f), ImVec2(x, y + h + 1.4f), color, 1.3f); // left
		DrawLine(ImVec2(x + w, y - 1.3f), ImVec2(x + w, y + h + 1.4f), color, 1.3f);  // right
		DrawLine(ImVec2(x, y + h), ImVec2(x + w, y + h), color, 1.3f);   // bottom 
	}
	auto D3DOverlay::RectFilled(float x0, float y0, float x1, float y1, ImColor color, float rounding, int rounding_corners_flags)-> VOID
	{
		auto vList = ImGui::GetBackgroundDrawList();
		vList->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color, rounding, rounding_corners_flags);
	}
	auto D3DOverlay::ProgressBar(float x, float y, float w, float h, int value, int v_max, ImColor barColor, bool Outlined, ImColor Outlinecolor)-> VOID
	{
		auto vList = ImGui::GetBackgroundDrawList();
		if (Outlined)
			vList->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), Outlinecolor, 0.0f, 0, 2.0f);
		RectFilled(x, y, x + w, y + ((h / float(v_max)) * (float)value), barColor, 0.0f, 0);
	}
	auto D3DOverlay::DrawOutlinedText(ImFont* pFont, const std::string& text, const ImVec2& pos, float size, ImU32 color, bool center)-> VOID
	{
		auto vList = ImGui::GetBackgroundDrawList();

		std::stringstream stream(text);
		std::string line;
		float y = 0.0f;
		int i = 0;

		while (std::getline(stream, line))
		{
			ImVec2 textSize = pFont->CalcTextSizeA(size, FLT_MAX, 0.0f, line.c_str());

			if (center)
			{
				vList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
				vList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
				vList->AddText(pFont, size, ImVec2(pos.x - textSize.x / 2.0f, pos.y + textSize.y * i), ImGui::GetColorU32(color), line.c_str());
			}
			else
			{//
				vList->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
				vList->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
				vList->AddText(pFont, size, ImVec2(pos.x, pos.y + textSize.y * i), ImGui::GetColorU32(color), line.c_str());
			}

			y = pos.y + textSize.y * (i + 1);
			i++;
		}
	}

	auto D3DOverlay::DrawCorneredBox(float X, float Y, float W, float H, const ImU32& color, float thickness) -> VOID
	{
		auto vList = ImGui::GetBackgroundDrawList();

		float lineW = (W / 3);
		float lineH = (H / 3);
		//black outlines
		auto col = ImGui::GetColorU32(color);

		//corners
		vList->AddLine(ImVec2(X, Y - thickness / 2), ImVec2(X, Y + lineH), col, thickness);//top left
		vList->AddLine(ImVec2(X - thickness / 2, Y), ImVec2(X + lineW, Y), col, thickness);

		vList->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W + thickness / 2, Y), col, thickness);//top right horizontal
		vList->AddLine(ImVec2(X + W, Y - thickness / 2), ImVec2(X + W, Y + lineH), col, thickness);

		vList->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H + (thickness / 2)), col, thickness);//bot left
		vList->AddLine(ImVec2(X - thickness / 2, Y + H), ImVec2(X + lineW, Y + H), col, thickness);

		vList->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W + thickness / 2, Y + H), col, thickness);//bot right
		vList->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H + (thickness / 2)), col, thickness);

	}

	auto D3DOverlay::DrawCircle(const ImVec2& aPoint, const FLOAT aR, ImU32 aColor, const FLOAT aLineWidth) -> VOID
	{
		auto vList = ImGui::GetBackgroundDrawList();
		vList->AddCircle(aPoint, aR, aColor, 120, aLineWidth);
	}
	auto D3DOverlay::DrawCircle(float x, float y, float radius, ImVec4 color, int segments)-> VOID
	{
		auto vList = ImGui::GetBackgroundDrawList();
		vList->AddCircle(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(color), segments);
	}
	auto D3DOverlay::DrawString(const ImVec2& aPos, const std::string& aString, ImU32 aColor) -> VOID
	{
		auto vList = ImGui::GetBackgroundDrawList();
		vList->AddText(aPos, aColor, aString.data());
	}

	auto D3DOverlay::IsInScrren(const ImVec2& aPos) -> BOOLEAN
	{
		return !(aPos.x > m_vRenderWidth || aPos.x<0 || aPos.y>m_vRenderHeight || aPos.y < 0);
	}

	auto D3DOverlay::SetUserRender(const _D3DOVERLAY_USER_RENDER& aUserRender) -> VOID
	{
		m_vUserRender = aUserRender;
	}

	auto D3DOverlay::InitOverlay(LPCTSTR aWndClassName, LPCTSTR aWndName) -> BOOLEAN
	{
		if (m_vWnd)
			return FALSE;
		if (!CreateOverlay(aWndClassName, aWndName))
		{
			CleanupOverlay();
			_tprintf(_T("Cannot create overlay window!"));
			return FALSE;
		}
		if (!CreateDeviceD3D())
		{
			CleanupOverlay();
			_tprintf(_T("Cannot create d3d device!"));
			return FALSE;
		}
		CreateImGuiContext();
		return true;
	}

	auto D3DOverlay::UninitOverlay() -> VOID
	{
		if (!m_vWnd)
			return;
		CleanupImGuiContext();
		CleanupDeviceD3D();
		CleanupOverlay();
		Reset();
	}
	auto D3DOverlay::TabButton(const char* label, int* index, int val, bool sameline) -> VOID
	{
		if (*index == val)
		{
			if (ImGui::Button(label, ImVec2(80, 25)))
				*index = val;
			if (sameline)
				ImGui::SameLine();
		}
		else
		{
			if (ImGui::Button(label, ImVec2(80, 25)))
				*index = val;
			if (sameline)
				ImGui::SameLine();
		}
	}

}