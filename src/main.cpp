#include "http_server.hpp"
#include "database.hpp"
#include "llm_gateway.hpp"
#include "session_manager.hpp"
#include "meihua_engine.hpp"
#include "liuyao_engine.hpp"
#include "knowledge_base.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <csignal>
#include <filesystem>
#include <map>

using namespace zhouyi;

// 全局服务器指针（用于信号处理）
static HttpServer* g_server = nullptr;

void SignalHandler(int sig) {
    (void)sig;
    if (g_server) {
        std::cout << "\n[Main] Shutting down..." << std::endl;
        g_server->Stop();
    }
}

// 简易JSON解析工具
static std::string ParseJsonString(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size() || json[pos] != '"') return "";
    pos++;
    std::string result;
    while (pos < json.size()) {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            result += json[pos + 1];
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

static int ParseJsonInt(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return 0;
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return 0;
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    std::string num;
    while (pos < json.size() && (std::isdigit(json[pos]) || json[pos] == '-')) {
        num += json[pos];
        pos++;
    }
    if (num.empty()) return 0;
    return std::stoi(num);
}

// MIME类型映射
static std::string GetMimeType(const std::string& ext) {
    static std::map<std::string, std::string> mime_map = {
        {".html", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "application/javascript; charset=utf-8"},
        {".json", "application/json; charset=utf-8"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"}
    };
    auto it = mime_map.find(ext);
    if (it != mime_map.end()) return it->second;
    return "application/octet-stream";
}

// 读取静态文件
static bool ReadFile(const std::string& filepath, std::string& content) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    std::ostringstream ss;
    ss << file.rdbuf();
    content = ss.str();
    return true;
}

// 加载config.json
static LLMConfig LoadConfig(const std::string& config_path) {
    LLMConfig config;
    std::string content;
    if (!ReadFile(config_path, content)) {
        std::cerr << "[Config] Cannot open " << config_path << ", using defaults" << std::endl;
        return config;
    }
    
    config.api_base_url = ParseJsonString(content, "api_base_url");
    if (config.api_base_url.empty()) config.api_base_url = "https://api.openai.com/v1";
    
    config.api_key = ParseJsonString(content, "api_key");
    config.model = ParseJsonString(content, "model");
    if (config.model.empty()) config.model = "gpt-3.5-turbo";
    
    int mt = ParseJsonInt(content, "max_tokens");
    if (mt > 0) config.max_tokens = mt;
    
    // temperature需要特殊处理
    std::string temp_str = "\"temperature\"";
    size_t pos = content.find(temp_str);
    if (pos != std::string::npos) {
        pos = content.find(':', pos + temp_str.size());
        if (pos != std::string::npos) {
            pos++;
            while (pos < content.size() && content[pos] == ' ') pos++;
            std::string num;
            while (pos < content.size() && (std::isdigit(content[pos]) || content[pos] == '.')) {
                num += content[pos];
                pos++;
            }
            if (!num.empty()) {
                try { config.temperature = std::stod(num); } catch (...) {}
            }
        }
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    std::cout << R"(
  _____                    _   _               _              
 |__  /___ _  ___  ___   __| | | |__   ___  ___| |_ _ __ __ _ 
   / // _` |/ _ \/ _ \ / _` | | '_ \ / _ \/ __| __| '__/ _` |
  / /| (_| |  __/ (_) | (_| | | | | | (_) \__ \ |_| | | (_| |
 /____\__,_|\___|\___/ \__,_| |_| |_|\___/|___/\__|_|  \__,_|
                                                              
  梅花易数 AI 大师 - Zhou Yi Master Agent
)" << std::endl;
    
    // 加载配置
    std::string config_path = "config.json";
    if (argc > 1) {
        config_path = argv[1];
    }
    
    LLMConfig llm_config = LoadConfig(config_path);
    std::cout << "[Config] Model: " << llm_config.model << std::endl;
    std::cout << "[Config] API: " << llm_config.api_base_url << std::endl;
    
    // 初始化CURL
    curl_global_init(CURL_GLOBAL_ALL);
    
    // 初始化数据库
    Database db;
    std::string db_path = "zhouyi.db";
    if (!db.Open(db_path)) {
        std::cerr << "[Main] Failed to open database" << std::endl;
        return 1;
    }
    db.InitTables();
    std::cout << "[DB] Database initialized: " << db_path << std::endl;
    
    // 初始化LLM网关
    LLMGateway llm;
    llm.SetConfig(llm_config);
    
    // 初始化知识库
    KnowledgeBase kb;
    if (!kb.LoadAll("data")) {
        std::cerr << "[Main] Warning: Knowledge base partially loaded" << std::endl;
    }
    
    // 初始化会话管理器
    SessionManager session_mgr(db, llm, kb);
    
    // 静态文件目录
    std::string static_dir = "wwwroot";
    
    // 创建HTTP服务器
    int port = 8080;
    int threads = 4;
    // 从config读取端口
    {
        std::string content;
        if (ReadFile(config_path, content)) {
            int p = ParseJsonInt(content, "port");
            if (p > 0) port = p;
            int t = ParseJsonInt(content, "thread_count");
            if (t > 0) threads = t;
        }
    }
    
    HttpServer server(port, threads);
    g_server = &server;
    
    // 注册信号处理
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    // ==================== API路由 ====================
    
    // GET / - 首页
    server.Get("/", [&static_dir](const HttpRequest& req, HttpResponse& resp) {
        std::string content;
        std::string index_path = static_dir + "/index.html";
        if (ReadFile(index_path, content)) {
            resp.SetHtml(content);
        } else {
            resp.SetStatus(404, "Not Found");
            resp.SetHtml("<h1>404 - index.html not found</h1>");
        }
    });
    
    // POST /api/divination - 梅花易数起卦 + LLM解读
    server.Post("/api/divination", [&session_mgr](const HttpRequest& req, HttpResponse& resp) {
        std::string session_id = ParseJsonString(req.body, "session_id");
        std::string question = ParseJsonString(req.body, "question");
        std::string mode = ParseJsonString(req.body, "mode");
        int num1 = ParseJsonInt(req.body, "num1");
        int num2 = ParseJsonInt(req.body, "num2");
        int num3 = ParseJsonInt(req.body, "num3");
        
        if (mode.empty()) mode = "random";
        
        // 如果没有session_id，创建新会话
        if (session_id.empty()) {
            std::string title = question.empty() ? "梅花易数" : question.substr(0, 20);
            session_id = session_mgr.CreateSession(title);
        }
        
        std::string result = session_mgr.Divinate(session_id, question, mode, num1, num2, num3);
        resp.SetJson(result);
    });
    
    // POST /api/liuyao - 六爻起卦 + LLM解读
    server.Post("/api/liuyao", [&session_mgr](const HttpRequest& req, HttpResponse& resp) {
        std::string session_id = ParseJsonString(req.body, "session_id");
        std::string question = ParseJsonString(req.body, "question");
        std::string mode = ParseJsonString(req.body, "mode");
        
        if (mode.empty()) mode = "random";
        
        // 如果没有session_id，创建新会话
        if (session_id.empty()) {
            std::string title = question.empty() ? "六爻预测" : "六爻·" + question.substr(0, 15);
            session_id = session_mgr.CreateSession(title);
        }
        
        std::string result = session_mgr.LiuyaoDivinate(session_id, question, mode);
        resp.SetJson(result);
    });
    
    // GET /api/liuyao/random - 随机六爻排盘（纯本地，不调用LLM）
    server.Get("/api/liuyao/random", [](const HttpRequest& req, HttpResponse& resp) {
        auto gua = LiuyaoEngine::RandomGua();
        resp.SetJson(gua.gua_json);
    });
    
    // POST /api/chat - 继续对话
    server.Post("/api/chat", [&session_mgr](const HttpRequest& req, HttpResponse& resp) {
        std::string session_id = ParseJsonString(req.body, "session_id");
        std::string message = ParseJsonString(req.body, "message");
        
        if (session_id.empty() || message.empty()) {
            resp.SetStatus(400, "Bad Request");
            resp.SetJson("{\"error\":\"session_id and message are required\"}");
            return;
        }
        
        std::string result = session_mgr.Chat(session_id, message);
        resp.SetJson(result);
    });
    
    // GET /api/sessions - 获取会话列表
    server.Get("/api/sessions", [&session_mgr](const HttpRequest& req, HttpResponse& resp) {
        auto sessions = session_mgr.GetSessions();
        
        std::ostringstream json;
        json << "{\"sessions\":[";
        for (size_t i = 0; i < sessions.size(); i++) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"id\":\"" << sessions[i].id << "\",";
            json << "\"title\":\"" << JsonBuilder::Escape(sessions[i].title) << "\",";
            json << "\"last_gua_json\":\"" << JsonBuilder::Escape(sessions[i].last_gua_json) << "\",";
            json << "\"created_at\":" << sessions[i].created_at << ",";
            json << "\"updated_at\":" << sessions[i].updated_at;
            json << "}";
        }
        json << "]}";
        resp.SetJson(json.str());
    });
    
    // GET /api/messages?session_id=xxx - 获取会话消息
    server.Get("/api/messages", [&session_mgr](const HttpRequest& req, HttpResponse& resp) {
        auto it = req.query_params.find("session_id");
        if (it == req.query_params.end()) {
            resp.SetStatus(400, "Bad Request");
            resp.SetJson("{\"error\":\"session_id is required\"}");
            return;
        }
        
        auto messages = session_mgr.GetMessages(it->second);
        
        std::ostringstream json;
        json << "{\"messages\":[";
        for (size_t i = 0; i < messages.size(); i++) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"id\":" << messages[i].id << ",";
            json << "\"role\":\"" << messages[i].role << "\",";
            json << "\"content\":\"" << JsonBuilder::Escape(messages[i].content) << "\",";
            json << "\"gua_json\":\"" << JsonBuilder::Escape(messages[i].gua_json) << "\",";
            json << "\"created_at\":" << messages[i].created_at;
            json << "}";
        }
        json << "]}";
        resp.SetJson(json.str());
    });
    
    // DELETE /api/sessions?id=xxx - 删除会话
    // 由于HTTP Server只支持GET/POST，用POST模拟DELETE
    server.Post("/api/sessions/delete", [&session_mgr](const HttpRequest& req, HttpResponse& resp) {
        std::string session_id = ParseJsonString(req.body, "session_id");
        if (session_id.empty()) {
            resp.SetStatus(400, "Bad Request");
            resp.SetJson("{\"error\":\"session_id is required\"}");
            return;
        }
        bool ok = session_mgr.DeleteSession(session_id);
        resp.SetJson(std::string("{\"success\":") + (ok ? "true" : "false") + "}");
    });
    
    // GET /api/gua/random - 随机起卦（不调用LLM，纯本地）
    server.Get("/api/gua/random", [](const HttpRequest& req, HttpResponse& resp) {
        auto gua = MeihuaEngine::RandomGua();
        resp.SetJson(gua.gua_json);
    });
    
    // POST /api/gua/number - 数字起卦（不调用LLM，纯本地）
    server.Post("/api/gua/number", [](const HttpRequest& req, HttpResponse& resp) {
        int num1 = ParseJsonInt(req.body, "num1");
        int num2 = ParseJsonInt(req.body, "num2");
        int num3 = ParseJsonInt(req.body, "num3");
        
        if (num1 <= 0 || num2 <= 0) {
            resp.SetStatus(400, "Bad Request");
            resp.SetJson("{\"error\":\"num1 and num2 are required and must be positive\"}");
            return;
        }
        
        auto gua = MeihuaEngine::NumberGua(num1, num2, num3);
        resp.SetJson(gua.gua_json);
    });
    
    // 静态文件处理
    server.SetStaticHandler([&static_dir](const std::string& path, std::string& content, std::string& content_type) -> bool {
        // 安全检查：防止路径穿越
        if (path.find("..") != std::string::npos) return false;
        
        std::string filepath = static_dir + path;
        if (ReadFile(filepath, content)) {
            // 获取扩展名
            size_t dot = path.rfind('.');
            if (dot != std::string::npos) {
                content_type = GetMimeType(path.substr(dot));
            } else {
                content_type = "application/octet-stream";
            }
            return true;
        }
        return false;
    });
    
    // 启动服务器
    std::cout << "[Main] Starting Zhou Yi Master Agent..." << std::endl;
    server.Run();
    
    // 清理
    curl_global_cleanup();
    std::cout << "[Main] Bye!" << std::endl;
    return 0;
}
