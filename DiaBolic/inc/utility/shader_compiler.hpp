#pragma once

namespace Util
{
    struct Shader
    {
        Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob{};
        Microsoft::WRL::ComPtr<IDxcBlob> rootSignatureBlob{};
    };

    enum class ShaderTypes : uint8_t
    {
        Vertex,
        Pixel,
        Compute,
        RootSignature,
    };

    namespace ShaderCompiler
    {
        [[nodiscard]] Shader Compile(const ShaderTypes& shaderType, const std::wstring_view shaderPath,
                                     const std::wstring_view entryPoint, const bool extractRootSignature = false);
    }
}

