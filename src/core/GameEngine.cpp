#include "PrecompiledHeader.h"
#include "core\GameEngine.h"

#include <chrono>
#include <ctime>

#include <iostream>

namespace Core
{
  void Engine::AddSystem(Systems::EngineSystem* system)
  {
    _systems.push_back(system);
  }

  void Engine::Start()
  {
    auto itEnd = _systems.end();
    for (auto it = _systems.begin(); it != itEnd; ++it)
    {
      (*it)->Initialize();
    }

    Loop();
  }

  void Engine::Loop()
  {
    auto previousFrameTime = std::chrono::system_clock::now();
    auto fixedDuration = std::chrono::duration<double>(0.0);
    auto targetDuration = std::chrono::duration<double>(0.0);
    auto fpsDuration = std::chrono::duration<double>(0.0);
    double fixedFrameTime = 1.0 / FIXED_FPS;
    double targetFrameTime = 1.0 / TARGET_FPS;
    unsigned int frameCount = 0;

    while (_interruptLoop == false)
    {
      auto currentFrameTime = std::chrono::system_clock::now();
      auto elapsedFrameTime = std::chrono::duration_cast<std::chrono::duration<double>>(currentFrameTime - previousFrameTime);
      fixedDuration += elapsedFrameTime;
      fpsDuration += elapsedFrameTime;
      previousFrameTime = currentFrameTime;

      auto itEnd = _systems.end();
#ifdef THROTTLE_FPS
      targetDuration += elapsedFrameTime;
      if (targetDuration.count() >= targetFrameTime)
      {
        for (auto it = _systems.begin(); it != itEnd; ++it)
        {
          (*it)->Update(targetDuration.count()); //conversion from '_Rep' to 'float', possible loss of data <- figure this out
        }
        targetDuration = std::chrono::duration<double>(0.0);
      }
#endif
#ifndef THROTTLE_FPS
      for (auto it = _systems.begin(); it != itEnd; ++it)
      {
        (*it)->Update(elapsedFrameTime.count());
      }
#endif

      if (fixedDuration.count() >= fixedFrameTime)
      {
        auto itEnd = _systems.end();
        for (auto it = _systems.begin(); it != itEnd; ++it)
        {
          (*it)->FixedUpdate((float)fixedFrameTime);
        }
        fixedDuration -= std::chrono::duration<double>(fixedFrameTime);
      }

      ++frameCount;
      if (fpsDuration.count() >= 1.0)
      {
        framesPerSecond = frameCount;

        frameCount = 0;
        fpsDuration = std::chrono::duration<double>(0.0);
      }
    }

    Cleanup();
  }

  void Engine::Cleanup()
  {
    auto itEnd = _systems.end();
    for (auto it = _systems.begin(); it != itEnd; ++it)
    {
      (*it)->Cleanup();
    }
  }

  void Engine::Stop()
  {
    _interruptLoop = true;
  }
}