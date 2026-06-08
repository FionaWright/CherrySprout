//
// Created by fiona on 25/09/2025.
//

#include "System/pch.h"
#include "Render/Camera.h"
#include "Utils/SharedUtils.h"

void Camera::Init(const XMFLOAT3 pos, const float pitch, const float yaw)
{
    m_pos = pos;

    SetRotation(pitch, yaw);
}

XMMATRIX Camera::GetViewMatrix() const
{
    const XMVECTOR forward = XMVector3Rotate(XMVectorSet(0,0,1,0), m_orientation);
    const XMVECTOR up = XMVector3Rotate(XMVectorSet(0,1,0,0), m_orientation);

    const XMVECTOR positionVector = XMLoadFloat3(&m_pos);

    return XMMatrixLookToLH(positionVector, forward, up);
}

void Camera::SetRotation(const XMFLOAT2 pitchYaw)
{
    SetRotation(pitchYaw.x, pitchYaw.y);
}

void Camera::SetRotation(const float pitch, const float yaw)
{
    m_yaw = yaw;
    m_pitch = pitch;

    const XMVECTOR yawQuat =
        XMQuaternionRotationAxis(
            XMVectorSet(0.f, 1.f, 0.f, 0.f),
            yaw);

    const XMVECTOR pitchQuat =
        XMQuaternionRotationAxis(
            XMVectorSet(1.f, 0.f, 0.f, 0.f),
            pitch);

    m_orientation =
        XMQuaternionNormalize(
            XMQuaternionMultiply(
                pitchQuat,
                yawQuat));
}

void Camera::Rotate(const float deltaYaw, const float deltaPitch)
{
    m_yaw += deltaYaw;
    m_pitch += deltaPitch;
    SetRotation(m_pitch, m_yaw);
}

void Camera::GetBasis(XMFLOAT3& right, XMFLOAT3& up, XMFLOAT3& forward) const
{
    const XMVECTOR rightV =
        XMVector3Rotate(
            XMVectorSet(1.f, 0.f, 0.f, 0.f),
            m_orientation);

    const XMVECTOR upV =
        XMVector3Rotate(
            XMVectorSet(0.f, 1.f, 0.f, 0.f),
            m_orientation);

    const XMVECTOR forwardV =
    XMVector3Rotate(
        XMVectorSet(0.f, 0.f, 1.f, 0.f),
        m_orientation);

    XMStoreFloat3(&right, rightV);
    XMStoreFloat3(&up, upV);
    XMStoreFloat3(&forward, forwardV);
}