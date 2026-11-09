#ifndef VELA_LOG_HPP
#define VELA_LOG_HPP

#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace vela
{
    // Severity of a message Vela logs. Off is only a minimum level: it silences everything
    enum class LogLevel : uint8_t
    {
        Debug = 0,  // per-object chatter, e.g. every pipeline and sampler created
        Info,       // device selection and capabilities
        Warning,    // Vulkan validation warnings, fallbacks
        Error,      // failures, Vulkan validation errors
        Off
    };

    // Receives every message at or above the minimum level. message has no trailing newline
    using LogCallback = std::function<void(LogLevel level, std::string_view message)>;

    namespace detail
    {
        struct LogState
        {
            std::mutex mutex;
            LogLevel minimum{LogLevel::Debug};
            LogCallback callback;
        };

        inline LogState& logState()
        {
            static LogState state;
            return state;
        }
    } //namespace detail

    // Sets the least severe level that is still logged. Debug (the default) logs everything.
    // minimum: messages below this level are dropped; LogLevel::Off drops all of them.
    inline void setLogLevel(LogLevel minimum)
    {
        detail::LogState& state = detail::logState();
        const std::scoped_lock lock(state.mutex);
        state.minimum = minimum;
    }

    // Sends Vela's messages to callback instead of the console (Info and Debug to stdout, Warning and Error to stderr).
    // callback: called for every message that passes the level, possibly from inside Vulkan's validation layers;
    //           an empty function restores console output.
    inline void setLogCallback(LogCallback callback)
    {
        detail::LogState& state = detail::logState();
        const std::scoped_lock lock(state.mutex);
        state.callback = std::move(callback);
    }

    // Logs one message through the current callback, or the console.
    // level:   severity of the message.
    // message: text without a trailing newline.
    inline void log(LogLevel level, std::string_view message)
    {
        detail::LogState& state = detail::logState();
        const std::scoped_lock lock(state.mutex);

        if (level == LogLevel::Off || level < state.minimum)
            return;

        if (state.callback)
        {
            state.callback(level, message);
            return;
        }

        std::ostream& stream = level >= LogLevel::Warning ? std::cerr : std::cout;
        stream << message << '\n';
    }

    namespace detail
    {
        // Collects one message with operator<< and logs it when destroyed; a trailing newline is dropped
        class LogLine
        {
        public:
            explicit LogLine(LogLevel level) : m_level(level) {}
            LogLine(const LogLine&) = delete;
            LogLine& operator=(const LogLine&) = delete;

            ~LogLine()
            {
                std::string message = m_stream.str();

                while (!message.empty() && message.back() == '\n')
                    message.pop_back();

                log(m_level, message);
            }

            template<typename T>
            LogLine& operator<<(const T& value)
            {
                m_stream << value;
                return *this;
            }

            LogLine& operator<<(std::ostream& (*manipulator)(std::ostream&))
            {
                m_stream << manipulator;
                return *this;
            }

        private:
            LogLevel m_level;
            std::ostringstream m_stream;
        };
    } //namespace detail
} //namespace vela

#endif //VELA_LOG_HPP
