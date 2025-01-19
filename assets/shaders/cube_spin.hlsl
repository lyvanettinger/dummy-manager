#include "constant_buffers.hlsli"

struct VSOutput
{
    float2 uv : TEXCOORD;
    float3 normal : NORMAL0;
    float4 position : SV_POSITION;
};

ConstantBuffer<SceneResources> scene : register(b0);

VSOutput VSmain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[scene.positionBufferIndex];
    StructuredBuffer<float2> uvBuffer = ResourceDescriptorHeap[scene.uvBufferIndex];
    StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[scene.normalBufferIndex];

    VSOutput result;
    result.position = mul(scene.MVP, float4(positionBuffer[vertexID], 1.0f));
    result.normal = normalBuffer[vertexID]; // TODO: multiply with inverse transpose
    result.uv = uvBuffer[vertexID];

    return result;
}

SamplerState defaultSampler : register(s0);

float4 PSmain(VSOutput PSinput) : SV_Target0
{
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[scene.textureIndex];
    return albedoTexture.Sample(defaultSampler, PSinput.uv);
}