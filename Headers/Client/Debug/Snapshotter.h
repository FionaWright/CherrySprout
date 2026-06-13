#ifndef H_SNAPSHOTTER_H
#define H_SNAPSHOTTER_H
#include <cstdint>
#include <dxgiformat.h>

#include "HWI/D3D.h"

class D12Resource;

class Snapshotter
{
public:
    static void ResourceToSnapshot(D3D* d3d, D12Resource* d12Resource, uint8_t*& data, size_t& dataSize);

    static const Image* PackData(const D3D* d3d, uint8_t* data, const D12Resource* d12Resource, ScratchImage& scratch);

    static void SnapshotToRgba8(const Image* image, ScratchImage& scratch);

    static void SnapshotToFile(const Image* image, const char* fileName, bool ldrIsPNG = false);

    static void Rgba8SnapshotToClipboard(const Image* image);
};

#endif