/**
 * @file utils.hpp
 * @brief General utility functions and helper classes
 * @author AGI Team
 * @version 1.0.0
 * @date 2025-12-28
 */

#ifndef AGI_UTILS_HPP
#define AGI_UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <functional>
#include <optional>
#include <variant>

namespace agi {

/**
 * @brief Path utility class
 */
class PathUtils {
public:
    /**
     * @brief Normalize path, remove extra slashes and dots
     * @param path Original path
     * @return Normalized path
     */
    static std::string normalize(const std::string& path) {
        std::string result;
        bool lastWasSlash = false;
        
        for (char c : path) {
            if (c == '/') {
                if (!lastWasSlash) {
                    result += '/';
                    lastWasSlash = true;
                }
            } else {
                result += c;
                lastWasSlash = false;
            }
        }
        
        // Remove trailing slashes (unless it's root directory)
        if (!result.empty() && result.back() == '/' && result.size() > 1) {
            result.pop_back();
        }
        
        return result;
    }
    
    /**
     * @brief Get parent directory of path
     * @param path Path
     * @return Parent directory path
     */
    static std::string parent(const std::string& path) {
        std::string normalized = normalize(path);
        auto pos = normalized.rfind('/');
        if (pos == std::string::npos) {
            return ".";
        }
        return normalized.substr(0, pos);
    }
    
    /**
     * @brief Get filename from path
     * @param path Path
     * @return Filename
     */
    static std::string filename(const std::string& path) {
        std::string normalized = normalize(path);
        auto pos = normalized.rfind('/');
        if (pos == std::string::npos) {
            return normalized;
        }
        return normalized.substr(pos + 1);
    }
    
    /**
     * @brief Check if path is within specified directory (prevent directory traversal attack)
     * @param path Path to check
     * @param base Base directory
     * @return Whether within base directory
     */
    static bool isWithin(const std::string& path, const std::string& base) {
        std::string normalizedPath = normalize(path);
        std::string normalizedBase = normalize(base);
        
        if (normalizedPath.find(normalizedBase) == 0) {
            // Ensure it's a subpath, not a prefix match
            if (normalizedPath.size() > normalizedBase.size()) {
                return normalizedPath[normalizedBase.size()] == '/';
            }
            return true;
        }
        return false;
    }
    
    /**
     * @brief Create directory (recursive)
     * @param path Directory path
     * @param mode Permission mode
     * @return Whether successful
     */
    static bool createDirectory(const std::string& path, mode_t mode = 0755) {
        std::string normalized = normalize(path);
        std::string current;
        
        for (size_t i = 0; i <= normalized.size(); ++i) {
            if (i == normalized.size() || normalized[i] == '/') {
                if (!current.empty()) {
                    if (mkdir(current.c_str(), mode) != 0 && errno != EEXIST) {
                        return false;
                    }
                }
                current += '/';
            } else {
                current += normalized[i];
            }
        }
        
        return true;
    }
};

/**
 * @brief String utility class
 */
class StringUtils {
public:
    /**
     * @brief Remove whitespace from both ends of string
     * @param str Input string
     * @return Trimmed string
     */
    static std::string trim(const std::string& str) {
        size_t start = 0;
        while (start < str.size() && std::isspace(str[start])) {
            ++start;
        }
        
        if (start >= str.size()) {
            return "";
        }
        
        size_t end = str.size() - 1;
        while (end > start && std::isspace(str[end])) {
            --end;
        }
        
        return str.substr(start, end - start + 1);
    }
    
    /**
     * @brief Convert string to lowercase
     * @param str Input string
     * @return Lowercase string
     */
    static std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        return result;
    }
    
    /**
     * @brief Convert string to uppercase
     * @param str Input string
     * @return Uppercase string
     */
    static std::string toUpper(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::toupper(c); });
        return result;
    }
    
    /**
     * @brief Split string by delimiter
     * @param str Input string
     * @param delimiter Delimiter character
     * @return Vector of split strings
     */
    static std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string token;
        
        while (std::getline(ss, token, delimiter)) {
            if (!token.empty()) {
                result.push_back(token);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Check if string starts with prefix
     * @param str String
     * @param prefix Prefix
     * @return Whether string starts with prefix
     */
    static bool startsWith(const std::string& str, const std::string& prefix) {
        if (str.size() < prefix.size()) {
            return false;
        }
        return str.compare(0, prefix.size(), prefix) == 0;
    }
    
    /**
     * @brief Check if string ends with suffix
     * @param str String
     * @param suffix Suffix
     * @return Whether string ends with suffix
     */
    static bool endsWith(const std::string& str, const std::string& suffix) {
        if (str.size() < suffix.size()) {
            return false;
        }
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    
    /**
     * @brief String replacement
     * @param str Original string
     * @param from String to replace
     * @param to Replacement string
     * @return Replaced string
     */
    static std::string replace(const std::string& str, const std::string& from,
                               const std::string& to) {
        std::string result = str;
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.size(), to);
            pos += to.size();
        }
        return result;
    }
    
    /**
     * @brief Escape JSON string
     * @param str Original string
     * @return Escaped string
     */
    static std::string jsonEscape(const std::string& str) {
        std::string result;
        for (char c : str) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:   result += c; break;
            }
        }
        return result;
    }
};

/**
 * @brief File utility class
 */
class FileUtils {
public:
    /**
     * @brief Check if file exists
     * @param path File path
     * @return Whether exists
     */
    static bool exists(const std::string& path) {
        std::ifstream file(path);
        return file.good();
    }
    
    /**
     * @brief Read file content
     * @param path File path
     * @return File content
     */
    static std::string read(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + path);
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    /**
     * @brief Write file content
     * @param path File path
     * @param content Content
     * @return Whether successful
     */
    static bool write(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        file << content;
        return file.good();
    }
    
    /**
     * @brief Append content to file
     * @param path File path
     * @param content Content
     * @return Whether successful
     */
    static bool append(const std::string& path, const std::string& content) {
        std::ofstream file(path, std::ios::app);
        if (!file.is_open()) {
            return false;
        }
        file << content;
        return file.good();
    }
    
    /**
     * @brief Get file size
     * @param path File path
     * @return File size in bytes, -1 on failure
     */
    static long long size(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return -1;
        }
        return file.tellg();
    }
    
    /**
     * @brief Recursively remove directory
     * @param path Directory path
     * @return Whether successful
     */
    static bool removeRecursive(const std::string& path) {
        // Use system command for recursive removal
        std::string cmd = "rm -rf \"" + path + "\"";
        return system(cmd.c_str()) == 0;
    }
};

/**
 * @brief Time utility class
 */
class TimeUtils {
public:
    /**
     * @brief Get current time as string
     * @param format Time format
     * @return Time string
     */
    static std::string now(const std::string& format = "%Y-%m-%d %H:%M:%S") {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), format.c_str());
        return ss.str();
    }
    
    /**
     * @brief Get ISO format time string
     * @return ISO time string
     */
    static std::string isoNow() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
        return ss.str();
    }
};

/**
 * @brief Command execution utility class
 */
class CommandUtils {
public:
    /**
     * @brief Execute command and get output
     * @param cmd Command
     * @param captureStderr Whether to capture stderr
     * @return Command output
     */
    static std::string exec(const std::string& cmd, bool captureStderr = false) {
        std::string fullCmd = cmd;
        if (captureStderr) {
            fullCmd += " 2>&1";
        } else {
            fullCmd += " 2>/dev/null";
        }
        
        FILE* pipe = popen(fullCmd.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("Cannot execute command: " + cmd);
        }
        
        char buffer[4096];
        std::string result;
        
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        
        int status = pclose(pipe);
        if (status != 0) {
            throw std::runtime_error("Command execution failed: " + cmd + " (exit code: " +
                                    std::to_string(status) + ")");
        }
        
        return result;
    }
    
    /**
     * @brief Execute command (silent mode)
     * @param cmd Command
     * @return Whether successful
     */
    static bool execSilent(const std::string& cmd) {
        return system(cmd.c_str()) == 0;
    }
};

/**
 * @brief Result type for error handling
 * @tparam T Result type
 */
template<typename T>
class Result {
private:
    std::optional<T> value_;
    std::optional<std::string> error_;
    
public:
    Result() = default;
    
    static Result<T> ok(const T& val) {
        Result<T> r;
        r.value_ = val;
        return r;
    }
    
    static Result<T> fail(const std::string& err) {
        Result<T> r;
        r.error_ = err;
        return r;
    }
    
    bool isOk() const { return value_.has_value(); }
    bool isFail() const { return error_.has_value(); }
    
    T value() const {
        if (!value_) {
            throw std::runtime_error("Result has no value: " + (error_ ? *error_ : "unknown"));
        }
        return *value_;
    }
    
    std::string error() const {
        if (!error_) {
            return "No error";
        }
        return *error_;
    }
    
    T valueOr(const T& defaultVal) const {
        return value_ ? *value_ : defaultVal;
    }
};

/**
 * @brief Configuration error exception class
 */
class ConfigError : public std::runtime_error {
public:
    ConfigError(const std::string& msg) : std::runtime_error(msg) {}
};

/**
 * @brief Jail error exception class
 */
class JailError : public std::runtime_error {
public:
    JailError(const std::string& msg) : std::runtime_error(msg) {}
};

} // namespace agi

#endif // AGI_UTILS_HPP
