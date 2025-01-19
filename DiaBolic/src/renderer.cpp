/*
*   Code written using these tutorials:
*     - https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics
*     - https://www.3dgep.com/category/graphics-programming/directx/
*/

#include "renderer.hpp"

#include "utility/dx12_helpers.hpp"
#include "utility/resource_util.hpp"
#include "glfw_app.hpp"
#include "descriptor_heap.hpp"
#include "command_queue.hpp"
#include "camera.hpp"

#include "pipelines/geometry_pipeline.hpp"
#include "pipelines/ui_pipeline.hpp"


Renderer::Renderer(std::shared_ptr<Application> app) :
	_app(app),
    _width(_app->GetWidth()),
    _height(_app->GetHeight()),
	_viewport(0.0f, 0.0f, static_cast<float>(_width), static_cast<float>(_height)),
	_scissorRect(0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height)),
    _useWarpDevice(false)
{
    _aspectRatio = static_cast<float>(_width) / static_cast<float>(_height);
    _camera = std::make_shared<Camera>();

    InitializeCore();
    InitializeCommandQueues();
    InitializeDescriptorHeaps();
    InitializeSwapchainResources();
    CreateDepthTarget();

    // Create pipelines
    _geometryPipeline = std::make_unique<GeometryPipeline>(*this, _camera);
    _uiPipeline = std::make_unique<UIPipeline>(*this);
}

Renderer::~Renderer()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    Flush();
}

void Renderer::Update(float deltaTime)
{
    _geometryPipeline->Update(deltaTime);
}

void Renderer::Render()
{
    auto commandList = _directCommandQueue->GetCommandList();

    // Set heaps for bindless.
    SetDescriptorHeaps(commandList);

    // Clear targets.
    auto rtvHandle = _rtvHeap->GetDescriptorHandleFromIndex(_renderTargetIndex[_frameIndex]);
    auto dsvHandle = _dsvHeap->GetDescriptorHandleFromIndex(_depthTargetIndex);
    Util::TransitionResource(commandList, _renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ClearRenderTargetView(rtvHandle.cpuDescriptorHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle.cpuDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Record command lists.
    commandList->OMSetRenderTargets(1, &rtvHandle.cpuDescriptorHandle, FALSE, &dsvHandle.cpuDescriptorHandle);
    commandList->RSSetViewports(1, &_viewport);
    commandList->RSSetScissorRects(1, &_scissorRect);
    _geometryPipeline->PopulateCommandlist(commandList);

    // Sync up resource(s) (might need this in between some stages later)
    Util::TransitionResource(commandList, _renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    // Execute commandlist.
    uint64_t fenceValue = _directCommandQueue->ExecuteCommandList(commandList);
    _fenceValues[_frameIndex] = fenceValue;

    // Present the frame.
    Util::ThrowIfFailed(_swapChain->Present(1, 0));

    // Wait for new back buffer to be done.
    _frameIndex = _swapChain->GetCurrentBackBufferIndex();
    _directCommandQueue->WaitForFenceValue(_fenceValues[_frameIndex]);
}

void Renderer::Flush()
{
    _directCommandQueue->Flush();
    _copyCommandQueue->Flush();
}

void Renderer::SetDescriptorHeaps(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& commandList) const
{
    const std::array<ID3D12DescriptorHeap* const, 2u> shaderVisibleDescriptorHeaps = {
        _srvHeap->GetDescriptorHeap(),
        _samplerHeap->GetDescriptorHeap(),
    };

    commandList->SetDescriptorHeaps(shaderVisibleDescriptorHeaps.size(), shaderVisibleDescriptorHeaps.data());
}


void Renderer::InitializeCore()
{
    UINT dxgiFactoryFlags = 0;

    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
#if defined(_DEBUG)
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    // Create Device.
    // https://www.3dgep.com/learning-directx-12-1/#Create_the_DirectX_12_Device
    // The DirectX 12 device is used to create resources (such as textures and buffers,
    // command lists, command queues, fences, heaps, etc�). It's not directly used for issuing draw or dispatch commands.
    // It can be considered a memory context that tracks allocations in GPU memory.
    Util::ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&_factory)));

    if (_useWarpDevice)
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
        Util::ThrowIfFailed(_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        Util::ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&_device)
        ));
    }
    else
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
        Util::GetHardwareAdapter(_factory.Get(), &hardwareAdapter, false); // bool: request for high performance adapter or not?

        Util::ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&_device)
        ));
    }

    Util::CheckFeatureSupport(_device);

}

void Renderer::InitializeCommandQueues()
{
    _directCommandQueue = std::make_unique<CommandQueue>(_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    _copyCommandQueue = std::make_unique<CommandQueue>(_device, D3D12_COMMAND_LIST_TYPE_COPY);
}

void Renderer::InitializeDescriptorHeaps()
{
    _rtvHeap = std::make_unique<DescriptorHeap>(_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_COUNT, L"Render Target View");
    _dsvHeap = std::make_unique<DescriptorHeap>(_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, L"Depth Stencil View");
    _srvHeap = std::make_unique<DescriptorHeap>(_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MAX_CBV_SRV_UAV_COUNT, L"Shader Resource View");
    _samplerHeap = std::make_unique<DescriptorHeap>(_device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                                                               D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE, L"Sampler Descriptor Heap");
}

void Renderer::InitializeSwapchainResources()
{
    // Describe and create the swap chain.
    // https://www.3dgep.com/learning-directx-12-1/#Create_the_Swap_Chain
    // The primary purpose of the swap chain is to present the rendered image to the screen.
    // Back buffer: currently being rendered to
    // Front buffer: currently being presented
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = _app->GetWidth();
    swapChainDesc.Height = _app->GetHeight();
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    Util::ThrowIfFailed(_factory->CreateSwapChainForHwnd(
        _directCommandQueue->GetCommandQueue().Get(),        // Swap chain needs the queue so that it can force a flush on it.
        _app->GetHWND(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    // This sample does not support fullscreen transitions.
    Util::ThrowIfFailed(_factory->MakeWindowAssociation(_app->GetHWND(), DXGI_MWA_NO_ALT_ENTER));

    Util::ThrowIfFailed(swapChain.As(&_swapChain));
    _frameIndex = _swapChain->GetCurrentBackBufferIndex();

    CreateRenderTargets();
}

void Renderer::CreateRenderTargets()
{
    // Create a RTV for each frame.
    for (UINT n = 0; n < FRAME_COUNT; n++)
    {
        Util::ThrowIfFailed(_swapChain->GetBuffer(n, IID_PPV_ARGS(&_renderTargets[n])));
        const D3D12_RENDER_TARGET_VIEW_DESC desc = {
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        };
        _renderTargetIndex[n] = CreateRtv(desc, _renderTargets[n]);
        std::wstring name = std::wstring(L"Render Target ") + std::to_wstring(n);
        _renderTargets[n]->SetName(name.c_str());
    }
}

void Renderer::CreateDepthTarget()
{
    // Flush any GPU commands that might be referencing the depth buffer.
    Flush();

    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, _width, _height,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    Util::ThrowIfFailed(_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&_depthTarget)
    ));

    const D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {
        .Format = DXGI_FORMAT_D32_FLOAT,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE,
        .Texture2D = {
            .MipSlice = 0
        }
    };

    _depthTargetIndex = CreateDsv(dsv, _depthTarget);
    _depthTarget->SetName(L"Depth Target");
}

uint32_t Renderer::CreateCbv(const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvCreationDesc) const
{
    const uint32_t cbvIndex = _srvHeap->GetCurrentDescriptorIndex();

    _device->CreateConstantBufferView(&cbvCreationDesc,
                                       _srvHeap->GetCurrentDescriptorHandle().cpuDescriptorHandle);

    _srvHeap->OffsetCurrentHandle();

    return cbvIndex;
}

uint32_t Renderer::CreateSrv(const D3D12_SHADER_RESOURCE_VIEW_DESC& srvCreationDesc, const Microsoft::WRL::ComPtr<ID3D12Resource>& resource) const
{
    const uint32_t srvIndex = _srvHeap->GetCurrentDescriptorIndex();

    _device->CreateShaderResourceView(resource.Get(), &srvCreationDesc,
                                       _srvHeap->GetCurrentDescriptorHandle().cpuDescriptorHandle);

    _srvHeap->OffsetCurrentHandle();

    return srvIndex;
}

uint32_t Renderer::CreateUav(const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavCreationDesc, const Microsoft::WRL::ComPtr<ID3D12Resource>& resource) const
{
    const uint32_t uavIndex = _srvHeap->GetCurrentDescriptorIndex();

    _device->CreateUnorderedAccessView(
        resource.Get(), nullptr, &uavCreationDesc,
        _srvHeap->GetCurrentDescriptorHandle().cpuDescriptorHandle);

    _srvHeap->OffsetCurrentHandle();

    return uavIndex;
}

uint32_t Renderer::CreateRtv(const D3D12_RENDER_TARGET_VIEW_DESC& rtvCreationDesc, const Microsoft::WRL::ComPtr<ID3D12Resource>& resource) const
{
    const uint32_t rtvIndex = _rtvHeap->GetCurrentDescriptorIndex();

    _device->CreateRenderTargetView(resource.Get(), &rtvCreationDesc,
                                     _rtvHeap->GetCurrentDescriptorHandle().cpuDescriptorHandle);

    _rtvHeap->OffsetCurrentHandle();

    return rtvIndex;
}

uint32_t Renderer::CreateDsv(const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvCreationDesc, const Microsoft::WRL::ComPtr<ID3D12Resource>& resource) const
{
    const uint32_t dsvIndex = _dsvHeap->GetCurrentDescriptorIndex();

    _device->CreateDepthStencilView(resource.Get(), &dsvCreationDesc,
                                     _dsvHeap->GetCurrentDescriptorHandle().cpuDescriptorHandle);

    _dsvHeap->OffsetCurrentHandle();

    return dsvIndex;
}