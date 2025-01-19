#include "utility/resource_util.hpp"
#include "utility/dx12_helpers.hpp"

#include "pipelines/geometry_pipeline.hpp"

#include "command_queue.hpp"
#include "renderer.hpp"
#include "camera.hpp"
#include "descriptor_heap.hpp"
#include "utility/shader_compiler.hpp"

using namespace Util;
using namespace Microsoft::WRL;

GeometryPipeline::GeometryPipeline(Renderer& renderer, std::shared_ptr<Camera>& camera)
    : _renderer(renderer)
    , _camera(camera)
{
    CreatePipeline();
    InitializeAssets();
}

GeometryPipeline::~GeometryPipeline()
{

}

void GeometryPipeline::PopulateCommandlist(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& commandList)
{
    // Set necessary stuff.
    commandList->SetPipelineState(_pipelineState.Get());
    commandList->SetGraphicsRootSignature(_rootSignature.Get());

    // Start recording.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetIndexBuffer(&_indexBufferView);

    // Update the MVP matrix
    XMMATRIX mvpMatrix = XMMatrixMultiply(_camera->model, _camera->view);
    mvpMatrix = XMMatrixMultiply(mvpMatrix, _camera->projection);
    _scene.MVP = mvpMatrix;
    commandList->SetGraphicsRoot32BitConstants(0, 64, &_scene, 0);

    commandList->DrawIndexedInstanced(_indexCount, 1, 0, 0, 0);
}

void GeometryPipeline::Update(float deltaTime)
{
    static double totalTime = 0.0f;
    totalTime += deltaTime;
    if (totalTime > 4.0f)
    {
        totalTime = 0.0f;
    }

    // Update the model matrix.
    float angle = static_cast<float>(totalTime * 90.0);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    _camera->model = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    // Update the view matrix.
    _camera->view = XMMatrixLookAtLH(_camera->position, _camera->position + _camera->front, _camera->up);

    // Update the projection matrix.
    _camera->projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(_camera->fov), _renderer._aspectRatio, 0.1f, 100.0f);
}

void GeometryPipeline::CreatePipeline()
{
    // Create an empty root signature.
    // https://www.3dgep.com/learning-directx-12-2/#Root_Signatures
    // A root signature defines the paramteres that are passed to the shader pipeline.
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
        D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

    CD3DX12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].InitAsConstants(64, 0, 0, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_STATIC_SAMPLER_DESC defaultSampler;
    defaultSampler.Init(0);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &defaultSampler, rootSignatureFlags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(_renderer._device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));

    const auto& vertexShaderBlob = ShaderCompiler::Compile(ShaderTypes::Vertex, L"assets/shaders/cube_spin.hlsl", L"VSmain").shaderBlob;
    const auto& pixelShaderBlob = ShaderCompiler::Compile(ShaderTypes::Pixel, L"assets/shaders/cube_spin.hlsl", L"PSmain").shaderBlob;

    // Setup blend descriptions.
    constexpr D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {
        .BlendEnable = FALSE,
        .LogicOpEnable = FALSE,
        .SrcBlend = D3D12_BLEND_SRC_ALPHA,
        .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
        .BlendOp = D3D12_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D12_BLEND_ONE,
        .DestBlendAlpha = D3D12_BLEND_ZERO,
        .BlendOpAlpha = D3D12_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
    };

    D3D12_BLEND_DESC blendDesc = {
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE,
    };

    for(uint8_t i = 0; i < FRAME_COUNT; i++)
    {
        blendDesc.RenderTarget[i] = renderTargetBlendDesc;
    }

    // Setup depth stencil state.
    const D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {
        .DepthEnable = TRUE,
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable = FALSE,
        .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
        .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
    };

    // Setup PSO.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature = _rootSignature.Get(),
        .VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()),
        .PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()),
        .BlendState = blendDesc,
        .SampleMask = UINT32_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState = depthStencilDesc,
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = FRAME_COUNT,
        .DSVFormat = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc{.Count = 1u, .Quality = 0u},
        .NodeMask = 0u,
    };

    for(uint8_t i = 0; i < FRAME_COUNT; i++)
    {
        psoDesc.RTVFormats[i] = DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    ThrowIfFailed(_renderer._device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pipelineState)));
}

void GeometryPipeline::InitializeAssets()
{
    auto commandList = _renderer._copyCommandQueue->GetCommandList();

    std::vector<XMFLOAT3> cubeVertices;
    std::vector<XMFLOAT3> cubeNormals;
    std::vector<XMFLOAT2> cubeUVs;
    std::vector<uint16_t> cubeIndices;
    CreateCube(cubeVertices, cubeNormals, cubeUVs, cubeIndices, 2.5f);

    // Create the positions buffer.
    ComPtr<ID3D12Resource> positionIntermediateBuffer;
    LoadBufferResource(_renderer._device, commandList,
        &_positionBuffer.resource, &positionIntermediateBuffer,
        cubeVertices.size(), sizeof(XMFLOAT3), cubeVertices.data());
    _positionBuffer.resource->SetName(L"Cube Positions");

    const D3D12_SHADER_RESOURCE_VIEW_DESC positionDesc = {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer = {
            .FirstElement = 0u,
            .NumElements = static_cast<UINT>(cubeVertices.size()),
            .StructureByteStride = static_cast<UINT>(sizeof(XMFLOAT3)),
          },
    };
    _positionBuffer.srvIndex = _renderer.CreateSrv(positionDesc, _positionBuffer.resource);


    // Create the normals buffer.
    ComPtr<ID3D12Resource> normalsIntermediateBuffer;
    LoadBufferResource(_renderer._device, commandList,
        &_normalBuffer.resource, &normalsIntermediateBuffer,
        cubeNormals.size(), sizeof(XMFLOAT3), cubeNormals.data());
    _normalBuffer.resource->SetName(L"Cube Normals");

    const D3D12_SHADER_RESOURCE_VIEW_DESC normalsDesc = {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer = {
            .FirstElement = 0u,
            .NumElements = static_cast<UINT>(cubeNormals.size()),
            .StructureByteStride = static_cast<UINT>(sizeof(XMFLOAT3)),
          },
    };
    _normalBuffer.srvIndex = _renderer.CreateSrv(normalsDesc, _normalBuffer.resource);


    // Create the uvs buffer.
    ComPtr<ID3D12Resource> uvIntermediateBuffer;
    LoadBufferResource(_renderer._device, commandList,
        &_uvBuffer.resource, &uvIntermediateBuffer,
        cubeUVs.size(), sizeof(XMFLOAT2), cubeUVs.data());
    _uvBuffer.resource->SetName(L"Cube UVs");

    const D3D12_SHADER_RESOURCE_VIEW_DESC uvDesc = {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer = {
            .FirstElement = 0u,
            .NumElements = static_cast<UINT>(cubeUVs.size()),
            .StructureByteStride = static_cast<UINT>(sizeof(XMFLOAT2)),
          },
    };
    _uvBuffer.srvIndex = _renderer.CreateSrv(uvDesc, _uvBuffer.resource);


    // Create the index buffer.
    ComPtr<ID3D12Resource> indexIntermediateBuffer;
    LoadBufferResource(_renderer._device, commandList,
        &_indexBuffer, &indexIntermediateBuffer,
        cubeIndices.size(), sizeof(uint16_t), cubeIndices.data());
    _indexCount = static_cast<uint32_t>(cubeIndices.size());
    _indexBuffer->SetName(L"Cube Indices");

    _indexBufferView = {
        .BufferLocation = _indexBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = static_cast<UINT>(_indexCount * sizeof(uint16_t)),
        .Format = DXGI_FORMAT_R16_UINT,
    };

    // Create the texture.
    ComPtr<ID3D12Resource> albedoIntermediateBuffer;
    DXGI_FORMAT format{};
    LoadTextureFromFile(_renderer._device, commandList,
        &_albedoTexture.resource, &albedoIntermediateBuffer,
        L"assets/textures/jeremygraphics.png", format);
    _albedoTexture.resource->SetName(L"jeremygraphics.png");

    const D3D12_SHADER_RESOURCE_VIEW_DESC textureDesc = {
        .Format = format,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0u,
            .MipLevels = 1u,
            .PlaneSlice = 0u,
          },
    };
    _albedoTexture.srvIndex = _renderer.CreateSrv(textureDesc, _albedoTexture.resource);


    _scene.positionBufferIndex = _positionBuffer.srvIndex;
    _scene.normalBufferIndex = _normalBuffer.srvIndex;
    _scene.uvBufferIndex = _uvBuffer.srvIndex;
    _scene.textureIndex = _albedoTexture.srvIndex;

    // Execute list
    uint64_t fenceValue = _renderer._copyCommandQueue->ExecuteCommandList(commandList);
    _renderer._copyCommandQueue->WaitForFenceValue(fenceValue);
}