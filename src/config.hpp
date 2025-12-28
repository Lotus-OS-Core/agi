/**
 * @file config.hpp
 * @brief JSON configuration parser and management class
 * @author AGI Team
 * @version 1.0.0
 * @date 2025-12-28
 */

#ifndef AGI_CONFIG_HPP
#define AGI_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <stdexcept>
#include <iostream>
#include "utils.hpp"

namespace agi {

/**
 * @brief JSON value type
 */
class JsonValue {
public:
    using ValueType = std::variant<
        std::nullptr_t,
        bool,
        double,
        std::string,
        std::vector<JsonValue>,
        std::map<std::string, JsonValue>
    >;
    
private:
    ValueType value_;
    
public:
    JsonValue() : value_(nullptr) {}
    
    // Constructors
    JsonValue(std::nullptr_t) : value_(nullptr) {}
    JsonValue(bool val) : value_(val) {}
    JsonValue(double val) : value_(val) {}
    JsonValue(const char* val) : value_(std::string(val)) {}
    JsonValue(const std::string& val) : value_(val) {}
    JsonValue(const std::vector<JsonValue>& val) : value_(val) {}
    JsonValue(const std::map<std::string, JsonValue>& val) : value_(val) {}
    
    // Type checking methods
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
    bool isBool() const { return std::holds_alternative<bool>(value_); }
    bool isNumber() const { return std::holds_alternative<double>(value_); }
    bool isString() const { return std::holds_alternative<std::string>(value_); }
    bool isArray() const { return std::holds_alternative<std::vector<JsonValue>>(value_); }
    bool isObject() const { return std::holds_alternative<std::map<std::string, JsonValue>>(value_); }
    
    // Get values
    bool asBool() const { return std::get<bool>(value_); }
    double asNumber() const { return std::get<double>(value_); }
    std::string asString() const { return std::get<std::string>(value_); }
    std::vector<JsonValue>& asArray() { return std::get<std::vector<JsonValue>>(value_); }
    const std::vector<JsonValue>& asArray() const { return std::get<std::vector<JsonValue>>(value_); }
    std::map<std::string, JsonValue>& asObject() { return std::get<std::map<std::string, JsonValue>>(value_); }
    const std::map<std::string, JsonValue>& asObject() const { return std::get<std::map<std::string, JsonValue>>(value_); }
    
    // Index access (for arrays and objects)
    JsonValue& operator[](size_t index) {
        if (!isArray()) {
            throw std::runtime_error("Not an array type");
        }
        return asArray()[index];
    }
    
    const JsonValue& operator[](size_t index) const {
        if (!isArray()) {
            throw std::runtime_error("Not an array type");
        }
        return asArray()[index];
    }
    
    JsonValue& operator[](const std::string& key) {
        if (!isObject()) {
            throw std::runtime_error("Not an object type");
        }
        return asObject()[key];
    }
    
    const JsonValue& operator[](const std::string& key) const {
        if (!isObject()) {
            throw std::runtime_error("Not an object type");
        }
        auto it = asObject().find(key);
        if (it == asObject().end()) {
            throw std::runtime_error("Key does not exist: " + key);
        }
        return it->second;
    }
    
    bool contains(const std::string& key) const {
        if (!isObject()) return false;
        return asObject().find(key) != asObject().end();
    }
    
    // Serialization
    std::string serialize(int indent = 0) const {
        std::string result;
        serializeTo(result, indent, 0);
        return result;
    }
    
private:
    void serializeTo(std::string& result, int indent, int currentIndent) const {
        std::string spaces(currentIndent * indent, ' ');
        
        if (isNull()) {
            result += "null";
        } else if (isBool()) {
            result += asBool() ? "true" : "false";
        } else if (isNumber()) {
            result += std::to_string(asNumber());
        } else if (isString()) {
            result += "\"" + StringUtils::jsonEscape(asString()) + "\"";
        } else if (isArray()) {
            result += "[\n";
            const auto& arr = asArray();
            for (size_t i = 0; i < arr.size(); ++i) {
                result += spaces + std::string(indent, ' ');
                arr[i].serializeTo(result, indent, currentIndent + 1);
                if (i < arr.size() - 1) result += ",";
                result += "\n";
            }
            result += spaces + "]";
        } else if (isObject()) {
            result += "{\n";
            const auto& obj = asObject();
            size_t count = 0;
            for (const auto& [key, val] : obj) {
                result += spaces + std::string(indent, ' ');
                result += "\"" + StringUtils::jsonEscape(key) + "\": ";
                val.serializeTo(result, indent, currentIndent + 1);
                if (++count < obj.size()) result += ",";
                result += "\n";
            }
            result += spaces + "}";
        }
    }
};

/**
 * @brief JSON parser
 */
class JsonParser {
private:
    std::string json_;
    size_t pos_;
    
public:
    JsonParser(const std::string& json) : json_(json), pos_(0) {}
    
    JsonValue parse() {
        skipWhitespace();
        JsonValue result = parseValue();
        skipWhitespace();
        if (pos_ < json_.size()) {
            throw ConfigError("JSON parsing complete but there is remaining content");
        }
        return result;
    }
    
    static JsonValue parseFromString(const std::string& json) {
        JsonParser parser(json);
        return parser.parse();
    }
    
private:
    void skipWhitespace() {
        while (pos_ < json_.size() && std::isspace(json_[pos_])) {
            ++pos_;
        }
    }
    
    char peek() {
        if (pos_ < json_.size()) {
            return json_[pos_];
        }
        return '\0';
    }
    
    char consume() {
        if (pos_ < json_.size()) {
            return json_[pos_++];
        }
        return '\0';
    }
    
    JsonValue parseValue() {
        skipWhitespace();
        char c = peek();
        
        if (c == '\0') {
            throw ConfigError("Unexpected end of string");
        }
        
        if (c == 'n') {
            return parseNull();
        } else if (c == 't' || c == 'f') {
            return parseBool();
        } else if (c == '"') {
            return parseString();
        } else if (c == '[') {
            return parseArray();
        } else if (c == '{') {
            return parseObject();
        } else if (c == '-' || std::isdigit(c)) {
            return parseNumber();
        } else {
            throw ConfigError("Invalid character: " + std::string(1, c));
        }
    }
    
    JsonValue parseNull() {
        expect("null");
        return nullptr;
    }
    
    JsonValue parseBool() {
        if (peek() == 't') {
            expect("true");
            return true;
        } else {
            expect("false");
            return false;
        }
    }
    
    JsonValue parseString() {
        expect("\"");
        std::string result;
        
        while (peek() != '"' && peek() != '\0') {
            char c = consume();
            
            if (c == '\\') {
                char next = consume();
                switch (next) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/'; break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case 'u':
                        result += parseUnicodeEscape();
                        break;
                    default:
                        result += next;
                        break;
                }
            } else {
                result += c;
            }
        }
        
        expect("\"");
        return result;
    }
    
    char parseUnicodeEscape() {
        std::string hex;
        for (int i = 0; i < 4; ++i) {
            hex += consume();
        }
        
        int value = std::stoi(hex, nullptr, 16);
        return static_cast<char>(value);
    }
    
    JsonValue parseNumber() {
        size_t start = pos_;
        bool hasDecimal = false;
        
        if (consume() == '-') {}
        else { --pos_; }
        
        // Integer part
        while (std::isdigit(peek())) {
            consume();
        }
        
        // Fractional part
        if (peek() == '.') {
            hasDecimal = true;
            consume();
            while (std::isdigit(peek())) {
                consume();
            }
        }
        
        // Exponent part
        if (peek() == 'e' || peek() == 'E') {
            consume();
            if (peek() == '+' || peek() == '-') {
                consume();
            }
            while (std::isdigit(peek())) {
                consume();
            }
        }
        
        double value = std::stod(json_.substr(start, pos_ - start));
        return value;
    }
    
    JsonValue parseArray() {
        expect("[");
        std::vector<JsonValue> result;
        
        skipWhitespace();
        if (peek() == ']') {
            consume();
            return result;
        }
        
        while (true) {
            result.push_back(parseValue());
            skipWhitespace();
            
            if (peek() == ',') {
                consume();
                continue;
            } else if (peek() == ']') {
                consume();
                break;
            } else {
                throw ConfigError("Expected comma or closing bracket in array");
            }
        }
        
        return result;
    }
    
    JsonValue parseObject() {
        expect("{");
        std::map<std::string, JsonValue> result;
        
        skipWhitespace();
        if (peek() == '}') {
            consume();
            return result;
        }
        
        while (true) {
            skipWhitespace();
            
            if (peek() != '"') {
                throw ConfigError("Object keys must be strings");
            }
            
            std::string key = parseString().asString();
            skipWhitespace();
            
            expect(":");
            skipWhitespace();
            
            result[key] = parseValue();
            skipWhitespace();
            
            if (peek() == ',') {
                consume();
                continue;
            } else if (peek() == '}') {
                consume();
                break;
            } else {
                throw ConfigError("Expected comma or closing bracket in object");
            }
        }
        
        return result;
    }
    
    void expect(const std::string& str) {
        if (json_.substr(pos_, str.size()) != str) {
            throw ConfigError("Expected: " + str);
        }
        pos_ += str.size();
    }
};

/**
 * @brief SSH configuration structure
 */
struct SshConfig {
    int port = 22;
    std::string listen_address = "127.0.0.1";
    std::string root_password = "";
    bool password_auth = true;
    bool pubkey_auth = true;
    int max_auth_tries = 3;
    int client_alive_interval = 300;
    std::string banner_message = "";
};

/**
 * @brief Resource limits configuration
 */
struct ResourceLimits {
    long max_cpu_time = 300;       // seconds
    long max_memory = 524288;      // KB
    long max_file_size = 1048576;  // KB
    int max_processes = 64;
    int max_open_files = 256;
};

/**
 * @brief Mount point configuration
 */
struct MountPoint {
    std::string source;
    std::string target;
    std::string type = "";
    unsigned long flags = 0;
    bool read_only = false;
};

/**
 * @brief Allowed user/group configuration
 */
struct UserConfig {
    std::string name;
    std::string shell = "/bin/bash";
    std::string home = "/home";
    bool sudo = false;
};

/**
 * @brief Environment configuration
 */
struct EnvironmentConfig {
    std::string name;
    std::string description = "";
    std::string os_template = "debian";
    std::string architecture = "x86_64";
    
    // Path configuration
    std::string root_path = "";
    std::string data_path = "";
    
    // SSH configuration
    SshConfig ssh;
    
    // Resource limits
    ResourceLimits limits;
    
    // Mount configuration
    std::vector<MountPoint> mounts;
    
    // User configuration
    std::vector<UserConfig> users;
    
    // Environment variables
    std::map<std::string, std::string> environment;
    
    // Startup scripts
    std::string init_script = "";
    std::string cleanup_script = "";
    
    // Status
    bool enabled = true;
};

/**
 * @brief Global configuration
 */
struct GlobalConfig {
    std::string app_name = "agi";
    std::string version = "1.0.0";
    std::string base_path = "/var/lib/agi";
    std::string config_path = "/etc/agi";
    std::string log_path = "/var/log/agi";
    std::string template_path = "/usr/share/agi/templates";
    
    std::string log_level = "INFO";
    bool daemonize = false;
    std::string pid_file = "/var/run/agi.pid";
    
    std::vector<EnvironmentConfig> environments;
};

/**
 * @brief Configuration manager
 */
class ConfigManager {
private:
    GlobalConfig config_;
    std::string config_file_;
    std::string error_;
    
public:
    ConfigManager() : config_file_("/etc/agi/agi_config.json") {}
    
    bool load(const std::string& path = "") {
        if (!path.empty()) {
            config_file_ = path;
        }
        
        if (!FileUtils::exists(config_file_)) {
            error_ = "Configuration file does not exist: " + config_file_;
            return false;
        }
        
        try {
            std::string content = FileUtils::read(config_file_);
            JsonValue json = JsonParser::parseFromString(content);
            
            if (!json.isObject()) {
                error_ = "Root element of configuration file must be an object";
                return false;
            }
            
            parseGlobalConfig(json.asObject());
            error_.clear();
            return true;
            
        } catch (const std::exception& e) {
            error_ = std::string("Configuration parsing error: ") + e.what();
            return false;
        }
    }
    
    bool save(const std::string& path = "") {
        std::string outputPath = path.empty() ? config_file_ : path;
        
        try {
            JsonValue json = serializeToJson();
            std::string content = json.serialize(2);
            
            if (!FileUtils::write(outputPath, content)) {
                error_ = "Cannot write configuration file: " + outputPath;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            error_ = std::string("Configuration save error: ") + e.what();
            return false;
        }
    }
    
    const GlobalConfig& getConfig() const { return config_; }
    GlobalConfig& getConfig() { return config_; }
    
    const std::string& getError() const { return error_; }
    
    bool validate() {
        error_.clear();
        
        // Check base path
        if (config_.base_path.empty()) {
            error_ = "base_path cannot be empty";
            return false;
        }
        
        // Check environment configurations
        for (const auto& env : config_.environments) {
            if (env.name.empty()) {
                error_ = "Environment name cannot be empty";
                return false;
            }
            
            // Check port range
            if (env.ssh.port < 1 || env.ssh.port > 65535) {
                error_ = "SSH port must be in range 1-65535";
                return false;
            }
            
            // Check path safety
            std::string unsafePath = "/etc,/var/lib,/var/run";
            for (const auto& mount : env.mounts) {
                for (const auto& bad : StringUtils::split(unsafePath, ',')) {
                    if (StringUtils::startsWith(mount.source, StringUtils::trim(bad))) {
                        error_ = "Mounting system critical paths is forbidden: " + mount.source;
                        return false;
                    }
                }
            }
        }
        
        return true;
    }
    
    const EnvironmentConfig* findEnvironment(const std::string& name) const {
        for (const auto& env : config_.environments) {
            if (env.name == name) {
                return &env;
            }
        }
        return nullptr;
    }
    
    static ConfigManager createDefault() {
        ConfigManager manager;
        manager.config_.base_path = "/var/lib/agi";
        manager.config_.log_path = "/var/log/agi";
        
        // Add default environment example
        EnvironmentConfig devEnv;
        devEnv.name = "dev-env";
        devEnv.description = "Development environment";
        devEnv.os_template = "debian";
        devEnv.ssh.port = 2201;
        devEnv.ssh.listen_address = "127.0.0.1";
        manager.config_.environments.push_back(devEnv);
        
        return manager;
    }
    
    std::string generateDefaultConfig() {
        ConfigManager def = createDefault();
        return def.serializeToJson().serialize(2);
    }
    
private:
    void parseGlobalConfig(const std::map<std::string, JsonValue>& json) {
        // Parse global configuration
        for (const auto& [key, value] : json) {
            if (key == "global") {
                parseGlobalSection(value.asObject());
            } else if (key == "environments") {
                parseEnvironments(value.asArray());
            }
        }
    }
    
    void parseGlobalSection(const std::map<std::string, JsonValue>& global) {
        for (const auto& [key, value] : global) {
            if (key == "base_path") config_.base_path = value.asString();
            else if (key == "log_path") config_.log_path = value.asString();
            else if (key == "template_path") config_.template_path = value.asString();
            else if (key == "log_level") config_.log_level = value.asString();
            else if (key == "daemonize") config_.daemonize = value.asBool();
        }
    }
    
    void parseEnvironments(const std::vector<JsonValue>& envArray) {
        for (const auto& envJson : envArray) {
            if (!envJson.isObject()) continue;
            
            const auto& envObj = envJson.asObject();
            EnvironmentConfig env;
            
            // Basic information
            if (envObj.count("name")) env.name = envObj.at("name").asString();
            if (envObj.count("description")) env.description = envObj.at("description").asString();
            if (envObj.count("os_template")) env.os_template = envObj.at("os_template").asString();
            if (envObj.count("architecture")) env.architecture = envObj.at("architecture").asString();
            
            // SSH configuration
            if (envObj.count("ssh")) {
                const auto& sshObj = envObj.at("ssh").asObject();
                if (sshObj.count("port")) env.ssh.port = static_cast<int>(sshObj.at("port").asNumber());
                if (sshObj.count("listen_address")) env.ssh.listen_address = sshObj.at("listen_address").asString();
            }
            
            // User configuration
            if (envObj.count("users")) {
                for (const auto& userJson : envObj.at("users").asArray()) {
                    if (!userJson.isObject()) continue;
                    const auto& userObj = userJson.asObject();
                    UserConfig user;
                    if (userObj.count("name")) user.name = userObj.at("name").asString();
                    env.users.push_back(user);
                }
            }
            
            config_.environments.push_back(env);
        }
    }
    
    JsonValue serializeToJson() const {
        JsonValue global(JsonValue::ValueType(std::map<std::string, JsonValue>{}));
        global["base_path"] = config_.base_path;
        global["log_path"] = config_.log_path;
        global["template_path"] = config_.template_path;
        global["log_level"] = config_.log_level;
        global["daemonize"] = config_.daemonize;
        
        JsonValue envArray(std::vector<JsonValue>{});
        for (const auto& env : config_.environments) {
            JsonValue envJson(std::map<std::string, JsonValue>{});
            envJson["name"] = env.name;
            envJson["description"] = env.description;
            envJson["os_template"] = env.os_template;
            envJson["architecture"] = env.architecture;
            envJson["enabled"] = env.enabled;
            
            JsonValue sshObj(std::map<std::string, JsonValue>{});
            sshObj["port"] = env.ssh.port;
            sshObj["listen_address"] = env.ssh.listen_address;
            envJson["ssh"] = sshObj;
            
            envArray.asArray().push_back(envJson);
        }
        
        JsonValue result(std::map<std::string, JsonValue>{});
        result["global"] = global;
        result["environments"] = envArray;
        
        return result;
    }
};

} // namespace agi

#endif // AGI_CONFIG_HPP
