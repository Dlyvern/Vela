#ifndef LITTLE_ENGINE_FLY_CAMERA_HPP
#define LITTLE_ENGINE_FLY_CAMERA_HPP

#include "Vela/Core/Window.hpp"
#include "LittleEngine/Camera.hpp"

namespace little
{
    class FlyCamera
    {
    public:
        void frame(const vela::math::Vector3f& center, float diagonal);
        void update(vela::core::Window& window, float deltaTime, bool acceptInput);

        [[nodiscard]] Camera& camera();
        [[nodiscard]] float speed() const;

        void setSpeed(float speed);

    private:
        Camera m_camera;

        float m_speed{1.0f};
        float m_sensitivity{0.1f};

        bool m_firstMouse{true};
        double m_lastX{0.0};
        double m_lastY{0.0};
    };
} //namespace little

#endif //LITTLE_ENGINE_FLY_CAMERA_HPP
