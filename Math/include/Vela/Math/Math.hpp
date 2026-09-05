#ifndef VELA_MATH_MATH_HPP
#define VELA_MATH_MATH_HPP

#include "Vector.hpp"
#include "Matrix.hpp"

#include <cmath>

namespace vela::math
{
    template<typename T>
    [[nodiscard]]
    T lengthSquared(const Vector3<T>& vector) noexcept
    {
        return vector.x * vector.x + 
            vector.y * vector.y +
            vector.z * vector.z;
    }

    template<typename T>
    [[nodiscard]]
    T length(const Vector3<T>& vector) noexcept
    {
        return std::sqrt(lengthSquared(vector));
    }

    template<typename T>
    [[nodiscard]]
    Vector3<T> normalize(const Vector3<T>& vector) noexcept
    {
        const T len = length(vector);

        if (len == T{0})
            return Vector3<T>{};

        return Vector3<T>{vector.x / len, vector.y / len, vector.z / len};
    }

    template<typename T>
    [[nodiscard]]
    T dot(const Vector3<T>& a, const Vector3<T>& b) noexcept
    {
        return
            a.x * b.x +
            a.y * b.y +
            a.z * b.z;
    }

    template<typename T>
    [[nodiscard]]
    Vector3<T> cross(const Vector3<T>& a, const Vector3<T>& b) noexcept
    {
        return Vector3<T>{
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    template<typename T>
    constexpr T radians(T degrees) noexcept
    {
        return degrees * static_cast<T>(0.01745329251994329576923690768489);
    }

    template<typename T>
    constexpr T degrees(T radians) noexcept
    {
        return radians * static_cast<T>(57.295779513082320876798154814105);
    }

    inline math::Mat4 ortho(float left, float right, float bottom,  float top, float nearPlane, float farPlane) noexcept
    {
        Mat4 result{1.0f};

        result[0][0] = 2.0f / (right - left);
        result[1][1] = 2.0f / (top - bottom);
        result[2][2] = 1.0f / (nearPlane - farPlane);

        result[3][0] = -(right + left) / (right - left);
        result[3][1] = -(top + bottom) / (top - bottom);
        result[3][2] = nearPlane / (nearPlane - farPlane);

        return result;
    }

    template<typename T>
    constexpr T clamp(T value, T minValue, T maxValue) noexcept
    {
        if (value < minValue)
            return minValue;

        if (value > maxValue)
            return maxValue;

        return value;
    }

    inline Mat4 perspective(float fovY, float aspect, float nearPlane, float farPlane) noexcept
    {
        const float tanHalfFov = std::tan(fovY * 0.5f);

        Mat4 result{0.0f};

        result[0][0] = 1.0f / (aspect * tanHalfFov);
        result[1][1] = 1.0f / tanHalfFov;

        result[2][2] = farPlane / (nearPlane - farPlane);
        result[2][3] = -1.0f;

        result[3][2] =
            (farPlane * nearPlane) / (nearPlane - farPlane);

        return result;
    }

    inline Mat4 lookAt(const Vector3f& eye, const Vector3f& center, const Vector3f& up) noexcept
    {
        const Vector3f forward = normalize(center - eye);
        const Vector3f right = normalize(cross(forward, up));
        const Vector3f cameraUp = cross(right, forward);

        Mat4 result{1.0f};

        result[0][0] = right.x;
        result[1][0] = right.y;
        result[2][0] = right.z;

        result[0][1] = cameraUp.x;
        result[1][1] = cameraUp.y;
        result[2][1] = cameraUp.z;

        result[0][2] = -forward.x;
        result[1][2] = -forward.y;
        result[2][2] = -forward.z;

        result[3][0] = -dot(right, eye);
        result[3][1] = -dot(cameraUp, eye);
        result[3][2] = dot(forward, eye);

        return result;
    }

    inline Mat4 translate(const Mat4& matrix, const Vector3f& translation) noexcept
    {
        Mat4 result = matrix;

        result[3][0] =
            matrix[0][0] * translation.x +
            matrix[1][0] * translation.y +
            matrix[2][0] * translation.z +
            matrix[3][0];

        result[3][1] =
            matrix[0][1] * translation.x +
            matrix[1][1] * translation.y +
            matrix[2][1] * translation.z +
            matrix[3][1];

        result[3][2] =
            matrix[0][2] * translation.x +
            matrix[1][2] * translation.y +
            matrix[2][2] * translation.z +
            matrix[3][2];

        return result;
    }


    inline Mat4 rotate(const Mat4& matrix, float angle, const Vector3f& axis) noexcept
    {
        const float c = std::cos(angle);
        const float s = std::sin(angle);

        const Vector3f a = normalize(axis);

        const float x = a.x;
        const float y = a.y;
        const float z = a.z;

        const float oneMinusC = 1.0f - c;

        Mat4 rotation{1.0f};

        rotation[0][0] = c + x * x * oneMinusC;
        rotation[0][1] = x * y * oneMinusC + z * s;
        rotation[0][2] = x * z * oneMinusC - y * s;

        rotation[1][0] = y * x * oneMinusC - z * s;
        rotation[1][1] = c + y * y * oneMinusC;
        rotation[1][2] = y * z * oneMinusC + x * s;

        rotation[2][0] = z * x * oneMinusC + y * s;
        rotation[2][1] = z * y * oneMinusC - x * s;
        rotation[2][2] = c + z * z * oneMinusC;

        return matrix * rotation;
    }
} //namespace vela::math

#endif //VELA_MATH_MATH_HPP