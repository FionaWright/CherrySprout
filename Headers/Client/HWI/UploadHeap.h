//
// Created by fionaw on 31/05/2026.
//

#ifndef CHERRYSPROUT_UPLOADHEAP_H
#define CHERRYSPROUT_UPLOADHEAP_H

#include "D12Resource.h"

#include <d3d12.h>
#include <cstdint>

class UploadHeap
{
public:
    void Init(ID3D12Device* device, size_t maxUploadSize);
    ~UploadHeap();

    size_t GetAssignedUploadOffset(size_t size, size_t alignmentRequirement);
    ID3D12Resource* GetUploadResource() const { return m_resource.GetResource(); }
    uint8_t* GetMappedPointer(size_t offset) const;

    bool IsInitialized() const { return m_mappedPointer != nullptr; }

    void FreeAssignedData();

private:
    D12Resource m_resource;
    size_t m_maxUploadSize = 0;
    size_t m_currUploadOffset = 0;
    uint8_t* m_mappedPointer = nullptr;
};


#endif //CHERRYSPROUT_UPLOADHEAP_H