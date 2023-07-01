#pragma once

#include "core\EngineSystem.h"

#include <list>

#define FIXED_FPS 60

#define THROTTLE_FPS
#define TARGET_FPS 120

namespace Core
{
  class Engine
  {
  public:
    Engine() {}
    static Engine* GetInstance()
    {
      static Engine instance;
      return &instance;
    }

    void AddSystem(Systems::EngineSystem* system);
    void Start();
    void Stop();
    unsigned int GetFPS() { return framesPerSecond; }
  protected:
    Engine(Engine const&) = delete;
    void operator=(Engine const&) = delete;

    void Loop();
    void Cleanup();
  private:
    std::list<Systems::EngineSystem*> _systems;
    bool _interruptLoop = false;
    unsigned int framesPerSecond = 0;
  };
}