#ifndef __LLM_GATEWAY_HPP__
#define __LLM_GATEWAY_HPP__

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <curl/curl.h>
#include <mutex>

namespace zhouyi {

// 简易JSON构建器（避免引入第三方JSON库）
class JsonBuilder {
public:
    static std::string Escape(const std::string& s) {
        std::string r;
        for (char c : s) {
            switch (c) {
                case '"':  r += "\\\""; break;
                case '\\': r += "\\\\"; break;
                case '\n': r += "\\n"; break;
                case '\r': r += "\\r"; break;
                case '\t': r += "\\t"; break;
                default:   r += c;
            }
        }
        return r;
    }
};

// 消息结构
struct ChatMessage {
    std::string role;    // system, user, assistant
    std::string content;
    
    std::string ToJson() const {
        return "{\"role\":\"" + JsonBuilder::Escape(role) + "\",\"content\":\"" + JsonBuilder::Escape(content) + "\"}";
    }
};

// LLM配置
struct LLMConfig {
    std::string api_base_url = "https://api.openai.com/v1";
    std::string api_key = "";
    std::string model = "gpt-3.5-turbo";
    int max_tokens = 2048;
    double temperature = 0.7;
};

// CURL响应缓冲
struct CurlResponse {
    std::string data;
    long status_code = 0;
};

class LLMGateway {
private:
    LLMConfig _config;
    std::mutex _curl_mutex;
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        CurlResponse* resp = static_cast<CurlResponse*>(userp);
        resp->data.append(static_cast<char*>(contents), total);
        return total;
    }
    
    // 简易JSON解析 - 提取字符串字段
    static std::string ExtractJsonString(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        
        // 找到冒号
        pos = json.find(':', pos + search.size());
        if (pos == std::string::npos) return "";
        
        // 跳过空白
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        
        if (pos >= json.size() || json[pos] != '"') return "";
        pos++; // 跳过开头引号
        
        std::string result;
        while (pos < json.size()) {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                switch (json[pos + 1]) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    default:   result += json[pos + 1];
                }
                pos += 2;
            } else if (json[pos] == '"') {
                break;
            } else {
                result += json[pos];
                pos++;
            }
        }
        return result;
    }

public:
    LLMGateway() = default;
    
    void SetConfig(const LLMConfig& config) {
        _config = config;
    }
    
    // 同步调用LLM
    // messages: 对话历史
    // 返回: assistant的回复内容
    std::string Chat(const std::vector<ChatMessage>& messages) {
        std::lock_guard<std::mutex> lock(_curl_mutex);
        
        // 构建请求JSON
        std::ostringstream body;
        body << "{";
        body << "\"model\":\"" << JsonBuilder::Escape(_config.model) << "\",";
        body << "\"messages\":[";
        for (size_t i = 0; i < messages.size(); i++) {
            if (i > 0) body << ",";
            body << messages[i].ToJson();
        }
        body << "],";
        body << "\"max_tokens\":" << _config.max_tokens << ",";
        body << "\"temperature\":" << _config.temperature;
        body << "}";
        
        std::string url = _config.api_base_url + "/chat/completions";
        std::string body_str = body.str();
        
        // 初始化CURL
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize CURL");
        }
        
        CurlResponse response;
        
        // 设置请求头
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string auth = "Authorization: Bearer " + _config.api_key;
        headers = curl_slist_append(headers, auth.c_str());
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body_str.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
        }
        
        if (response.status_code != 200) {
            std::cerr << "[LLM] HTTP " << response.status_code << ": " << response.data << std::endl;
            throw std::runtime_error("LLM API returned HTTP " + std::to_string(response.status_code));
        }
        
        // 解析响应: choices[0].message.content
        // 简易解析 - 找到 "content" 字段
        std::string content = ExtractLLMContent(response.data);
        if (content.empty()) {
            std::cerr << "[LLM] Failed to parse response: " << response.data << std::endl;
            throw std::runtime_error("Failed to parse LLM response");
        }
        
        return content;
    }
    
    // 从OpenAI格式响应中提取content
    static std::string ExtractLLMContent(const std::string& json) {
        // 找到 "choices" 数组中的第一个 "message" 对象的 "content"
        size_t choices_pos = json.find("\"choices\"");
        if (choices_pos == std::string::npos) return "";
        
        size_t message_pos = json.find("\"message\"", choices_pos);
        if (message_pos == std::string::npos) return "";
        
        size_t content_pos = json.find("\"content\"", message_pos);
        if (content_pos == std::string::npos) return "";
        
        // 从content位置开始提取字符串
        size_t colon_pos = json.find(':', content_pos + 9);
        if (colon_pos == std::string::npos) return "";
        
        size_t start = colon_pos + 1;
        while (start < json.size() && (json[start] == ' ' || json[start] == '\t')) start++;
        
        if (start >= json.size() || json[start] != '"') return "";
        start++; // 跳过开头引号
        
        std::string result;
        size_t pos = start;
        while (pos < json.size()) {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                switch (json[pos + 1]) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    default:   result += json[pos + 1];
                }
                pos += 2;
            } else if (json[pos] == '"') {
                break;
            } else {
                result += json[pos];
                pos++;
            }
        }
        return result;
    }
};

} // namespace zhouyi

#endif
