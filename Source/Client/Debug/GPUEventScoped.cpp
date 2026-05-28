//
// Created by fionaw on 27/10/2025.
//

#include "System/pch.h"

#include "Debug/GPUEventScoped.h"

#ifdef _DEBUG
#   include <WinPixEventRuntime/pix3.h>
#endif

GPUEventScoped::GPUEventScoped(ID3D12GraphicsCommandList* cmdList, const LPCWSTR label)
{
#ifdef _DEBUG
    m_heldCmdList = cmdList;
    PIXBeginEvent(cmdList, 0, label);
#endif
}

GPUEventScoped::GPUEventScoped(ID3D12GraphicsCommandList* cmdList, const LPCSTR label)
{
#ifdef _DEBUG
    m_heldCmdList = cmdList;
    PIXBeginEvent(cmdList, 0, label);
#endif
}

GPUEventScoped::~GPUEventScoped()
{
#ifdef _DEBUG
    PIXEndEvent(m_heldCmdList);
#endif
}
