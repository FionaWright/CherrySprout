//
// Created by fiona on 25/09/2025.
//

#ifndef PT_CAMERA_H
#define PT_CAMERA_H

class Camera
{
public:
    void Init(XMFLOAT3 pos, float pitch, float yaw);
    XMMATRIX GetViewMatrix() const;
    void SetRotation(float pitch, float yaw);
    void Rotate(float deltaYaw, float deltaPitch);
    void RotateYaw(float radians);
    void RotatePitch(float radians);
    void GetBasis(XMFLOAT3& right, XMFLOAT3& up, XMFLOAT3& forward) const;

    XMFLOAT3 GetPosition() const { return m_pos; }
    void SetPosition(const XMFLOAT3& pos) { m_pos = pos; }
    void AddPosition(const XMFLOAT3 offset) { m_pos.x += offset.x; m_pos.y += offset.y; m_pos.z += offset.z; }

private:
    XMFLOAT3 m_pos = XMFLOAT3(0,0,0);
    float m_pitch = 0.0f, m_yaw = 0.0f;
    XMVECTOR m_orientation{};
};


#endif //PT_CAMERA_H