#ifndef VELA_MATH_QUATERNION_HPP
#define VELA_MATH_QUATERNION_HPP

#include "Vector.hpp"

namespace vela::math
{
    // Unit quaternions for rotations. Default constructed is the identity
    class Quaternion
    {
    public:
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        float w{1.0f};

        constexpr Quaternion() = default;

        // Note the order: x, y, z, then w
        constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

        [[nodiscard]]
        static constexpr Quaternion identity() noexcept
        {
            return {};
        }

        constexpr Quaternion operator*(const Quaternion& other) const noexcept
        {
            return {
                w * other.x + x * other.w + y * other.z - z * other.y,
                w * other.y + y * other.w + z * other.x - x * other.z,
                w * other.z + z * other.w + x * other.y - y * other.x,
                w * other.w - x * other.x - y * other.y - z * other.z};
        }

        // Rotates a vector
        constexpr Vector3<float> operator*(const Vector3<float>& vector) const noexcept
        {
            const Vector3<float> axis{x, y, z};

            const Vector3<float> uv{
                axis.y * vector.z - axis.z * vector.y,
                axis.z * vector.x - axis.x * vector.z,
                axis.x * vector.y - axis.y * vector.x};

            const Vector3<float> uuv{
                axis.y * uv.z - axis.z * uv.y,
                axis.z * uv.x - axis.x * uv.z,
                axis.x * uv.y - axis.y * uv.x};

            return vector + (uv * w + uuv) * 2.0f;
        }

        constexpr Quaternion operator*(float scalar) const noexcept
        {
            return {x * scalar, y * scalar, z * scalar, w * scalar};
        }

        constexpr Quaternion operator+(const Quaternion& other) const noexcept
        {
            return {x + other.x, y + other.y, z + other.z, w + other.w};
        }

        constexpr Quaternion operator-() const noexcept
        {
            return {-x, -y, -z, -w};
        }

        constexpr Quaternion& operator*=(const Quaternion& other) noexcept
        {
            *this = *this * other;
            return *this;
        }

        constexpr bool operator==(const Quaternion& other) const noexcept = default;
    };
} //namespace vela::math

#endif //VELA_MATH_QUATERNION_HPP
