#ifndef VELA_MATH_VECTOR_HPP
#define VELA_MATH_VECTOR_HPP

#include <cstddef>

namespace vela::math
{
    template<typename T>
    class Vector2
    {
    public:
        T x{};
        T y{};

        constexpr Vector2() = default;

        constexpr Vector2(T x, T y) : x(x), y(y) {}

        constexpr explicit Vector2(T scalar) : x(scalar), y(scalar) {}

        template<typename U>
        constexpr Vector2(const Vector2<U>& other) : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}

        constexpr T& operator[](std::size_t index) noexcept
        {
            return index == 0 ? x : y;
        }

        constexpr const T& operator[](std::size_t index) const noexcept
        {
            return index == 0 ? x : y;
        }

        constexpr Vector2 operator+(const Vector2& other) const noexcept
        {
            return {x + other.x, y + other.y};
        }

        constexpr Vector2 operator-(const Vector2& other) const noexcept
        {
            return {x - other.x, y - other.y};
        }

        constexpr Vector2 operator-() const noexcept
        {
            return {-x, -y};
        }

        constexpr Vector2 operator*(const Vector2& other) const noexcept
        {
            return {x * other.x, y * other.y};
        }

        constexpr Vector2 operator*(T scalar) const noexcept
        {
            return {x * scalar, y * scalar};
        }

        constexpr Vector2 operator/(T scalar) const noexcept
        {
            return {x / scalar, y / scalar};
        }

        constexpr Vector2& operator+=(const Vector2& other) noexcept
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        constexpr Vector2& operator-=(const Vector2& other) noexcept
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        constexpr Vector2& operator*=(T scalar) noexcept
        {
            x *= scalar;
            y *= scalar;
            return *this;
        }

        constexpr Vector2& operator/=(T scalar) noexcept
        {
            x /= scalar;
            y /= scalar;
            return *this;
        }

        constexpr bool operator==(const Vector2& other) const noexcept = default;
    };

    template<typename T>
    class Vector3
    {
    public:
        T x{};
        T y{};
        T z{};

        constexpr Vector3() = default;

        constexpr Vector3(T x, T y, T z) : x(x), y(y), z(z) {}

        constexpr explicit Vector3(T scalar) : x(scalar), y(scalar), z(scalar) {}

        constexpr Vector3(const Vector2<T>& xy, T z) : x(xy.x), y(xy.y), z(z) {}

        template<typename U>
        constexpr Vector3(const Vector3<U>& other) : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)) {}

        constexpr T& operator[](std::size_t index) noexcept
        {
            return index == 0 ? x : (index == 1 ? y : z);
        }

        constexpr const T& operator[](std::size_t index) const noexcept
        {
            return index == 0 ? x : (index == 1 ? y : z);
        }

        constexpr T* data() noexcept
        {
            return &x;
        }

        constexpr const T* data() const noexcept
        {
            return &x;
        }

        constexpr Vector3 operator+(const Vector3& other) const noexcept
        {
            return {x + other.x, y + other.y, z + other.z};
        }

        constexpr Vector3 operator-(const Vector3& other) const noexcept
        {
            return {x - other.x, y - other.y, z - other.z};
        }

        constexpr Vector3 operator-() const noexcept
        {
            return {-x, -y, -z};
        }

        // Component-wise
        constexpr Vector3 operator*(const Vector3& other) const noexcept
        {
            return {x * other.x, y * other.y, z * other.z};
        }

        constexpr Vector3 operator/(const Vector3& other) const noexcept
        {
            return {x / other.x, y / other.y, z / other.z};
        }

        constexpr Vector3 operator*(T scalar) const noexcept
        {
            return {x * scalar, y * scalar, z * scalar};
        }

        constexpr Vector3 operator/(T scalar) const noexcept
        {
            return {x / scalar, y / scalar, z / scalar};
        }

        constexpr Vector3& operator+=(const Vector3& other) noexcept
        {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        constexpr Vector3& operator-=(const Vector3& other) noexcept
        {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }

        constexpr Vector3& operator*=(T scalar) noexcept
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        constexpr Vector3& operator/=(T scalar) noexcept
        {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            return *this;
        }

        constexpr bool operator==(const Vector3& other) const noexcept = default;
    };

    template<typename T>
    class Vector4
    {
    public:
        T x{};
        T y{};
        T z{};
        T w{};

        constexpr Vector4() = default;

        constexpr Vector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

        constexpr explicit Vector4(T scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}

        constexpr Vector4(const Vector3<T>& xyz, T w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

        template<typename U>
        constexpr Vector4(const Vector4<U>& other)
            : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)), w(static_cast<T>(other.w)) {}

        [[nodiscard]]
        constexpr Vector3<T> xyz() const noexcept
        {
            return {x, y, z};
        }

        constexpr T& operator[](std::size_t index) noexcept
        {
            return index == 0 ? x : (index == 1 ? y : (index == 2 ? z : w));
        }

        constexpr const T& operator[](std::size_t index) const noexcept
        {
            return index == 0 ? x : (index == 1 ? y : (index == 2 ? z : w));
        }

        constexpr T* data() noexcept
        {
            return &x;
        }

        constexpr const T* data() const noexcept
        {
            return &x;
        }

        constexpr Vector4 operator+(const Vector4& other) const noexcept
        {
            return {x + other.x, y + other.y, z + other.z, w + other.w};
        }

        constexpr Vector4 operator-(const Vector4& other) const noexcept
        {
            return {x - other.x, y - other.y, z - other.z, w - other.w};
        }

        constexpr Vector4 operator-() const noexcept
        {
            return {-x, -y, -z, -w};
        }

        constexpr Vector4 operator*(const Vector4& other) const noexcept
        {
            return {x * other.x, y * other.y, z * other.z, w * other.w};
        }

        constexpr Vector4 operator*(T scalar) const noexcept
        {
            return {x * scalar, y * scalar, z * scalar, w * scalar};
        }

        constexpr Vector4 operator/(T scalar) const noexcept
        {
            return {x / scalar, y / scalar, z / scalar, w / scalar};
        }

        constexpr Vector4& operator+=(const Vector4& other) noexcept
        {
            x += other.x;
            y += other.y;
            z += other.z;
            w += other.w;
            return *this;
        }

        constexpr Vector4& operator*=(T scalar) noexcept
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            w *= scalar;
            return *this;
        }

        constexpr Vector4& operator/=(T scalar) noexcept
        {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            w /= scalar;
            return *this;
        }

        constexpr bool operator==(const Vector4& other) const noexcept = default;
    };

    template<typename T>
    constexpr Vector2<T> operator*(T scalar, const Vector2<T>& vector) noexcept
    {
        return vector * scalar;
    }

    template<typename T>
    constexpr Vector3<T> operator*(T scalar, const Vector3<T>& vector) noexcept
    {
        return vector * scalar;
    }

    template<typename T>
    constexpr Vector4<T> operator*(T scalar, const Vector4<T>& vector) noexcept
    {
        return vector * scalar;
    }

    using Vector4f = Vector4<float>;
    using Vector4i = Vector4<int>;

    using Vector3f = Vector3<float>;
    using Vector3i = Vector3<int>;

    using Vector2f = Vector2<float>;
    using Vector2i = Vector2<int>;
} //namespace vela::math

#endif //VELA_MATH_VECTOR_HPP
