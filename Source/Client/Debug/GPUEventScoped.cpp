//
// Created by fionaw on 27/10/2025.
//

#include "System/pch.h"

#include "Debug/GPUEventScoped.h"

#include "Utils/Helper.h"

#if !NDEBUG
#   include <WinPixEventRuntime/pix3.h>
#endif

GPUEventScoped::GPUEventScoped(ID3D12GraphicsCommandList* cmdList, const LPCWSTR label)
{
#if !NDEBUG
    m_heldCmdList = cmdList;
    PIXBeginEvent(cmdList, 0, label);
#endif
}

GPUEventScoped::GPUEventScoped(ID3D12GraphicsCommandList* cmdList, const LPCSTR label)
{
#if !NDEBUG
    m_heldCmdList = cmdList;
    PIXBeginEvent(cmdList, 0, label);
#endif
}

GPUEventScoped::~GPUEventScoped()
{
#if !NDEBUG
    PIXEndEvent(m_heldCmdList);
#endif
}
