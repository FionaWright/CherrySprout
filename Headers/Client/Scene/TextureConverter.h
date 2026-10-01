#ifndef H_TEXTURE_CONVERTER_H
#define H_TEXTURE_CONVERTER_H

#include "HWI/D3D.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "Utils/CBVs.h"

class TextureConverter
{
public:
    void Init(const D3D* d3d);

    void Convert(const D3D* d3d, Heap* heap, ID3D12GraphicsCommandList* cmdList, D12Resource* source, TextureConvertMode mode, D12Resource* dest4, D12Resource* dest1 = nullptr);

private:
    Pipeline m_pipeline;
    RootSig m_rootSig;
    RootConstants m_rootConstants;
};

#endif