//
// Created by fionaw on 25/09/2025.
//

#include "System/pch.h"
#include "Render/CameraController.h"

#include "imgui.h"
#include "System/Config.h"
#include "System/Input.h"

bool CameraController::UpdateCamera(const double deltaTime_ms)
{
    const bool mouseOverGUI = Input::GetMousePos().x < Config::GetSystem().WindowAppGuiWidth || Input::GetMousePos().x > Config::GetSystem().WindowAppGuiWidth + Config::GetSystem().RtvWidth;
    const bool inTextField = ImGui::GetIO().WantCaptureKeyboard;

    bool changed = false;

    if (Input::IsMouseRight() && !mouseOverGUI)
    {
        XMFLOAT2 deltaMouse = Input::GetMousePosDelta();
        deltaMouse.x *= m_rotationSpeed;
        deltaMouse.y *= m_rotationSpeed;
        m_camera.Rotate(deltaMouse.x, deltaMouse.y);

        changed = deltaMouse.x != 0 || deltaMouse.y != 0;
    }

    XMFLOAT3 right, up, forward;
    m_camera.GetBasis(right, up, forward);

    if (Input::IsMouseMiddle() && !mouseOverGUI)
    {
        XMFLOAT2 deltaMouse = Input::GetMousePosDelta();
        const float panSpeed = m_speedPan;
        deltaMouse.x *= -panSpeed;
        deltaMouse.y *= panSpeed;

        const XMFLOAT3 upTranslation = XMFLOAT3(up.x * deltaMouse.y, up.y * deltaMouse.y, up.z * deltaMouse.y);
        m_camera.AddPosition(upTranslation);

        const XMFLOAT3 rightTranslation = XMFLOAT3(right.x * deltaMouse.x, right.y * deltaMouse.x, right.z * deltaMouse.x);
        m_camera.AddPosition(rightTranslation);

        changed =  deltaMouse.x != 0 || deltaMouse.y != 0;
    }

    if (inTextField)
        return changed;

    float forwardScalar = 0.0f;
    if (!mouseOverGUI)
        forwardScalar = Input::GetMouseWheelDelta() * m_speedScroll;

    const float speedPerFrameWASD = m_speedWASD * deltaTime_ms;

    if (Input::IsKey(KeyCode::W))
    {
        forwardScalar += speedPerFrameWASD;
    }
    else if (Input::IsKey(KeyCode::S))
    {
        forwardScalar -= speedPerFrameWASD;
    }

    const XMFLOAT3 forwardTranslation = XMFLOAT3(forward.x * forwardScalar, forward.y * forwardScalar, forward.z * forwardScalar);
    m_camera.AddPosition(forwardTranslation);

    float rightScalar = 0;
    if (Input::IsKey(KeyCode::A))
    {
        rightScalar -= speedPerFrameWASD;
    }
    else if (Input::IsKey(KeyCode::D))
    {
        rightScalar += speedPerFrameWASD;
    }

    const XMFLOAT3 rightTranslation = XMFLOAT3(right.x * rightScalar, right.y * rightScalar, right.z * rightScalar);
    m_camera.AddPosition(rightTranslation);

    float upScalar = 0;
    if (Input::IsKey(KeyCode::E))
    {
        upScalar += speedPerFrameWASD;
    }
    else if (Input::IsKey(KeyCode::Q))
    {
        upScalar -= speedPerFrameWASD;
    }

    const XMFLOAT3 upTranslation = XMFLOAT3(up.x * upScalar, up.y * upScalar, up.z * upScalar);
    m_camera.AddPosition(upTranslation);

    return changed || forwardScalar != 0 || rightScalar != 0 || upScalar != 0;
}
