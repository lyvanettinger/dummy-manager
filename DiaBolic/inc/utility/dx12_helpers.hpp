#pragma once

namespace Util
{
    void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter);

    void CheckFeatureSupport(const Microsoft::WRL::ComPtr<ID3D12Device>& device);


    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::exception();
        }
    }
}