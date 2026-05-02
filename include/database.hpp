#ifndef __DATABASE_HPP__
#define __DATABASE_HPP__

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <sqlite3.h>
#include <iostream>

namespace zhouyi {

class Database {
private:
    sqlite3* _db;
    std::mutex _mutex;
    std::string _db_path;

    bool ExecSQL(const std::string& sql) {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "[DB ERROR] " << (errMsg ? errMsg : "unknown") << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }

public:
    Database() : _db(nullptr) {}
    ~Database() { Close(); }

    bool Open(const std::string& path) {
        _db_path = path;
        std::lock_guard<std::mutex> lock(_mutex);
        int rc = sqlite3_open(path.c_str(), &_db);
        if (rc != SQLITE_OK) {
            std::cerr << "[DB] Cannot open database: " << sqlite3_errmsg(_db) << std::endl;
            return false;
        }
        // Enable WAL mode for better concurrent performance
        ExecSQL("PRAGMA journal_mode=WAL;");
        ExecSQL("PRAGMA foreign_keys=ON;");
        return true;
    }

    void Close() {
        if (_db) {
            sqlite3_close(_db);
            _db = nullptr;
        }
    }

    bool InitTables() {
        std::lock_guard<std::mutex> lock(_mutex);
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS users (
                id TEXT PRIMARY KEY,
                username TEXT UNIQUE,
                nickname TEXT,
                created_at INTEGER DEFAULT (strftime('%s','now'))
            );

            CREATE TABLE IF NOT EXISTS sessions (
                id TEXT PRIMARY KEY,
                user_id TEXT,
                title TEXT DEFAULT '新对话',
                last_gua_json TEXT,
                created_at INTEGER DEFAULT (strftime('%s','now')),
                updated_at INTEGER DEFAULT (strftime('%s','now')),
                FOREIGN KEY (user_id) REFERENCES users(id)
            );

            CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                session_id TEXT,
                role TEXT NOT NULL,
                content TEXT NOT NULL,
                gua_json TEXT,
                created_at INTEGER DEFAULT (strftime('%s','now')),
                FOREIGN KEY (session_id) REFERENCES sessions(id) ON DELETE CASCADE
            );
        )";
        return ExecSQL(sql);
    }

    // User operations
    bool CreateUser(const std::string& id, const std::string& username, const std::string& nickname) {
        std::lock_guard<std::mutex> lock(_mutex);
        sqlite3_stmt* stmt;
        const char* sql = "INSERT OR IGNORE INTO users (id, username, nickname) VALUES (?, ?, ?);";
        if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, nickname.c_str(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }

    // Session operations
    bool CreateSession(const std::string& session_id, const std::string& user_id, const std::string& title) {
        std::lock_guard<std::mutex> lock(_mutex);
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO sessions (id, user_id, title) VALUES (?, ?, ?);";
        if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, title.c_str(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }

    bool UpdateSessionGua(const std::string& session_id, const std::string& gua_json) {
        std::lock_guard<std::mutex> lock(_mutex);
        sqlite3_stmt* stmt;
        const char* sql = "UPDATE sessions SET last_gua_json=?, updated_at=strftime('%s','now') WHERE id=?;";
        if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, gua_json.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, session_id.c_str(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }

    struct SessionInfo {
        std::string id;
        std::string user_id;
        std::string title;
        std::string last_gua_json;
        int64_t created_at;
        int64_t updated_at;
    };

    std::vector<SessionInfo> GetUserSessions(const std::string& user_id, int limit = 50) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<SessionInfo> sessions;
        sqlite3_stmt* stmt;
        const char* sql = "SELECT id, user_id, title, last_gua_json, created_at, updated_at "
                          "FROM sessions WHERE user_id=? ORDER BY updated_at DESC LIMIT ?;";
        if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return sessions;
        sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, limit);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            SessionInfo s;
            s.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            s.user_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            s.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* gua = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            s.last_gua_json = gua ? gua : "";
            s.created_at = sqlite3_column_int64(stmt, 4);
            s.updated_at = sqlite3_column_int64(stmt, 5);
            sessions.push_back(s);
        }
        sqlite3_finalize(stmt);
        return sessions;
    }

    bool DeleteSession(const std::string& session_id) {
        std::lock_guard<std::mutex> lock(_mutex);
        sqlite3_stmt* stmt;
        // Delete messages first (though CASCADE should handle it)
        const char* sql1 = "DELETE FROM messages WHERE session_id=?;";
        if (sqlite3_prepare_v2(_db, sql1, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        const char* sql2 = "DELETE FROM sessions WHERE id=?;";
        if (sqlite3_prepare_v2(_db, sql2, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }

    // Message operations
    bool AddMessage(const std::string& session_id, const std::string& role, 
                    const std::string& content, const std::string& gua_json = "") {
        std::lock_guard<std::mutex> lock(_mutex);
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO messages (session_id, role, content, gua_json) VALUES (?, ?, ?, ?);";
        if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, role.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
        if (gua_json.empty()) {
            sqlite3_bind_null(stmt, 4);
        } else {
            sqlite3_bind_text(stmt, 4, gua_json.c_str(), -1, SQLITE_TRANSIENT);
        }
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }

    struct MessageInfo {
        int64_t id;
        std::string session_id;
        std::string role;
        std::string content;
        std::string gua_json;
        int64_t created_at;
    };

    std::vector<MessageInfo> GetSessionMessages(const std::string& session_id, int limit = 20) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<MessageInfo> messages;
        sqlite3_stmt* stmt;
        const char* sql = "SELECT id, session_id, role, content, gua_json, created_at "
                          "FROM messages WHERE session_id=? ORDER BY id DESC LIMIT ?;";
        if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return messages;
        sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, limit);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            MessageInfo m;
            m.id = sqlite3_column_int64(stmt, 0);
            m.session_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            m.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            m.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            const char* gua = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            m.gua_json = gua ? gua : "";
            m.created_at = sqlite3_column_int64(stmt, 5);
            messages.push_back(m);
        }
        sqlite3_finalize(stmt);
        // Reverse so oldest is first
        std::reverse(messages.begin(), messages.end());
        return messages;
    }
};

} // namespace zhouyi

#endif
