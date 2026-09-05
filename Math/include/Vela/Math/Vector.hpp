#ifndef VELA_MATH_VECTOR_HPP
#define VELA_MATH_VECTOR_HPP

namespace vela::math
{   
    template<typename T>
    class Vector3
    {
    public:
        T x;
        T y;
        T z;

        Vector3() = default;

        Vector3(T x, T y, T z) : x(x), y(y), z(z) {}

        template<typename U>
        Vector3(const Vector3<U>& other) : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)) {}  
        
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
    };

    template<typename T>
    class Vector2
    {
    public:
        T x;
        T y;

        Vector2() = default;

        Vector2(T x, T y) : x(x), y(y) {}

        template<typename U>
        Vector2(const Vector2<U>& other) : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}   
    };

    using Vector3f = Vector3<float>;
    using Vector3i = Vector3<int>;

    using Vector2f = Vector2<float>;
    using Vector2i = Vector2<int>;
} //namespace vela::math

#endif //VELA_MATH_VECTOR_HPP