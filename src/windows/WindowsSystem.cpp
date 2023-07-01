#include "PrecompiledHeader.h"
#include "windows/WindowsSystem.h"
#include "core/GameEngine.h"
#include "graphics/GraphcisSystem.h"

#include <tchar.h>

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam);

namespace Systems
{
  void Windows::Initialize()
  {
    if (!registerClass())
    {
      // ERROR HANDLE
    }

    if (!initWindow())
    {
      // ERROR HANDLE
    }
  }

  void Windows::Update(float dt)
  {
    MSG msg;
    while (PeekMessageA(&msg, windowHandle, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  void Windows::SetMainParameters(HINSTANCE hInstance, int nCmdShow)
  {
    instanceHandle = hInstance;
    commandShow = nCmdShow;
  }

  void Windows::Show()
  {
    ShowWindow(windowHandle, commandShow);
  }

  bool Windows::registerClass()
  {
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instanceHandle;
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = _T(WINDOW_CLASS_NAME);
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    return RegisterClassEx(&wcex);
  }

  bool Windows::initWindow()
  {
    windowHandle = CreateWindowEx(
      WS_EX_OVERLAPPEDWINDOW,
      _T(WINDOW_CLASS_NAME),
      _T(WINDOW_TITLE_BAR),
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT,
      STARTING_RESOLUTION_WIDTH,
      STARTING_RESOLUTION_HEIGHT,
      NULL,
      NULL,
      instanceHandle,
      NULL
    );

    if (!windowHandle)
    {
      return false;
    }

    // ShowWindow(windowHandle, commandShow);
    // UpdateWindow(windowHandle);

    return true;
  }
}

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam)
{

  switch (message)
  {
  case WM_PAINT:
    Systems::Graphics::GetInstance()->Render();
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    Core::Engine::GetInstance()->Stop();
    break;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
    break;
  }

  return 0;
}