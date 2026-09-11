#include "LittleEngine/Camera.hpp"

#include <algorithm>

#include "Vela/Math/Math.hpp"

namespace little
{
    Camera::Camera()
    {
        updateCameraVectors();
    }

    const vela::math::Vector3f& Camera::getPosition() const
    {
        return m_position;
    }

    const vela::math::Vector3f& Camera::getForward() const
    {
        return m_forward;
    }

    const vela::math::Vector3f& Camera::getUp() const
    {
        return m_up;
    }

    vela::math::Mat4 Camera::getViewMatrix() const
    {
        return vela::math::lookAt(m_position, m_position + m_forward, m_up);
    }

    float Camera::getPitch() const
    {
        return m_pitch;
    }

    float Camera::getYaw() const
    {
        return m_yaw;
    }

    float Camera::getFOV() const
    {
        return m_fov;
    }

    float Camera::getNear() const
    {
        return m_near;
    };

    float Camera::getFar() const
    {
        return m_far;
    };

    float Camera::getAspect() const
    {
        return m_aspect;
    }

    Camera::ProjectionMode Camera::getProjectionMode() const
    {
        return m_projectionMode;
    }

    float Camera::getOrthographicSize() const
    {
        return m_orthographicSize;
    }

    vela::math::Mat4 Camera::getProjectionMatrix() const
    {
        vela::math::Mat4 proj{1.0f};

        if (m_projectionMode == ProjectionMode::Orthographic)
        {
            const float halfHeight = std::max(m_orthographicSize * 0.5f, 0.001f);
            const float halfWidth = std::max(halfHeight * m_aspect, 0.001f);
            proj = vela::math::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, m_near, m_far);
        }
        else
            proj = vela::math::perspective(vela::math::radians(m_fov), m_aspect, m_near, m_far);

        proj[1][1] *= -1; //For vulkan shit

        return proj;
    }

    void Camera::setYaw(float yaw)
    {
        m_yaw = yaw;
        updateCameraVectors();
    }

    void Camera::setFOV(float fov)
    {
        m_fov = vela::math::clamp(fov, 1.0f, 179.0f);
    }

    void Camera::setAspect(float aspect)
    {
        m_aspect = std::max(aspect, 0.001f);
    }

    void Camera::setNear(float nearPlane)
    {
        m_near = std::max(nearPlane, 0.001f);
        if (m_far <= m_near)
            m_far = m_near + 0.001f;
    }

    void Camera::setFar(float farPlane)
    {
        m_far = std::max(farPlane, m_near + 0.001f);
    }

    void Camera::setProjectionMode(ProjectionMode mode)
    {
        m_projectionMode = mode;
    }

    void Camera::setOrthographicSize(float size)
    {
        m_orthographicSize = std::max(size, 0.001f);
    }

    void Camera::setPosition(const vela::math::Vector3f &position)
    {
        m_position = position;
    }

    void Camera::setPitch(float pitch)
    {
        m_pitch = vela::math::clamp(pitch, -89.0f, 89.0f);
        updateCameraVectors();
    }

    void Camera::updateCameraVectors()
    {
        vela::math::Vector3f forward;
        forward.x = cos(vela::math::radians(m_yaw)) * cos(vela::math::radians(m_pitch));
        forward.y = sin(vela::math::radians(m_pitch));
        forward.z = sin(vela::math::radians(m_yaw)) * cos(vela::math::radians(m_pitch));
        
        m_forward = vela::math::normalize(forward);
        m_right = vela::math::normalize(vela::math::cross(m_forward, vela::math::Vector3f(0.0f, 1.0f, 0.0f)));
        m_up = vela::math::normalize(vela::math::cross(m_right, m_forward));
    }
} //namespace little