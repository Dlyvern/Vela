#ifndef VELA_MATH_MATRIX_HPP
#define VELA_MATH_MATRIX_HPP

#include "Vector.hpp"

#include <cstddef>

namespace vela::math
{
    // Column-major, m[column * 4 + row]. Same memory layout as GLSL mat4
    class Mat4
    {
    public:
        constexpr Mat4() noexcept
            : m{}
        {}

        constexpr explicit Mat4(float diagonal) noexcept
            : m{}
        {
            m[0] = diagonal;
            m[5] = diagonal;
            m[10] = diagonal;
            m[15] = diagonal;
        }

        constexpr Mat4(const Vector4<float>& column0, const Vector4<float>& column1,
                       const Vector4<float>& column2, const Vector4<float>& column3) noexcept
            : m{}
        {
            setColumn(0, column0);
            setColumn(1, column1);
            setColumn(2, column2);
            setColumn(3, column3);
        }

        constexpr float* data() noexcept
        {
            return m;
        }

        constexpr const float* data() const noexcept
        {
            return m;
        }

        [[nodiscard]]
        constexpr Vector4<float> getColumn(std::size_t column) const noexcept
        {
            return {m[column * 4 + 0], m[column * 4 + 1], m[column * 4 + 2], m[column * 4 + 3]};
        }

        constexpr void setColumn(std::size_t column, const Vector4<float>& value) noexcept
        {
            m[column * 4 + 0] = value.x;
            m[column * 4 + 1] = value.y;
            m[column * 4 + 2] = value.z;
            m[column * 4 + 3] = value.w;
        }

        constexpr Mat4 operator*(const Mat4& other) const noexcept
        {
            Mat4 result;

            for (std::size_t column = 0; column < 4; ++column)
            {
                for (std::size_t row = 0; row < 4; ++row)
                {
                    result[column][row] =
                        (*this)[0][row] * other[column][0] +
                        (*this)[1][row] * other[column][1] +
                        (*this)[2][row] * other[column][2] +
                        (*this)[3][row] * other[column][3];
                }
            }

            return result;
        }

        constexpr Vector4<float> operator*(const Vector4<float>& vector) const noexcept
        {
            Vector4<float> result;

            for (std::size_t row = 0; row < 4; ++row)
            {
                result[row] =
                    (*this)[0][row] * vector.x +
                    (*this)[1][row] * vector.y +
                    (*this)[2][row] * vector.z +
                    (*this)[3][row] * vector.w;
            }

            return result;
        }

        constexpr Mat4& operator*=(const Mat4& other) noexcept
        {
            *this = *this * other;
            return *this;
        }

        [[nodiscard]]
        constexpr bool operator==(const Mat4& other) const noexcept
        {
            for (std::size_t i = 0; i < 16; ++i)
            {
                if (m[i] != other.m[i])
                    return false;
            }

            return true;
        }

        [[nodiscard]]
        constexpr bool operator!=(const Mat4& other) const noexcept
        {
            return !(*this == other);
        }

        constexpr float* operator[](std::size_t column) noexcept
        {
            return &m[column * 4];
        }

        constexpr const float* operator[](std::size_t column) const noexcept
        {
            return &m[column * 4];
        }

    private:
        float m[16];
    };

} //namespace vela::math

#endif //VELA_MATH_MATRIX_HPP
