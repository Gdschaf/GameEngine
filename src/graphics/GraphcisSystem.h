#pragma once

#include <wrl.h>
#include <dxgi1_6.h>

#include <cstdint>
#include <chrono>

#include "core\EngineSystem.h"

#define DEBUG_GRAPHICS
#define NUM_FRAMES 3 // The number of swap chain back buffers.

using namespace Microsoft::WRL;

namespace Systems 
{
  class Graphics : public EngineSystem
  {
  public:
    Graphics() {}
    static Graphics* GetInstance()
    {
      static Graphics instance;
      return &instance;
    }

    virtual void Initialize();
    virtual void Update(float dt) {}
    virtual void FixedUpdate(float dt) {}
    virtual void Cleanup();
    void Render();
  private:
    // Used for the singleton, C++ 11, deletes these functions
    Graphics(Graphics const&) = delete;
    void operator=(Graphics const&) = delete;

    bool vsync = true;
    bool tearingSupported = false;
    bool initialized = false;

    ComPtr<ID3D12Device2> device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<IDXGISwapChain4> swapChain;
    ComPtr<ID3D12Resource> backBuffers[NUM_FRAMES];
    ComPtr<ID3D12CommandAllocator> commandAllocators[NUM_FRAMES];
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
    UINT rtvDescriptorSize = 0;
    UINT currentBackBufferIndex = 0;

    // Synchronization objects
    ComPtr<ID3D12Fence> fence;
    uint64_t fenceValue = 0;
    uint64_t frameFenceValues[NUM_FRAMES] = {};
    HANDLE fenceEvent = nullptr;

    void throwIfFailed(HRESULT hr);
    void enableDebugLayer();
    ComPtr<IDXGIAdapter4> getAdapter();
    bool checkTearingSupport();
    HANDLE createEventHandle();

    ComPtr<ID3D12Device2> createDevice(ComPtr<IDXGIAdapter4> adapter);

    ComPtr<ID3D12CommandQueue> createCommandQueue(
      ComPtr<ID3D12Device2> device,
      D3D12_COMMAND_LIST_TYPE type
    );

    ComPtr<IDXGISwapChain4> createSwapChain(
      HWND hWnd, 
      ComPtr<ID3D12CommandQueue> commandQueue, 
      uint32_t width, uint32_t height, 
      uint32_t bufferCount
    );

    ComPtr<ID3D12DescriptorHeap> createDescriptorHeap(
      ComPtr<ID3D12Device2> device, 
      D3D12_DESCRIPTOR_HEAP_TYPE type, 
      uint32_t numDescriptors
    );

    void updateRenderTargetViews(
      ComPtr<ID3D12Device2> device, 
      ComPtr<IDXGISwapChain4> swapChain, 
      ComPtr<ID3D12DescriptorHeap> descriptorHeap
    );

    ComPtr<ID3D12CommandAllocator> createCommandAllocator(
      ComPtr<ID3D12Device> device,
      D3D12_COMMAND_LIST_TYPE type
    );

    ComPtr<ID3D12GraphicsCommandList> createCommandList(
      ComPtr<ID3D12Device2> device,
      ComPtr<ID3D12CommandAllocator> commandAllocator,
      D3D12_COMMAND_LIST_TYPE type
    );

    ComPtr<ID3D12Fence> createFence(ComPtr<ID3D12Device2> device);

    uint64_t signal(
      ComPtr<ID3D12CommandQueue> commandQueue,
      ComPtr<ID3D12Fence> fence,
      uint64_t& fenceValue
    );

    void waitForFenceValue(
      ComPtr<ID3D12Fence> fence,
      uint64_t fenceValue,
      HANDLE fenceEvent,
      std::chrono::milliseconds duration = std::chrono::milliseconds::max()
    );

    void flush(
      ComPtr<ID3D12CommandQueue> commandQueue,
      ComPtr<ID3D12Fence> fence,
      uint64_t& fenceValue,
      HANDLE fenceEvent
    );
  };
}