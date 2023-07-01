#pragma once

namespace Systems
{
  class EngineSystem
  {
  public:
    virtual void Initialize() = 0;
    virtual void Update(float dt) = 0;
    virtual void FixedUpdate(float dt) = 0;
    virtual void Cleanup() = 0;
  };
}