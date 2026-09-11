#ifndef LITTLE_ENGINE_CAMERA_HPP
#define LITTLE_ENGINE_CAMERA_HPP

#include "Vela/Math/Matrix.hpp"
#include "Vela/Math/Vector.hpp"

#include <cstdint>

namespace little
{
    class Camera
    {
    public:
        enum class ProjectionMode : uint8_t
        {
            Perspective = 0,
            Orthographic = 1
        };

        Camera();

        [[nodiscard]] const vela::math::Vector3f& getPosition() const;
        [[nodiscard]] const vela::math::Vector3f& getForward() const;
        [[nodiscard]] const vela::math::Vector3f& getUp() const;
        [[nodiscard]] vela::math::Mat4 getViewMatrix() const;
        [[nodiscard]] float getPitch() const;
        [[nodiscard]] float getYaw() const;
        [[nodiscard]] vela::math::Mat4 getProjectionMatrix() const;

        [[nodiscard]] float getFOV() const;
        [[nodiscard]] float getNear() const;
        [[nodiscard]] float getFar() const;
        [[nodiscard]] float getAspect() const;
        [[nodiscard]] ProjectionMode getProjectionMode() const;
        [[nodiscard]] float getOrthographicSize() const;

        void setYaw(float yaw);
        void setPitch(float pitch);
        void setPosition(const vela::math::Vector3f &position);
        void setFOV(float fov);
        void setAspect(float aspect);
        void setNear(float nearPlane);
        void setFar(float farPlane);
        void setProjectionMode(ProjectionMode mode);
        void setOrthographicSize(float size);

        void updateCameraVectors();

        ~Camera() = default;

    private:
        vela::math::Vector3f m_position{2.0f, 2.0f, 2.0f};
        vela::math::Vector3f m_up{0.0f, 1.0f, 0.0f};
        vela::math::Vector3f m_right{0.0f, 1.0f, 0.0f};
        vela::math::Vector3f m_forward{0.0f, 0.0f, -1.0f};

        float m_yaw{-90.0f};
        float m_pitch{0.0f};

        float m_fov{60.0f};
        float m_aspect{16.0f / 9.0f};
        float m_near{0.1f};
        float m_far{1000.0f};
        float m_orthographicSize{10.0f};
        ProjectionMode m_projectionMode{ProjectionMode::Perspective};
    };
} //namespace little

#endif //LITTLE_ENGINE_CAMERA_HPP