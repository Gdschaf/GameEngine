#pragma once

#include "core\EngineSystem.h"

#define WINDOW_CLASS_NAME "GameEngine"
#define WINDOW_TITLE_BAR "Testing game engine window"

#define STARTING_RESOLUTION_WIDTH 800
#define STARTING_RESOLUTION_HEIGHT 600

namespace Systems
{
  class Windows : public EngineSystem
  {
  public:
    Windows() {}
    static Windows* GetInstance()
    {
      static Windows instance;
      return &instance;
    }

    virtual void Initialize();
    virtual void Update(float dt);
    virtual void FixedUpdate(float dt) {}
    virtual void Cleanup() {}

    void SetMainParameters(HINSTANCE hInstance, int nCmdShow);
    HINSTANCE GetInstanceHandle() { return instanceHandle; }
    HWND GetWindowsHandle() { return windowHandle; }
    void Show();
  private:
    // Used for singleton, C++ 11, deletes these functions
    Windows(Windows const&) = delete;
    void operator=(Windows const&) = delete;

    bool registerClass();
    bool initWindow();
    HINSTANCE instanceHandle{}; //these curly braces apparently initialize to 0? 
    HWND windowHandle{};
    int commandShow{};
  };
}