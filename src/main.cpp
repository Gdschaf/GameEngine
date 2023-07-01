#include "PrecompiledHeader.h"
#include "core\GameEngine.h"
#include "windows\WindowsSystem.h"
#include "graphics\GraphcisSystem.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
  Core::Engine* engine = Core::Engine::GetInstance();
  Systems::Windows* windowsSystem = Systems::Windows::GetInstance();
  engine->AddSystem(windowsSystem);
  engine->AddSystem(Systems::Graphics::GetInstance());

  windowsSystem->SetMainParameters(hInstance, nCmdShow);

  engine->Start();
}