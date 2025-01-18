struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VSOutput
{
    float2 uv : TEXCOORD;
    float3 normal : NORMAL0;
    float4 position : SV_POSITION;
};

cbuffer ModelViewProjectionCB : register(b0)
{
    matrix MVP;
};

VSOutput VSmain(VSInput VSinput)
{
    VSOutput result;

    result.position = mul(MVP, float4(VSinput.position, 1.0f));
    result.normal = VSinput.normal; // TODO: multiply with inverse transpose
    result.uv = VSinput.uv;

    return result;
}

// AlbedoTexture : register(t0);
// SamplerState AlbedoSampler : register(s0);

float4 PSmain(VSOutput PSinput) : SV_TARGET
{
    return float4(PSinput.uv.x, PSinput.uv.y, 0.0, 1.0);
    //return AlbedoTexture.Sample(AlbedoSampler, input.uv);
}