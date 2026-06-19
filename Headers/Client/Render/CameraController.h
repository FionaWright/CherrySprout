//
// Created by fionaw on 25/09/2025.
//

#ifndef PT_CAMERACONTROLLER_H
#define PT_CAMERACONTROLLER_H

#include "Camera.h"

class CameraController
{
public:
    void Init(const XMFLOAT3& pos, const float pitch, const float yaw) { m_camera.Init(pos, pitch, yaw); }
    bool UpdateCamera(double deltaTime_ms);

    [[nodiscard]] XMMATRIX GetViewMatrix() const { return m_camera.GetViewMatrix(); }
    [[nodiscard]] Camera& GetCamera() { return m_camera; }
    [[nodiscard]] const Camera& GetCamera() const { return m_camera; }

private:
    Camera m_camera;
    float m_speedScroll = 5.0f;
    float m_speedPan = 0.02f;
    float m_speedWASD = 1.0f;
    float m_rotationSpeed = 0.01f;
};

#endif //PT_CAMERACONTROLLER_H