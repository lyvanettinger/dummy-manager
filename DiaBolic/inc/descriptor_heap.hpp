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
    DescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device2>& device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType,
                                uint32_t descriptorCount, const std::wstring& descriptorHeapName);
    ~DescriptorHeap() = default;

    DescriptorHeap(const DescriptorHeap& other) = delete;
    DescriptorHeap& operator=(const DescriptorHeap& other) = delete;

    DescriptorHeap(DescriptorHeap&& other) = delete;
    DescriptorHeap& operator=(DescriptorHeap&& other) = delete;

    [[nodiscard]] ID3D12DescriptorHeap* const GetDescriptorHeap() const
    {
        return _descriptorHeap.Get();
    }

    [[nodiscard]] uint32_t GetDescriptorSize() const
    {
        return _descriptorSize;
    };

    [[nodiscard]] DescriptorHandle GetDescriptorHandleFromStart() const
    {
        return _descriptorHandleFromHeapStart;
    };

    [[nodiscard]] DescriptorHandle GetCurrentDescriptorHandle() const
    {
        return _currentDescriptorHandle;
    };

    [[nodiscard]] DescriptorHandle GetDescriptorHandleFromIndex(const uint32_t index) const;

    // Returns a index that can be used to directly index into a descriptor heap.
    [[nodiscard]] uint32_t GetDescriptorIndex(const DescriptorHandle& descriptorHandle) const;
    [[nodiscard]] uint32_t GetCurrentDescriptorIndex() const;

    // Used to offset a X_Handle passed into function.
    void OffsetDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& handle, const uint32_t offset = 1u) const;
    void OffsetDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE& handle, const uint32_t offset = 1u) const;
    void OffsetDescriptor(DescriptorHandle& handle, const uint32_t offset = 1u) const;

    void OffsetCurrentHandle(const uint32_t offset = 1u);

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _descriptorHeap{};
    uint32_t _descriptorSize{};

    DescriptorHandle _descriptorHandleFromHeapStart{};
    DescriptorHandle _currentDescriptorHandle{};
};