/**
 * @file logger.hpp
 * @brief Logging system implementation
 * @author AGI Team
 * @version 1.0.0
 * @date 2025-12-28
 */

#ifndef AGI_LOGGER_HPP
#define AGI_LOGGER_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <filesystem>
#include "utils.hpp"

namespace agi {

/**
 * @brief Log level
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

/**
 * @brief Log entry
 */
struct LogEntry {
    LogLevel level;
    std::string message;
    std::string timestamp;
    std::string category;
    
    std::string levelToString() const {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR:   return "ERROR";
            case LogLevel::CRITICAL:return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
    
    std::string format(const std::string& format = "[%timestamp%] [%level%] [%category%] %message%") const {
        std::string result = format;
        
        size_t pos;
        while ((pos = result.find("%timestamp%")) != std::string::npos) {
            result.replace(pos, 11, timestamp);
        }
        while ((pos = result.find("%level%")) != std::string::npos) {
            result.replace(pos, 7, levelToString());
        }
        while ((pos = result.find("%category%")) != std::string::npos) {
            result.replace(pos, 10, category);
        }
        while ((pos = result.find("%message%")) != std::string::npos) {
            result.replace(pos, 9, message);
        }
        
        return result;
    }
};

/**
 * @brief Log output target base class
 */
class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(const LogEntry& entry) = 0;
    virtual void flush() = 0;
};

/**
 * @brief Console log output
 */
class ConsoleLogSink : public LogSink {
private:
    bool use_colors_;
    bool show_debug_;
    
public:
    ConsoleLogSink(bool use_colors = true, bool show_debug = false)
        : use_colors_(use_colors), show_debug_(show_debug) {}
    
    void write(const LogEntry& entry) override {
        if (entry.level == LogLevel::DEBUG && !show_debug_) {
            return;
        }
        
        std::string formatted = entry.format();
        
        if (use_colors_) {
            std::cout << getColor(entry.level) << formatted 
                      << getResetColor() << std::endl;
        } else {
            std::cout << formatted << std::endl;
        }
    }
    
    void flush() override {
        std::cout.flush();
    }
    
private:
    static std::string getColor(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "\033[37m";  // White
            case LogLevel::INFO:    return "\033[32m";  // Green
            case LogLevel::WARNING: return "\033[33m";  // Yellow
            case LogLevel::ERROR:   return "\033[31m";  // Red
            case LogLevel::CRITICAL:return "\033[35m";  // Purple
            default: return "\033[0m";
        }
    }
    
    static std::string getResetColor() {
        return "\033[0m";
    }
};

/**
 * @brief File log output
 */
class FileLogSink : public LogSink {
private:
    std::string filepath_;
    std::ofstream file_;
    bool is_open_;
    
public:
    explicit FileLogSink(const std::string& filepath) 
        : filepath_(filepath), is_open_(false) {
        open();
    }
    
    ~FileLogSink() override {
        close();
    }
    
    void open() {
        if (!filepath_.empty()) {
            file_.open(filepath_, std::ios::app | std::ios::out);
            is_open_ = file_.is_open();
        }
    }
    
    void close() {
        if (file_.is_open()) {
            file_.close();
            is_open_ = false;
        }
    }
    
    void write(const LogEntry& entry) override {
        if (!is_open_) return;
        
        file_ << entry.format() << std::endl;
    }
    
    void flush() override {
        if (file_.is_open()) {
            file_.flush();
        }
    }
    
    bool isOpen() const {
        return is_open_;
    }
};

/**
 * @brief Main logging system class
 */
class Logger {
private:
    std::vector<std::unique_ptr<LogSink>> sinks_;
    LogLevel min_level_;
    std::string default_category_;
    bool initialized_;
    
public:
    Logger() : min_level_(LogLevel::DEBUG), default_category_("agi"), initialized_(false) {}
    
    ~Logger() {
        flush();
    }
    
    /**
     * @brief Initialize logging system
     */
    bool initialize(const std::string& log_path = "/var/log/agi/agi.log",
                    LogLevel level = LogLevel::INFO,
                    bool console_output = true) {
        if (initialized_) {
            return true;
        }
        
        min_level_ = level;
        
        // Create log directory
        std::string dir = PathUtils::parent(log_path);
        if (!dir.empty() && !std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
        
        // Add file log
        if (!log_path.empty()) {
            auto file_sink = std::make_unique<FileLogSink>(log_path);
            if (file_sink->isOpen()) {
                sinks_.push_back(std::move(file_sink));
            }
        }
        
        // Add console log
        if (console_output) {
            sinks_.push_back(std::make_unique<ConsoleLogSink>(true, level <= LogLevel::DEBUG));
        }
        
        initialized_ = true;
        
        info("agi", "Logging system initialization complete");
        return true;
    }
    
    /**
     * @brief Add log output target
     */
    void addSink(std::unique_ptr<LogSink> sink) {
        sinks_.push_back(std::move(sink));
    }
    
    /**
     * @brief Set minimum log level
     */
    void setMinLevel(LogLevel level) {
        min_level_ = level;
    }
    
    /**
     * @brief Set default category
     */
    void setDefaultCategory(const std::string& category) {
        default_category_ = category;
    }
    
    /**
     * @brief Record log
     */
    void log(LogLevel level, const std::string& category, const std::string& message) {
        if (level < min_level_) {
            return;
        }
        
        LogEntry entry;
        entry.level = level;
        entry.message = message;
        entry.timestamp = TimeUtils::isoNow();
        entry.category = category.empty() ? default_category_ : category;
        
        for (const auto& sink : sinks_) {
            sink->write(entry);
        }
    }
    
    /**
     * @brief Convenient log methods
     */
    void debug(const std::string& category, const std::string& message) {
        log(LogLevel::DEBUG, category, message);
    }
    
    void info(const std::string& category, const std::string& message) {
        log(LogLevel::INFO, category, message);
    }
    
    void warning(const std::string& category, const std::string& message) {
        log(LogLevel::WARNING, category, message);
    }
    
    void error(const std::string& category, const std::string& message) {
        log(LogLevel::ERROR, category, message);
    }
    
    void critical(const std::string& category, const std::string& message) {
        log(LogLevel::CRITICAL, category, message);
    }
    
    /**
     * @brief Flush all output
     */
    void flush() {
        for (const auto& sink : sinks_) {
            sink->flush();
        }
    }
    
    /**
     * @brief Get global log instance
     */
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
};

/**
 * @brief Global log macros
 */
#define AGI_LOG_DEBUG(msg)    agi::Logger::getInstance().debug("agi", msg)
#define AGI_LOG_INFO(msg)     agi::Logger::getInstance().info("agi", msg)
#define AGI_LOG_WARNING(msg)  agi::Logger::getInstance().warning("agi", msg)
#define AGI_LOG_ERROR(msg)    agi::Logger::getInstance().error("agi", msg)
#define AGI_LOG_CRITICAL(msg) agi::Logger::getInstance().critical("agi", msg)

/**
 * @brief Initialize logging system
 */
inline bool initLogger(const std::string& log_path = "/var/log/agi/agi.log",
                       LogLevel level = LogLevel::INFO) {
    return Logger::getInstance().initialize(log_path, level, true);
}

} // namespace agi

#endif // AGI_LOGGER_HPP
