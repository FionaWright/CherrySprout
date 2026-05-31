//
// Created by fionaw on 31/05/2026.
//

#include "HWI/UploadHeap.h"

#include <cassert>

#include "Utils/Helper.h"

size_t Align(const size_t value, const size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

void UploadHeap::Init(ID3D12Device* device, const size_t maxUploadSize)
{
    if (m_mappedPointer)
    {
        m_resource.GetResource()->Unmap(0, nullptr);
    }

    m_maxUploadSize = maxUploadSize;

    m_resource.InitUpload("Upload Heap", device, m_maxUploadSize);

    V(m_resource.GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedPointer)));
}

UploadHeap::~UploadHeap()
{
    m_resource.GetResource()->Unmap(0, nullptr);
}

// CBVs must be 256-aligned
// ? must be 16-aligned
size_t UploadHeap::GetAssignedUploadOffset(const size_t size, const size_t alignmentRequirement)
{
    const size_t offset = Align(m_currUploadOffset, alignmentRequirement);
    m_currUploadOffset = offset + size;

    assert(m_currUploadOffset <= m_maxUploadSize);
    return offset;
}

uint8_t* UploadHeap::GetMappedPointer(const size_t offset) const
{
    assert(m_mappedPointer);
    return m_mappedPointer + offset;
}

void UploadHeap::UnreserveData() // Done when you know that the data isn't currently being used on the GPU nor in a current cmdList
{
    m_currUploadOffset = 0;
}
