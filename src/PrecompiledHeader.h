#pragma once

#define NOMINMAX // Gets rid of the min max macros in windows.h that was conflicting with chrono::milliseconds::max() in GraphicsSystem.h

#include <windows.h>
#include <directx\d3dx12.h>