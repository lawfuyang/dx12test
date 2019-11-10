#pragma once

#include "system.h"

::HWND g_EngineWindowHandle = 0;

class EngineWindowThread
{
public:
    static void Initialize()
    {
        ms_EngineWindowThread = std::thread{ [&]() { Run(); } };
    }

    static void Shutdown()
    {
        if (g_EngineWindowHandle)
            DestroyWindow(g_EngineWindowHandle);
        g_EngineWindowHandle = 0;

        ms_EngineWindowThread.join();
    }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto Get_X_LPARAM = [&](LPARAM lParam) { return ((int32_t)(int64_t)LOWORD(lParam)); };
        auto Get_Y_LPARAM = [&](LPARAM lParam) { return ((int32_t)(int64_t)HIWORD(lParam)); };

        switch (message)
        {
        case WM_CLOSE:
            ms_Exit = true;
            break;
        case WM_MOUSEMOVE:
            RECT rect;
            GetClientRect(g_EngineWindowHandle, &rect);
            g_Mouse.ProcessMouseMove((uint32_t)wParam, Get_X_LPARAM(lParam) - rect.left, Get_Y_LPARAM(lParam) - rect.top);
            // NO BREAK HERE!
        case WM_KEYUP:
            g_Keyboard.ProcessKeyUp((uint32_t)wParam);
            break;
        case WM_KEYDOWN:
            g_Keyboard.ProcessKeyDown((uint32_t)wParam);
            break;
        case WM_LBUTTONDOWN:
            g_Mouse.UpdateButton(Mouse::Left, true);
            break;
        case WM_LBUTTONUP:
            g_Mouse.UpdateButton(Mouse::Left, false);
            break;
        case WM_RBUTTONDOWN:
            g_Mouse.UpdateButton(Mouse::Right, true);
            break;
        case WM_RBUTTONUP:
            g_Mouse.UpdateButton(Mouse::Right, false);
            break;
        case WM_MOUSEWHEEL:
            g_Mouse.ProcessMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            break;
        default:
            break;
        }

        System::GetInstance().ProcessWindowMessage(hWnd, message, wParam, lParam);
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    static std::string GetLastErrorAsString()
    {
        //Get the error message, if any.
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0)
            return ""; //No error message has been recorded

        LPSTR messageBuffer = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        return std::string{ messageBuffer };
    }

    static void Run()
    {
        HINSTANCE hInstance = 0;

        WNDCLASS wc = { 0 };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
        wc.lpszClassName = System::APP_NAME.c_str();

        bbeVerify(SUCCEEDED(RegisterClass(&wc)), "ApplicationWin : Failed to create window: %s", GetLastErrorAsString().c_str());

        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
        RECT rect;
        rect.left = 0; rect.top = 0; rect.right = (uint32_t)System::APP_WINDOW_WIDTH; rect.bottom = (uint32_t)System::APP_WINDOW_HEIGHT;
        AdjustWindowRect(&rect, style, false);

        RECT desktop;
        const HWND hDesktop = GetDesktopWindow();
        GetWindowRect(hDesktop, &desktop);

        const uint32_t nativeWidth = desktop.right - desktop.left;
        const uint32_t nativeHeight = desktop.bottom - desktop.top - 100;
        const uint32_t windowTopLeftX = (nativeWidth - (uint32_t)System::APP_WINDOW_WIDTH) / 2;
        const uint32_t windowTopLeftY = (nativeHeight - (uint32_t)System::APP_WINDOW_HEIGHT) / 2;

        g_EngineWindowHandle = CreateWindow(wc.lpszClassName,
                               System::APP_NAME.c_str(),
                               style,
                               windowTopLeftX, windowTopLeftY,
                               rect.right - rect.left,
                               rect.bottom - rect.top,
                               0, 0, hInstance, NULL);

        bbeAssert(g_EngineWindowHandle != 0, "ApplicationWin : Failed to create window: %s", GetLastErrorAsString().c_str());

        ShowCursor(true);
        SetCursor(LoadCursor(NULL, IDC_ARROW));

        MSG msg;
        BOOL bRet;
        while ((bRet = GetMessageW(&msg, NULL, 0, 0)) != 0)
        {
            if (bRet == -1)
            {
                bbeError(false, "Can't get new message: %s", GetLastErrorAsString().c_str());
                _exit(0);
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (ms_Exit)
            {
                break;
            }
        }
    }

    inline static bool ms_Exit = false;
    inline static std::thread ms_EngineWindowThread;
};

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    BBE_UNUSED_PARAM(hInstance);
    BBE_UNUSED_PARAM(hPrevInstance);
    BBE_UNUSED_PARAM(lpCmdLine);
    BBE_UNUSED_PARAM(nCmdShow);

    EngineWindowThread::Initialize();

    System::GetInstance().Initialize();
    System::GetInstance().Loop();
    System::GetInstance().Shutdown();

    EngineWindowThread::Shutdown();
}
