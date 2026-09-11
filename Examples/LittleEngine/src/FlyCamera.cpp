#include "LittleEngine/FlyCamera.hpp"

#include "Vela/Math/Math.hpp"

#include <algorithm>

namespace little
{
    void FlyCamera::frame(const vela::math::Vector3f& center, float diagonal)
    {
        m_speed = diagonal * 0.25f;

        m_camera.setNear(std::max(diagonal / 1000.0f, 0.01f));
        m_camera.setFar(diagonal * 3.0f);
        m_camera.setPosition({center.x, center.y, center.z + diagonal * 0.6f});
        m_camera.updateCameraVectors();
    }

    void FlyCamera::update(vela::core::Window& window, float deltaTime, bool acceptInput)
    {
        if (!acceptInput)
        {
            m_firstMouse = true;
            return;
        }

        auto position = m_camera.getPosition();

        const auto forward = m_camera.getForward();
        const auto right = vela::math::normalize(
            vela::math::cross(forward, vela::math::Vector3f(0.0f, 1.0f, 0.0f)));

        const float sprint = window.isKeyDown(vela::core::Key::LeftShift) ? 5.0f : 1.0f;
        const float step = m_speed * sprint * deltaTime;

        if (window.isKeyDown(vela::core::Key::W))
            position = position + forward * step;

        if (window.isKeyDown(vela::core::Key::S))
            position = position - forward * step;

        if (window.isKeyDown(vela::core::Key::A))
            position = position - right * step;

        if (window.isKeyDown(vela::core::Key::D))
            position = position + right * step;

        if (window.isKeyDown(vela::core::Key::E))
            position.y += step;

        if (window.isKeyDown(vela::core::Key::Q))
            position.y -= step;

        m_camera.setPosition(position);

        if (!window.isMouseButtonDown(vela::core::MouseButton::Right))
        {
            m_firstMouse = true;
            return;
        }

        double x = 0.0;
        double y = 0.0;

        window.getCursorPosition(x, y);

        if (m_firstMouse)
        {
            m_lastX = x;
            m_lastY = y;
            m_firstMouse = false;

            return;
        }

        const float offsetX = static_cast<float>(x - m_lastX);
        const float offsetY = static_cast<float>(y - m_lastY);

        m_lastX = x;
        m_lastY = y;

        m_camera.setYaw(m_camera.getYaw() + offsetX * m_sensitivity);
        m_camera.setPitch(std::clamp(m_camera.getPitch() - offsetY * m_sensitivity, -89.0f, 89.0f));
        m_camera.updateCameraVectors();
    }

    Camera& FlyCamera::camera()
    {
        return m_camera;
    }

    float FlyCamera::speed() const
    {
        return m_speed;
    }

    void FlyCamera::setSpeed(float speed)
    {
        m_speed = speed;
    }
} //namespace little
