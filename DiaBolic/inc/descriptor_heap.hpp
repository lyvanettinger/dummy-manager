#pragma once

struct DescriptorHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

    uint32_t descriptorSize{};

    void offset()
    {
        cpuDescriptorHandle.ptr += descriptorSize;
        gpuDescriptorHandle.ptr += descriptorSize;
    }
};

class DescriptorHeap
{
public:
    explicit DescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device2>& const device, const D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType,
                                const uint32_t descriptorCount, const std::wstring& descriptorHeapName);
    ~DescriptorHeap() = default;

    DescriptorHeap(const DescriptorHeap& other) = delete;
    DescriptorHeap& operator=(const DescriptorHeap& other) = delete;

    DescriptorHeap(DescriptorHeap&& other) = delete;
    DescriptorHeap& operator=(DescriptorHeap&& other) = delete;

    ID3D12DescriptorHeap* const getDescriptorHeap() const
    {
        return _descriptorHeap.Get();
    }

    uint32_t getDescriptorSize() const
    {
        return _descriptorSize;
    };

    DescriptorHandle getDescriptorHandleFromStart() const
    {
        return _descriptorHandleFromHeapStart;
    };

    DescriptorHandle getCurrentDescriptorHandle() const
    {
        return _currentDescriptorHandle;
    };

    DescriptorHandle getDescriptorHandleFromIndex(const uint32_t index) const;

    // Returns a index that can be used to directly index into a descriptor heap.
    [[nodiscard]] uint32_t getDescriptorIndex(const DescriptorHandle& descriptorHandle) const;
    [[nodiscard]] uint32_t getCurrentDescriptorIndex() const;

    // Used to offset a X_Handle passed into function.
    void offsetDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& handle, const uint32_t offset = 1u) const;
    void offsetDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE& handle, const uint32_t offset = 1u) const;
    void offsetDescriptor(DescriptorHandle& handle, const uint32_t offset = 1u) const;

    void offsetCurrentHandle(const uint32_t offset = 1u);

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _descriptorHeap{};
    uint32_t _descriptorSize{};

    DescriptorHandle _descriptorHandleFromHeapStart{};
    DescriptorHandle _currentDescriptorHandle{};
};