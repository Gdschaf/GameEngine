#include "PrecompiledHeader.h"
#include "graphics/GraphcisSystem.h"
#include "windows/WindowsSystem.h"

#include <iostream>
#include <cassert>

// for reference: https://www.3dgep.com/learning-directx-12-1/

namespace Systems 
{
  void Graphics::Initialize()
  {
    enableDebugLayer();
    tearingSupported = checkTearingSupport();

    ComPtr<IDXGIAdapter4> dxgiAdapter4 = getAdapter();

    device = createDevice(dxgiAdapter4);

    commandQueue = createCommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    swapChain = createSwapChain(
      Systems::Windows::GetInstance()->GetWindowsHandle(), 
      commandQueue, 
      STARTING_RESOLUTION_WIDTH, 
      STARTING_RESOLUTION_HEIGHT, 
      NUM_FRAMES
    );

    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

    rtvDescriptorHeap = createDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_FRAMES);
    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    updateRenderTargetViews(device, swapChain, rtvDescriptorHeap);

    for (int i = 0; i < NUM_FRAMES; ++i)
    {
      commandAllocators[i] = createCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    commandList = createCommandList(device, commandAllocators[currentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

    fence = createFence(device);
    fenceEvent = createEventHandle();

    Systems::Windows::GetInstance()->Show();

    initialized = true;
  }

  // void Graphics::Update(float dt)
  // {
  //   render(); //should this be on WM_PAINT event?
  // }

  void Graphics::Cleanup()
  {
    flush(commandQueue, fence, fenceValue, fenceEvent);

    CloseHandle(fenceEvent);
  }

  void Graphics::Render()
  {
    if (!initialized)
      return;

    auto commandAllocator = commandAllocators[currentBackBufferIndex];
    auto backBuffer = backBuffers[currentBackBufferIndex];

    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), nullptr);

    // Clear the render target.
    {
      CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_PRESENT, 
        D3D12_RESOURCE_STATE_RENDER_TARGET
      );
    
      commandList->ResourceBarrier(1, &barrier);
      FLOAT clearColor[] = { 0.627f, 0.125f, 0.941f, 1.0f };
      CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
        rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        currentBackBufferIndex, 
        rtvDescriptorSize
      );
    
      commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // Present
    {
      CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
      commandList->ResourceBarrier(1, &barrier);

      throwIfFailed(commandList->Close());

      ID3D12CommandList* const commandLists[] = {
          commandList.Get()
      };
      commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

      UINT syncInterval = vsync ? 1 : 0;
      UINT presentFlags = tearingSupported && !vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
      throwIfFailed(swapChain->Present(syncInterval, presentFlags));

      frameFenceValues[currentBackBufferIndex] = signal(commandQueue, fence, fenceValue);

      currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

      waitForFenceValue(fence, frameFenceValues[currentBackBufferIndex], fenceEvent);
    }

  }

  void Graphics::throwIfFailed(HRESULT hr)
  {
    if (FAILED(hr))
    {
      throw std::exception();
    }
  }
  
  void Graphics::enableDebugLayer()
  {
#ifdef DEBUG_GRAPHICS
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    ComPtr<ID3D12Debug> debugInterface;
    throwIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
  }

  ComPtr<IDXGIAdapter4> Graphics::getAdapter()
  {
    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;
#ifdef DEBUG_GRAPHICS
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    throwIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;

    SIZE_T maxDedicatedVideoMemory = 0;
    for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
    {
      DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
      dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

      // Check to see if the adapter can create a D3D12 device without actually 
      // creating it. The adapter with the largest dedicated video memory
      // is favored.
      if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
        SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
          D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
        dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
      {
        maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
        throwIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
      }
    }

    return dxgiAdapter4;
  }

  ComPtr<ID3D12Device2> Graphics::createDevice(ComPtr<IDXGIAdapter4> adapter)
  {
    ComPtr<ID3D12Device2> d3d12Device2;
    throwIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

#ifdef DEBUG_GRAPHICS
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
    {
      pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
      pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
      pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

      // Suppress whole categories of messages
          //D3D12_MESSAGE_CATEGORY Categories[] = {};

          // Suppress messages based on their severity level
      D3D12_MESSAGE_SEVERITY Severities[] =
      {
          D3D12_MESSAGE_SEVERITY_INFO
      };

      // Suppress individual messages by their ID
      D3D12_MESSAGE_ID DenyIds[] = {
          D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
          D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
          D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
      };

      D3D12_INFO_QUEUE_FILTER NewFilter = {};
      //NewFilter.DenyList.NumCategories = _countof(Categories);
      //NewFilter.DenyList.pCategoryList = Categories;
      NewFilter.DenyList.NumSeverities = _countof(Severities);
      NewFilter.DenyList.pSeverityList = Severities;
      NewFilter.DenyList.NumIDs = _countof(DenyIds);
      NewFilter.DenyList.pIDList = DenyIds;

      throwIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif

    return d3d12Device2;
  }

  ComPtr<ID3D12CommandQueue> Graphics::createCommandQueue(
    ComPtr<ID3D12Device2> device, 
    D3D12_COMMAND_LIST_TYPE type
  )
  {
    ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    throwIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

    return d3d12CommandQueue;
  }

  bool Graphics::checkTearingSupport()
  {
    bool allowTearing = false;

    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
      ComPtr<IDXGIFactory5> factory5;
      if (SUCCEEDED(factory4.As(&factory5)))
      {
        if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
        {
          allowTearing = false;
        }
      }
    }

    return allowTearing;
  }


  ComPtr<IDXGISwapChain4> Graphics::createSwapChain(
    HWND hWnd, 
    ComPtr<ID3D12CommandQueue> commandQueue, 
    uint32_t width, 
    uint32_t height, 
    uint32_t bufferCount
  )
  {
    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#ifdef DEBUG_GRAPHICS
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    throwIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // allow tearing if tearing support is available.
    swapChainDesc.Flags = checkTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    throwIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
      commandQueue.Get(),
      hWnd,
      &swapChainDesc,
      nullptr,
      nullptr,
      &swapChain1));

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
    throwIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    throwIfFailed(swapChain1.As(&dxgiSwapChain4));

    return dxgiSwapChain4;
  }

  ComPtr<ID3D12DescriptorHeap> Graphics::createDescriptorHeap(
    ComPtr<ID3D12Device2> device, 
    D3D12_DESCRIPTOR_HEAP_TYPE type, 
    uint32_t numDescriptors
  )
  {
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;

    throwIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
  }

  void Graphics::updateRenderTargetViews(
    ComPtr<ID3D12Device2> device, 
    ComPtr<IDXGISwapChain4> swapChain, 
    ComPtr<ID3D12DescriptorHeap> descriptorHeap
  )
  {
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < NUM_FRAMES; ++i)
    {
      ComPtr<ID3D12Resource> backBuffer;
      throwIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

      device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

      backBuffers[i] = backBuffer;

      rtvHandle.Offset(rtvDescriptorSize);
    }
  }

  ComPtr<ID3D12CommandAllocator> Graphics::createCommandAllocator(
    ComPtr<ID3D12Device> device, 
    D3D12_COMMAND_LIST_TYPE type
  )
  {
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    throwIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
  }

  ComPtr<ID3D12GraphicsCommandList> Graphics::createCommandList(
    ComPtr<ID3D12Device2> device, 
    ComPtr<ID3D12CommandAllocator> commandAllocator, 
    D3D12_COMMAND_LIST_TYPE type
  )
  {
    ComPtr<ID3D12GraphicsCommandList> commandList;
    throwIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    throwIfFailed(commandList->Close());

    return commandList;
  }

  ComPtr<ID3D12Fence> Graphics::createFence(ComPtr<ID3D12Device2> device)
  {
    ComPtr<ID3D12Fence> fence;

    throwIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    return fence;
  }

  HANDLE Graphics::createEventHandle()
  {
    HANDLE fenceEvent;

    fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent && "Failed to create fence event.");

    return fenceEvent;
  }

  uint64_t Graphics::signal(
    ComPtr<ID3D12CommandQueue> commandQueue, 
    ComPtr<ID3D12Fence> fence, 
    uint64_t& fenceValue
  )
  {
    uint64_t fenceValueForSignal = ++fenceValue;
    throwIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));

    return fenceValueForSignal;
  }

  void Graphics::waitForFenceValue(
    ComPtr<ID3D12Fence> fence, 
    uint64_t fenceValue, 
    HANDLE fenceEvent, 
    std::chrono::milliseconds duration)
  {
    if (fence->GetCompletedValue() < fenceValue)
    {
      throwIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
      WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
    }
  }

  void Graphics::flush(
    ComPtr<ID3D12CommandQueue> commandQueue, 
    ComPtr<ID3D12Fence> fence, 
    uint64_t& fenceValue, 
    HANDLE fenceEvent
  )
  {
    uint64_t fenceValueForSignal = signal(commandQueue, fence, fenceValue);
    waitForFenceValue(fence, fenceValueForSignal, fenceEvent);
  }
}