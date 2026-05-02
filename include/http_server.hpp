#ifndef __HTTP_SERVER_HPP__
#define __HTTP_SERVER_HPP__

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <mutex>

namespace zhouyi {

// HTTP请求结构
struct HttpRequest {
    std::string method;
    std::string path;
    std::string query_string;
    std::string body;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    
    // 解析查询参数
    void ParseQuery() {
        if (query_string.empty()) return;
        std::istringstream ss(query_string);
        std::string pair;
        while (std::getline(ss, pair, '&')) {
            size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                query_params[pair.substr(0, eq)] = pair.substr(eq + 1);
            }
        }
    }
};

// HTTP响应结构
struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() {
        headers["Content-Type"] = "text/html; charset=utf-8";
        headers["Connection"] = "keep-alive";
    }
    
    void SetJson(const std::string& json) {
        headers["Content-Type"] = "application/json; charset=utf-8";
        headers["Access-Control-Allow-Origin"] = "*";
        headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
        headers["Access-Control-Allow-Headers"] = "Content-Type";
        body = json;
    }
    
    void SetHtml(const std::string& html) {
        headers["Content-Type"] = "text/html; charset=utf-8";
        body = html;
    }
    
    void SetStatus(int code, const std::string& text) {
        status_code = code;
        status_text = text;
    }
    
    std::string ToString() const {
        std::ostringstream resp;
        resp << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
        for (const auto& h : headers) {
            resp << h.first << ": " << h.second << "\r\n";
        }
        resp << "Content-Length: " << body.size() << "\r\n";
        resp << "\r\n";
        resp << body;
        return resp.str();
    }
};

// 路由处理器类型
using RouteHandler = std::function<void(const HttpRequest&, HttpResponse&)>;
using StaticFileHandler = std::function<bool(const std::string& path, std::string& content, std::string& content_type)>;

class HttpServer {
private:
    int _port;
    int _thread_count;
    int _server_fd;
    std::atomic<bool> _running;
    std::vector<std::thread> _workers;
    
    // 路由表
    std::map<std::string, RouteHandler> _get_routes;
    std::map<std::string, RouteHandler> _post_routes;
    StaticFileHandler _static_handler;
    
    // 解析HTTP请求
    bool ParseRequest(const std::string& raw, HttpRequest& req) {
        // 解析请求行
        size_t line_end = raw.find("\r\n");
        if (line_end == std::string::npos) return false;
        
        std::string request_line = raw.substr(0, line_end);
        size_t sp1 = request_line.find(' ');
        size_t sp2 = request_line.find(' ', sp1 + 1);
        if (sp1 == std::string::npos || sp2 == std::string::npos) return false;
        
        req.method = request_line.substr(0, sp1);
        std::string uri = request_line.substr(sp1 + 1, sp2 - sp1 - 1);
        
        // 分离path和query
        size_t qm = uri.find('?');
        if (qm != std::string::npos) {
            req.path = uri.substr(0, qm);
            req.query_string = uri.substr(qm + 1);
            req.ParseQuery();
        } else {
            req.path = uri;
        }
        
        // URL decode
        req.path = UrlDecode(req.path);
        
        // 解析headers
        size_t pos = line_end + 2;
        while (pos < raw.size()) {
            size_t next = raw.find("\r\n", pos);
            if (next == std::string::npos || next == pos) break;
            std::string header_line = raw.substr(pos, next - pos);
            size_t colon = header_line.find(':');
            if (colon != std::string::npos) {
                std::string key = header_line.substr(0, colon);
                std::string val = header_line.substr(colon + 1);
                // trim
                while (!val.empty() && val[0] == ' ') val.erase(0, 1);
                req.headers[key] = val;
            }
            pos = next + 2;
        }
        
        // 解析body
        size_t body_start = raw.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            req.body = raw.substr(body_start + 4);
        }
        
        return true;
    }
    
    // 处理单个连接
    void HandleConnection(int client_fd) {
        // 设置超时
        struct timeval tv;
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        char buffer[8192];
        std::string accumulated;
        
        while (_running) {
            int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0) break;
            buffer[n] = '\0';
            accumulated += buffer;
            
            // 检查是否收到完整的请求
            size_t header_end = accumulated.find("\r\n\r\n");
            if (header_end == std::string::npos) continue;
            
            // 检查Content-Length
            HttpRequest req;
            if (!ParseRequest(accumulated, req)) {
                break;
            }
            
            // 处理OPTIONS预检请求
            if (req.method == "OPTIONS") {
                HttpResponse resp;
                resp.headers["Access-Control-Allow-Origin"] = "*";
                resp.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
                resp.headers["Access-Control-Allow-Headers"] = "Content-Type";
                resp.headers["Content-Length"] = "0";
                std::string resp_str = resp.ToString();
                send(client_fd, resp_str.c_str(), resp_str.size(), 0);
                break;
            }
            
            // 路由分发
            HttpResponse resp;
            RouteDispatch(req, resp);
            
            std::string resp_str = resp.ToString();
            send(client_fd, resp_str.c_str(), resp_str.size(), 0);
            
            // 非keep-alive则关闭
            auto it = req.headers.find("Connection");
            if (it != req.headers.end() && it->second == "close") break;
            
            accumulated.clear();
        }
        
        close(client_fd);
    }
    
    // 路由分发
    void RouteDispatch(const HttpRequest& req, HttpResponse& resp) {
        // 先匹配精确路由
        if (req.method == "GET") {
            auto it = _get_routes.find(req.path);
            if (it != _get_routes.end()) {
                it->second(req, resp);
                return;
            }
        } else if (req.method == "POST") {
            auto it = _post_routes.find(req.path);
            if (it != _post_routes.end()) {
                it->second(req, resp);
                return;
            }
        }
        
        // 尝试静态文件
        if (_static_handler) {
            std::string content, content_type;
            if (_static_handler(req.path, content, content_type)) {
                resp.headers["Content-Type"] = content_type;
                resp.body = content;
                return;
            }
        }
        
        // 404
        resp.SetStatus(404, "Not Found");
        resp.SetJson("{\"error\":\"Not Found\",\"path\":\"" + req.path + "\"}");
    }
    
    // URL解码
    static std::string UrlDecode(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.size(); i++) {
            if (str[i] == '%' && i + 2 < str.size()) {
                int val;
                std::istringstream iss(str.substr(i + 1, 2));
                if (iss >> std::hex >> val) {
                    result += static_cast<char>(val);
                    i += 2;
                } else {
                    result += str[i];
                }
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }

public:
    HttpServer(int port = 8080, int thread_count = 4) 
        : _port(port), _thread_count(thread_count), _server_fd(-1), _running(false) {}
    
    ~HttpServer() { Stop(); }
    
    void Get(const std::string& path, RouteHandler handler) {
        _get_routes[path] = handler;
    }
    
    void Post(const std::string& path, RouteHandler handler) {
        _post_routes[path] = handler;
    }
    
    void SetStaticHandler(StaticFileHandler handler) {
        _static_handler = handler;
    }
    
    bool Start() {
        // 忽略SIGPIPE
        signal(SIGPIPE, SIG_IGN);
        
        // 创建socket
        _server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_server_fd < 0) {
            std::cerr << "[HTTP] Failed to create socket" << std::endl;
            return false;
        }
        
        // 设置SO_REUSEADDR
        int opt = 1;
        setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(_port);
        
        if (bind(_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "[HTTP] Failed to bind port " << _port << std::endl;
            close(_server_fd);
            return false;
        }
        
        if (listen(_server_fd, 128) < 0) {
            std::cerr << "[HTTP] Failed to listen" << std::endl;
            close(_server_fd);
            return false;
        }
        
        _running = true;
        
        std::cout << "[HTTP] Server started on port " << _port 
                  << " with " << _thread_count << " threads" << std::endl;
        
        // 启动工作线程
        for (int i = 0; i < _thread_count; i++) {
            _workers.emplace_back([this]() {
                while (_running) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    
                    int client_fd = accept(_server_fd, (struct sockaddr*)&client_addr, &client_len);
                    if (client_fd < 0) {
                        if (_running) continue;
                        break;
                    }
                    
                    HandleConnection(client_fd);
                }
            });
        }
        
        return true;
    }
    
    void Stop() {
        _running = false;
        if (_server_fd >= 0) {
            close(_server_fd);
            _server_fd = -1;
        }
        for (auto& t : _workers) {
            if (t.joinable()) t.join();
        }
        _workers.clear();
    }
    
    void Run() {
        if (!Start()) return;
        // 主线程等待
        std::cout << "[HTTP] Press Ctrl+C to stop" << std::endl;
        while (_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

} // namespace zhouyi

#endif
