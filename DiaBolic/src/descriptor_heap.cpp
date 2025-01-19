#include "descriptor_heap.hpp"

#include "utility/dx12_helpers.hpp"

DescriptorHeap::DescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device2>& device, const D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType,
                                   const uint32_t descriptorCount, const std::wstring& descriptorHeapName)
    {
        const D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags = (descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV ||
                                                                 descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
                                                                    ? D3D12_DESCRIPTOR_HEAP_FLAG_NONE
                                                                    : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
        descriptorHeapDesc.Type = descriptorHeapType;
        descriptorHeapDesc.NumDescriptors = descriptorCount;
        descriptorHeapDesc.Flags = descriptorHeapFlags;
        descriptorHeapDesc.NodeMask = 0u;

        Util::ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&_descriptorHeap)));
        _descriptorHeap->SetName(descriptorHeapName.data());

        _descriptorSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

        _descriptorHandleFromHeapStart.cpuDescriptorHandle = _descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        _descriptorHandleFromHeapStart.gpuDescriptorHandle = (descriptorHeapFlags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
                                       ? _descriptorHeap->GetGPUDescriptorHandleForHeapStart()
                                       : CD3DX12_GPU_DESCRIPTOR_HANDLE{};
        _descriptorHandleFromHeapStart.descriptorSize = _descriptorSize;

        _currentDescriptorHandle = _descriptorHandleFromHeapStart;
    }

    DescriptorHandle DescriptorHeap::GetDescriptorHandleFromIndex(const uint32_t index) const
    {
        DescriptorHandle handle = GetDescriptorHandleFromStart();
        OffsetDescriptor(handle, index);

        return std::move(handle);
    }

    uint32_t DescriptorHeap::GetDescriptorIndex(const DescriptorHandle& descriptorHandle) const
    {
        return static_cast<uint32_t>(
            (descriptorHandle.gpuDescriptorHandle.ptr - _descriptorHandleFromHeapStart.gpuDescriptorHandle.ptr) /
            _descriptorSize);
    }

    uint32_t DescriptorHeap::GetCurrentDescriptorIndex() const
    {
        return GetDescriptorIndex(_currentDescriptorHandle);
    }

    void DescriptorHeap::OffsetDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& handle, const uint32_t offset) const
    {
        handle.ptr += _descriptorSize * static_cast<unsigned long long>(offset);
    }

    void DescriptorHeap::OffsetDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE& handle, const uint32_t offset) const
    {
        handle.ptr += _descriptorSize * static_cast<unsigned long long>(offset);
    }

    void DescriptorHeap::OffsetDescriptor(DescriptorHandle& descriptorHandle, const uint32_t offset) const
    {
        descriptorHandle.cpuDescriptorHandle.ptr += _descriptorSize * static_cast<unsigned long long>(offset);
        descriptorHandle.gpuDescriptorHandle.ptr += _descriptorSize * static_cast<unsigned long long>(offset);
    }

    void DescriptorHeap::OffsetCurrentHandle(const uint32_t offset)
    {
        OffsetDescriptor(_currentDescriptorHandle, offset);
    }