#ifndef VELA_MATH_MATH_HPP
#define VELA_MATH_MATH_HPP

#include "Vector.hpp"
#include "Matrix.hpp"
#include "Quaternion.hpp"

#include <cmath>

namespace vela::math
{
    inline constexpr float pi = 3.14159265358979323846f;

    template<typename T>
    [[nodiscard]]
    constexpr T dot(const Vector2<T>& a, const Vector2<T>& b) noexcept
    {
        return a.x * b.x + a.y * b.y;
    }

    template<typename T>
    [[nodiscard]]
    constexpr T dot(const Vector3<T>& a, const Vector3<T>& b) noexcept
    {
        return
            a.x * b.x +
            a.y * b.y +
            a.z * b.z;
    }

    template<typename T>
    [[nodiscard]]
    constexpr T dot(const Vector4<T>& a, const Vector4<T>& b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    template<typename VectorT>
    [[nodiscard]]
    auto lengthSquared(const VectorT& vector) noexcept
    {
        return dot(vector, vector);
    }

    template<typename VectorT>
    [[nodiscard]]
    auto length(const VectorT& vector) noexcept
    {
        return std::sqrt(lengthSquared(vector));
    }

    template<typename VectorT>
    [[nodiscard]]
    VectorT normalize(const VectorT& vector) noexcept
    {
        const auto len = length(vector);

        if (len == decltype(len){0})
            return VectorT{};

        return vector / len;
    }

    template<typename T>
    [[nodiscard]]
    constexpr Vector2<T> min(const Vector2<T>& a, const Vector2<T>& b) noexcept
    {
        return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y};
    }

    template<typename T>
    [[nodiscard]]
    constexpr Vector3<T> min(const Vector3<T>& a, const Vector3<T>& b) noexcept
    {
        return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z};
    }

    template<typename T>
    [[nodiscard]]
    constexpr Vector2<T> max(const Vector2<T>& a, const Vector2<T>& b) noexcept
    {
        return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y};
    }

    template<typename T>
    [[nodiscard]]
    constexpr Vector3<T> max(const Vector3<T>& a, const Vector3<T>& b) noexcept
    {
        return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z};
    }

    template<typename T>
    [[nodiscard]]
    Vector3<T> abs(const Vector3<T>& vector) noexcept
    {
        return {std::abs(vector.x), std::abs(vector.y), std::abs(vector.z)};
    }

    template<typename T>
    [[nodiscard]]
    constexpr T lerp(T a, T b, T t) noexcept
    {
        return a + (b - a) * t;
    }

    template<typename VectorT, typename T>
    [[nodiscard]]
    constexpr VectorT lerp(const VectorT& a, const VectorT& b, T t) noexcept
    {
        return a + (b - a) * t;
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

    template<typename T>
    constexpr Vector3<T> radians(const Vector3<T>& degrees) noexcept
    {
        return degrees * static_cast<T>(0.01745329251994329576923690768489);
    }

    template<typename T>
    constexpr Vector3<T> degrees(const Vector3<T>& radians) noexcept
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

    inline Mat4 scale(const Mat4& matrix, const Vector3f& factors) noexcept
    {
        Mat4 result = matrix;

        result.setColumn(0, matrix.getColumn(0) * factors.x);
        result.setColumn(1, matrix.getColumn(1) * factors.y);
        result.setColumn(2, matrix.getColumn(2) * factors.z);

        return result;
    }

    inline Mat4 transpose(const Mat4& matrix) noexcept
    {
        Mat4 result;

        for (std::size_t column = 0; column < 4; ++column)
            for (std::size_t row = 0; row < 4; ++row)
                result[column][row] = matrix[row][column];

        return result;
    }

    // General inverse. A singular matrix gives back a matrix of infinities/NaNs, like glm
    inline Mat4 inverse(const Mat4& matrix) noexcept
    {
        const float* m = matrix.data();
        float inv[16];

        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

        const float determinant = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        const float inverseDeterminant = 1.0f / determinant;

        Mat4 result;
        float* out = result.data();

        for (std::size_t i = 0; i < 16; ++i)
            out[i] = inv[i] * inverseDeterminant;

        return result;
    }

    inline Vector3f transformPoint(const Mat4& matrix, const Vector3f& point) noexcept
    {
        return (matrix * Vector4f(point, 1.0f)).xyz();
    }

    inline Vector3f transformDirection(const Mat4& matrix, const Vector3f& direction) noexcept
    {
        return (matrix * Vector4f(direction, 0.0f)).xyz();
    }

    // ---- Quaternions ----

    [[nodiscard]]
    inline float dot(const Quaternion& a, const Quaternion& b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    [[nodiscard]]
    inline float length(const Quaternion& quaternion) noexcept
    {
        return std::sqrt(dot(quaternion, quaternion));
    }

    [[nodiscard]]
    inline Quaternion normalize(const Quaternion& quaternion) noexcept
    {
        const float len = length(quaternion);

        if (len <= 0.0f)
            return Quaternion::identity();

        return quaternion * (1.0f / len);
    }

    [[nodiscard]]
    constexpr Quaternion conjugate(const Quaternion& quaternion) noexcept
    {
        return {-quaternion.x, -quaternion.y, -quaternion.z, quaternion.w};
    }

    [[nodiscard]]
    inline Quaternion inverse(const Quaternion& quaternion) noexcept
    {
        const float lengthSq = dot(quaternion, quaternion);

        if (lengthSq <= 0.0f)
            return Quaternion::identity();

        return conjugate(quaternion) * (1.0f / lengthSq);
    }

    // angle in radians
    [[nodiscard]]
    inline Quaternion angleAxis(float angle, const Vector3f& axis) noexcept
    {
        const Vector3f unitAxis = normalize(axis);
        const float s = std::sin(angle * 0.5f);

        return {unitAxis.x * s, unitAxis.y * s, unitAxis.z * s, std::cos(angle * 0.5f)};
    }

    // Euler angles in radians (x pitch, y yaw, z roll), applied as glm::quat(vec3) does
    [[nodiscard]]
    inline Quaternion fromEuler(const Vector3f& euler) noexcept
    {
        const float cx = std::cos(euler.x * 0.5f);
        const float cy = std::cos(euler.y * 0.5f);
        const float cz = std::cos(euler.z * 0.5f);
        const float sx = std::sin(euler.x * 0.5f);
        const float sy = std::sin(euler.y * 0.5f);
        const float sz = std::sin(euler.z * 0.5f);

        return {
            sx * cy * cz - cx * sy * sz,
            cx * sy * cz + sx * cy * sz,
            cx * cy * sz - sx * sy * cz,
            cx * cy * cz + sx * sy * sz};
    }

    // Inverse of fromEuler, radians. Matches glm::eulerAngles
    [[nodiscard]]
    inline Vector3f toEuler(const Quaternion& q) noexcept
    {
        constexpr float epsilon = 1e-6f;

        Vector3f euler;

        const float pitchY = 2.0f * (q.y * q.z + q.w * q.x);
        const float pitchX = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;

        if (std::abs(pitchX) < epsilon && std::abs(pitchY) < epsilon)
            euler.x = 2.0f * std::atan2(q.x, q.w);
        else
            euler.x = std::atan2(pitchY, pitchX);

        euler.y = std::asin(clamp(-2.0f * (q.x * q.z - q.w * q.y), -1.0f, 1.0f));

        const float rollY = 2.0f * (q.x * q.y + q.w * q.z);
        const float rollX = q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z;

        if (std::abs(rollX) < epsilon && std::abs(rollY) < epsilon)
            euler.z = 0.0f;
        else
            euler.z = std::atan2(rollY, rollX);

        return euler;
    }

    [[nodiscard]]
    inline Mat4 toMat4(const Quaternion& q) noexcept
    {
        const float xx = q.x * q.x;
        const float yy = q.y * q.y;
        const float zz = q.z * q.z;
        const float xy = q.x * q.y;
        const float xz = q.x * q.z;
        const float yz = q.y * q.z;
        const float wx = q.w * q.x;
        const float wy = q.w * q.y;
        const float wz = q.w * q.z;

        Mat4 result{1.0f};

        result[0][0] = 1.0f - 2.0f * (yy + zz);
        result[0][1] = 2.0f * (xy + wz);
        result[0][2] = 2.0f * (xz - wy);

        result[1][0] = 2.0f * (xy - wz);
        result[1][1] = 1.0f - 2.0f * (xx + zz);
        result[1][2] = 2.0f * (yz + wx);

        result[2][0] = 2.0f * (xz + wy);
        result[2][1] = 2.0f * (yz - wx);
        result[2][2] = 1.0f - 2.0f * (xx + yy);

        return result;
    }

    // Rotation from the upper 3x3, which must be orthonormal
    [[nodiscard]]
    inline Quaternion fromRotationMatrix(const Mat4& m) noexcept
    {
        const float trace = m[0][0] + m[1][1] + m[2][2];

        Quaternion q;

        if (trace > 0.0f)
        {
            const float s = std::sqrt(trace + 1.0f) * 2.0f;
            q.w = 0.25f * s;
            q.x = (m[1][2] - m[2][1]) / s;
            q.y = (m[2][0] - m[0][2]) / s;
            q.z = (m[0][1] - m[1][0]) / s;
        }
        else if (m[0][0] > m[1][1] && m[0][0] > m[2][2])
        {
            const float s = std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
            q.w = (m[1][2] - m[2][1]) / s;
            q.x = 0.25f * s;
            q.y = (m[1][0] + m[0][1]) / s;
            q.z = (m[2][0] + m[0][2]) / s;
        }
        else if (m[1][1] > m[2][2])
        {
            const float s = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
            q.w = (m[2][0] - m[0][2]) / s;
            q.x = (m[1][0] + m[0][1]) / s;
            q.y = 0.25f * s;
            q.z = (m[2][1] + m[1][2]) / s;
        }
        else
        {
            const float s = std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
            q.w = (m[0][1] - m[1][0]) / s;
            q.x = (m[2][0] + m[0][2]) / s;
            q.y = (m[2][1] + m[1][2]) / s;
            q.z = 0.25f * s;
        }

        return normalize(q);
    }

    [[nodiscard]]
    inline Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) noexcept
    {
        Quaternion end = b;
        float cosTheta = dot(a, b);

        // Take the short way round
        if (cosTheta < 0.0f)
        {
            end = -b;
            cosTheta = -cosTheta;
        }

        // Nearly parallel: sin(theta) is too small to divide by
        if (cosTheta > 1.0f - 1e-6f)
            return normalize(a * (1.0f - t) + end * t);

        const float theta = std::acos(cosTheta);
        const float sinTheta = std::sin(theta);

        return a * (std::sin((1.0f - t) * theta) / sinTheta) + end * (std::sin(t * theta) / sinTheta);
    }

    // Translation * rotation * scale
    [[nodiscard]]
    inline Mat4 compose(const Vector3f& translation, const Quaternion& rotation, const Vector3f& scaleFactors) noexcept
    {
        Mat4 result = scale(toMat4(rotation), scaleFactors);
        result[3][0] = translation.x;
        result[3][1] = translation.y;
        result[3][2] = translation.z;

        return result;
    }

    // Splits an affine TRS matrix. Skew and projection are dropped. False if a scale axis is zero
    inline bool decompose(const Mat4& matrix, Vector3f& outTranslation, Quaternion& outRotation, Vector3f& outScale) noexcept
    {
        constexpr float epsilon = 1e-8f;

        Vector3f axes[3] = {matrix.getColumn(0).xyz(), matrix.getColumn(1).xyz(), matrix.getColumn(2).xyz()};
        Vector3f scaleFactors{length(axes[0]), length(axes[1]), length(axes[2])};

        if (scaleFactors.x < epsilon || scaleFactors.y < epsilon || scaleFactors.z < epsilon)
            return false;

        for (int axis = 0; axis < 3; ++axis)
            axes[axis] /= scaleFactors[axis];

        // A mirrored basis: fold the flip into the scale so the rotation stays proper
        if (dot(axes[0], cross(axes[1], axes[2])) < 0.0f)
        {
            scaleFactors = -scaleFactors;

            for (Vector3f& axis : axes)
                axis = -axis;
        }

        Mat4 rotation{1.0f};
        rotation.setColumn(0, Vector4f(axes[0], 0.0f));
        rotation.setColumn(1, Vector4f(axes[1], 0.0f));
        rotation.setColumn(2, Vector4f(axes[2], 0.0f));

        outTranslation = matrix.getColumn(3).xyz();
        outRotation = fromRotationMatrix(rotation);
        outScale = scaleFactors;

        return true;
    }
} //namespace vela::math

#endif //VELA_MATH_MATH_HPP