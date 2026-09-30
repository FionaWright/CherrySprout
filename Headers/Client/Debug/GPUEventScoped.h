//
// Created by fionaw on 27/10/2025.
//

#ifndef PT_GPUEVENTSCOPED_H
#define PT_GPUEVENTSCOPED_H

#if !NDEBUG

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

#define GPU_SCOPE(cmdList, label) GPUEventScoped CONCAT(scope_, __COUNTER__)(cmdList, label)

class GPUEventScoped
{
public:
    GPUEventScoped(ID3D12GraphicsCommandList* cmdList, LPCWSTR label);
    GPUEventScoped(ID3D12GraphicsCommandList* cmdList, LPCSTR label);
    ~GPUEventScoped();

    GPUEventScoped(const GPUEventScoped&) = delete;
    GPUEventScoped& operator=(const GPUEventScoped&) = delete;
    GPUEventScoped(GPUEventScoped&&) = delete;
    GPUEventScoped& operator=(GPUEventScoped&&) = delete;

private:
    ID3D12GraphicsCommandList* m_heldCmdList;
};

#else

#define GPU_SCOPE(cmdList, label)

#endif


#endif //PT_GPUEVENTSCOPED_H