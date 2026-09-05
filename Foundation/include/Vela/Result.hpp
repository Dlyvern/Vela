#ifndef VELA_RESULT_HPP
#define VELA_RESULT_HPP

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace vela
{
    enum class ErrorCode : uint32_t
    {
        Unknown = 0,
        InvalidArgument,
        WindowCreationFailed,
        NoSuitableDevice,
        DeviceCreationFailed,
        SurfaceCreationFailed,
        SwapchainCreationFailed,
        AllocationFailed,
        ImageLoadFailed,
        ShaderLoadFailed,
        PipelineCreationFailed,
        DescriptorCreationFailed
    };

    struct Error
    {
        ErrorCode code{ErrorCode::Unknown};
        std::string message;
    };

    template<typename T>
    class Result
    {
    public:
        Result(T&& value) noexcept : m_value(std::move(value)) {}
        Result(Error error) noexcept : m_error(std::move(error)) {}

        [[nodiscard]] bool hasValue() const noexcept { return m_value.has_value(); }
        explicit operator bool() const noexcept { return hasValue(); }

        T& value() & noexcept { assert(hasValue()); return *m_value; }
        const T& value() const& noexcept { assert(hasValue()); return *m_value; }
        T&& value() && noexcept { assert(hasValue()); return std::move(*m_value); }

        T& operator*() & noexcept { return value(); }
        const T& operator*() const& noexcept { return value(); }
        T&& operator*() && noexcept { return std::move(*this).value(); }

        T* operator->() noexcept { assert(hasValue()); return &*m_value; }
        const T* operator->() const noexcept { assert(hasValue()); return &*m_value; }

        [[nodiscard]] const Error& error() const noexcept { assert(!hasValue()); return m_error; }
        [[nodiscard]] ErrorCode code() const noexcept { return hasValue() ? ErrorCode::Unknown : m_error.code; }
        [[nodiscard]] const std::string& message() const noexcept { assert(!hasValue()); return m_error.message; }

    private:
        std::optional<T> m_value;
        Error m_error;
    };

    template<>
    class Result<void>
    {
    public:
        Result() noexcept : m_succeeded(true) {}
        Result(Error error) noexcept : m_succeeded(false), m_error(std::move(error)) {}

        [[nodiscard]] bool hasValue() const noexcept { return m_succeeded; }
        explicit operator bool() const noexcept { return m_succeeded; }

        [[nodiscard]] const Error& error() const noexcept { assert(!m_succeeded); return m_error; }
        [[nodiscard]] ErrorCode code() const noexcept { return m_succeeded ? ErrorCode::Unknown : m_error.code; }
        [[nodiscard]] const std::string& message() const noexcept { assert(!m_succeeded); return m_error.message; }

    private:
        bool m_succeeded{false};
        Error m_error;
    };

    using Status = Result<void>;
} //namespace vela

#endif //VELA_RESULT_HPP
