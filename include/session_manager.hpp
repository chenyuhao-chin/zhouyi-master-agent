#ifndef __SESSION_MANAGER_HPP__
#define __SESSION_MANAGER_HPP__

#include "database.hpp"
#include "meihua_engine.hpp"
#include "liuyao_engine.hpp"
#include "llm_gateway.hpp"
#include "knowledge_base.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <algorithm>

namespace zhouyi {

// 会话管理器：串联数据库、梅花易数引擎、LLM网关
class SessionManager {
private:
    Database& _db;
    LLMGateway& _llm;
    KnowledgeBase& _kb;
    std::string _default_user_id;

    // 生成唯一ID
    static std::string GenerateId(const std::string& prefix = "s") {
        static int counter = 0;
        std::time_t now = std::time(nullptr);
        std::ostringstream oss;
        oss << prefix << "_" << now << "_" << (++counter);
        return oss.str();
    }

    // 构建梅花易数系统提示词（集成知识库）
    std::string BuildSystemPrompt(const MeihuaResult& gua, const std::string& user_question) {
        std::ostringstream prompt;
        
        // === 角色设定 ===
        prompt << "你是一位精通梅花易数、《易经》、五行八卦的AI大师。"
               << "你能根据卦象信息，结合深厚的易学知识，为用户提供专业、详尽、有启发性的卦象解读。\n\n";
        
        // === 知识库：核心规则 ===
        std::string core_kb = _kb.BuildCoreKnowledgeContext();
        if (!core_kb.empty()) {
            prompt << "# 参考知识库\n\n";
            prompt << core_kb << "\n";
        }
        
        // === 知识库：当前卦象的详细信息 ===
        std::string gua_kb = _kb.BuildGuaSpecificContext(gua.gua_name, gua.bian_gua_name);
        if (!gua_kb.empty()) {
            prompt << "# 当前卦象详细参考\n\n";
            prompt << gua_kb << "\n";
        }
        
        // === 本次卦象信息 ===
        prompt << "# 本次起卦信息\n\n";
        prompt << "## 卦象数据\n";
        prompt << "- **本卦**：" << gua.gua_name;
        {
            auto gua_info = MeihuaEngine::GetGuaInfo(gua.upper_num, gua.lower_num);
            prompt << "（" << gua_info.symbol << "）\n";
            if (!gua_info.judgment.empty()) {
                prompt << "- **卦辞**：" << gua_info.judgment << "\n";
            }
            if (!gua_info.image.empty()) {
                prompt << "- **象辞**：" << gua_info.image << "\n";
            }
        }
        prompt << "- **变卦**：" << gua.bian_gua_name << "\n";
        prompt << "- **动爻**：第" << gua.dong_yao << "爻（";
        // 判断动爻在体卦还是用卦
        if (gua.dong_yao >= 1 && gua.dong_yao <= 6) {
            bool dong_in_upper = (gua.dong_yao > 3);
            if (dong_in_upper) {
                prompt << "动爻在上卦";
            } else {
                prompt << "动爻在下卦";
            }
        }
        prompt << "）\n";
        
        prompt << "- **体卦**：" << MeihuaEngine::GetGuaInfo(gua.ti_num, gua.ti_num).name << "（五行：" << gua.wuxing_ti << "）\n";
        prompt << "- **用卦**：" << MeihuaEngine::GetGuaInfo(gua.yong_num, gua.yong_num).name << "（五行：" << gua.wuxing_yong << "）\n";
        
        // 体用生克关系（详细版）
        std::string relation = GetWuxingRelation(gua.wuxing_ti, gua.wuxing_yong);
        prompt << "- **体用关系**：体（" << gua.wuxing_ti << "）与用（" << gua.wuxing_yong << "）→ " << relation << "\n";
        
        prompt << "\n";
        
        // === 解读要求 ===
        prompt << "# 解读要求\n\n";
        prompt << "请按以下结构进行解读：\n\n";
        prompt << "### 1. 本卦解析\n";
        prompt << "解释本卦的卦象含义、卦辞和象辞所揭示的信息。\n\n";
        prompt << "### 2. 体用关系分析\n";
        prompt << "详细分析体用五行生克关系，解释这对求问者意味着什么。\n";
        prompt << "参考：用生体=大吉（外界助力），体生用=泄气（付出多），"
               << "用克体=不利（阻碍大），体克用=可得（需努力），比和=平稳。\n\n";
        prompt << "### 3. 动爻解读\n";
        prompt << "结合动爻位置（第" << gua.dong_yao << "爻），引用该爻的爻辞进行解读。\n\n";
        prompt << "### 4. 变卦启示\n";
        prompt << "解释变卦代表的最终趋势和结果。\n\n";
        prompt << "### 5. 综合建议\n";
        prompt << "综合以上分析，给出温和、有智慧的建议。语气要有大师风范，避免过于绝对。\n\n";
        
        if (!user_question.empty()) {
            prompt << "# 用户问题\n\n";
            prompt << "「" << user_question << "」\n\n";
            prompt << "请特别围绕用户的这个问题进行解读，使解读具有针对性。\n";
        }
        
        return prompt.str();
    }

    // 构建六爻系统提示词（集成知识库）
    std::string BuildLiuyaoSystemPrompt(const LiuyaoResult& gua, const std::string& user_question) {
        std::ostringstream prompt;
        
        // === 角色设定 ===
        prompt << "你是一位精通六爻预测、《易经》、纳甲六亲的AI大师。\n"
               << "你能根据六爻排盘信息，结合深厚的易学知识，为用户提供专业、详尽、有启发性的卦象解读。\n\n";
        
        // === 知识库：六爻基础知识 ===
        std::string liuyao_kb = _kb.BuildLiuyaoContext();
        if (!liuyao_kb.empty()) {
            prompt << "# 六爻参考知识库\n\n";
            prompt << liuyao_kb << "\n";
        }
        
        // === 核心五行知识 ===
        std::string core_kb = _kb.BuildCoreKnowledgeContext();
        if (!core_kb.empty()) {
            prompt << "# 参考知识库\n\n";
            prompt << core_kb << "\n";
        }
        
        // === 本次排盘信息 ===
        prompt << "# 本次六爻排盘信息\n\n";
        prompt << "## 基本信息\n";
        prompt << "- **起卦时间**：" << gua.gua_time << "\n";
        prompt << "- **年干支**：" << gua.ganzhi_year << "\n";
        prompt << "- **月干支**：" << gua.ganzhi_month << "\n";
        prompt << "- **日干支**：" << gua.ganzhi_day << "\n";
        prompt << "- **时干支**：" << gua.ganzhi_hour << "\n";
        prompt << "- **所属宫**：" << gua.gong_name << "（五行：" << gua.gong_wuxing << "）\n";
        prompt << "- **本卦**：" << gua.ben_gua_name << "\n";
        prompt << "- **变卦**：" << gua.bian_gua_name << "\n\n";
        
        // 六爻排盘表
        prompt << "## 六爻排盘表\n\n";
        prompt << "| 爻位 | 阴阳 | 动爻 | 地支 | 五行 | 六亲 | 六神 | 世应 |\n";
        prompt << "|------|------|------|------|------|------|------|------|\n";
        
        std::string pos_names[] = {"上爻", "五爻", "四爻", "三爻", "二爻", "初爻"};
        for (int i = 5; i >= 0; i--) {
            const auto& y = gua.yao[i];
            prompt << "| " << pos_names[i];
            prompt << " | " << (y.yinyang ? "阳——" : "阴— —");
            prompt << " | " << (y.is_dong ? "○" : " ");
            prompt << " | " << y.dizhi;
            prompt << " | " << y.wuxing;
            prompt << " | " << y.liuqin;
            prompt << " | " << y.liushen;
            if (y.is_shi) prompt << " | 【世】";
            else if (y.is_ying) prompt << " | 【应】";
            else prompt << " | ";
            prompt << " |\n";
        }
        prompt << "\n";
        
        // 动爻列表
        prompt << "**动爻**：";
        bool first_dong = true;
        for (int i = 0; i < 6; i++) {
            if (gua.yao[i].is_dong) {
                if (!first_dong) prompt << "、";
                prompt << "第" << (i + 1) << "爻";
                first_dong = false;
            }
        }
        if (first_dong) prompt << "无动爻（静卦）";
        prompt << "\n\n";
        
        // === 断卦要求 ===
        prompt << "# 断卦要求\n\n";
        prompt << "请按以下结构进行专业六爻断卦：\n\n";
        prompt << "### 1. 卦象总论\n";
        prompt << "介绍本卦和变卦的基本含义，所属宫位。\n\n";
        prompt << "### 2. 用神分析\n";
        prompt << "根据用户问题确定用神（父母、兄弟、子孙、妻财、官鬼），分析用神旺衰。\n\n";
        prompt << "### 3. 六爻逐爻分析\n";
        prompt << "从初爻到上爻逐爻分析，重点关注世爻、应爻、用神所在爻、动爻。\n\n";
        prompt << "### 4. 六神综合判断\n";
        prompt << "结合六神（青龙、朱雀、勾陈、螣蛇、白虎、玄武）分析吉凶。\n\n";
        prompt << "### 5. 动变分析\n";
        prompt << "分析动爻的变化趋势，变卦对本卦的影响。\n\n";
        prompt << "### 6. 综合断语\n";
        prompt << "综合以上分析给出最终断语和建议。语气要有大师风范，引用术语要准确。\n\n";
        
        if (!user_question.empty()) {
            prompt << "# 用户问题\n\n";
            prompt << "「" << user_question << "」\n\n";
            prompt << "请特别围绕用户的这个问题进行六爻断卦分析。\n";
        }
        
        return prompt.str();
    }

    // 五行生克关系
    static std::string GetWuxingRelation(const std::string& ti, const std::string& yong) {
        // 五行相生：木生火，火生土，土生金，金生水，水生木
        // 五行相克：木克土，土克水，水克火，火克金，金克木
        if (ti == yong) return "比和，平和之象";
        
        if ((ti == "木" && yong == "火") || (ti == "火" && yong == "土") ||
            (ti == "土" && yong == "金") || (ti == "金" && yong == "水") || 
            (ti == "水" && yong == "木")) {
            return "用生体，吉";
        }
        if ((yong == "木" && ti == "火") || (yong == "火" && ti == "土") ||
            (yong == "土" && ti == "金") || (yong == "金" && ti == "水") || 
            (yong == "水" && ti == "木")) {
            return "体生用，泄气，需努力";
        }
        if ((ti == "木" && yong == "土") || (ti == "土" && yong == "水") ||
            (ti == "水" && yong == "火") || (ti == "火" && yong == "金") || 
            (ti == "金" && yong == "木")) {
            return "体克用，可得，但费力";
        }
        if ((yong == "木" && ti == "土") || (yong == "土" && ti == "水") ||
            (yong == "水" && ti == "火") || (yong == "火" && ti == "金") || 
            (yong == "金" && ti == "木")) {
            return "用克体，不利，需谨慎";
        }
        return "关系复杂";
    }

public:
    SessionManager(Database& db, LLMGateway& llm, KnowledgeBase& kb) : _db(db), _llm(llm), _kb(kb) {
        _default_user_id = "default_user";
        // 确保默认用户存在
        _db.CreateUser(_default_user_id, "default", "默认用户");
    }

    // 创建新会话
    std::string CreateSession(const std::string& title = "新对话") {
        std::string session_id = GenerateId("sess");
        _db.CreateSession(session_id, _default_user_id, title);
        return session_id;
    }

    // 获取会话列表
    std::vector<Database::SessionInfo> GetSessions() {
        return _db.GetUserSessions(_default_user_id);
    }

    // 获取会话历史消息
    std::vector<Database::MessageInfo> GetMessages(const std::string& session_id) {
        return _db.GetSessionMessages(session_id);
    }

    // 删除会话
    bool DeleteSession(const std::string& session_id) {
        return _db.DeleteSession(session_id);
    }

    // 核心功能：梅花易数起卦 + LLM解读
    // mode: "random" | "number" | "text"
    std::string Divinate(const std::string& session_id, 
                         const std::string& user_input,
                         const std::string& mode = "random",
                         int num1 = 0, int num2 = 0, int num3 = 0) {
        // 1. 起卦
        MeihuaResult gua;
        if (mode == "number" && num1 > 0 && num2 > 0) {
            gua = MeihuaEngine::NumberGua(num1, num2, num3);
        } else if (mode == "text" && !user_input.empty()) {
            gua = MeihuaEngine::TextGua(user_input);
        } else {
            gua = MeihuaEngine::RandomGua();
        }
        
        // 保存用户消息
        _db.AddMessage(session_id, "user", user_input, gua.gua_json);
        
        // 2. 构建LLM请求
        // 获取历史消息构建上下文
        auto history = _db.GetSessionMessages(session_id, 10);
        
        std::vector<ChatMessage> messages;
        
        // 系统提示词
        ChatMessage sys_msg;
        sys_msg.role = "system";
        sys_msg.content = BuildSystemPrompt(gua, user_input);
        messages.push_back(sys_msg);
        
        // 添加历史对话（排除刚保存的用户消息）
        for (size_t i = 0; i < history.size() && i < 9; i++) {
            ChatMessage msg;
            msg.role = history[i].role;
            msg.content = history[i].content;
            messages.push_back(msg);
        }
        
        // 3. 调用LLM
        std::string reply;
        try {
            reply = _llm.Chat(messages);
        } catch (const std::exception& e) {
            reply = "⚠️ LLM调用失败：" + std::string(e.what()) + "\n\n";
            reply += "以下是卦象基本信息（本地解读）：\n\n";
            reply += "【" + gua.gua_name + "】→ 【" + gua.bian_gua_name + "】\n";
            reply += "动爻：第" + std::to_string(gua.dong_yao) + "爻\n";
            reply += "体卦：" + MeihuaEngine::GetGuaInfo(gua.ti_num, gua.ti_num).name + "（" + gua.wuxing_ti + "）\n";
            reply += "用卦：" + MeihuaEngine::GetGuaInfo(gua.yong_num, gua.yong_num).name + "（" + gua.wuxing_yong + "）\n";
            reply += "体用关系：" + GetWuxingRelation(gua.wuxing_ti, gua.wuxing_yong) + "\n";
            
            auto gua_info = MeihuaEngine::GetGuaInfo(gua.upper_num, gua.lower_num);
            reply += "卦辞：" + gua_info.judgment + "\n";
            reply += "象辞：" + gua_info.image + "\n";
        }
        
        // 4. 保存assistant回复
        _db.AddMessage(session_id, "assistant", reply);
        
        // 5. 更新会话的最新卦象
        _db.UpdateSessionGua(session_id, gua.gua_json);
        
        // 6. 返回结果JSON
        std::ostringstream result;
        result << "{";
        result << "\"gua\":" << gua.gua_json << ",";
        result << "\"reply\":\"" << JsonBuilder::Escape(reply) << "\",";
        result << "\"session_id\":\"" << session_id << "\"";
        result << "}";
        return result.str();
    }

    // 核心功能：六爻排卦 + LLM解读
    std::string LiuyaoDivinate(const std::string& session_id,
                                const std::string& user_input,
                                const std::string& mode = "random") {
        // 1. 排卦
        LiuyaoResult gua;
        if (mode == "random") {
            gua = LiuyaoEngine::RandomGua();
        } else {
            gua = LiuyaoEngine::RandomGua(); // 默认随机
        }
        
        // 保存用户消息
        _db.AddMessage(session_id, "user", user_input, gua.gua_json);
        
        // 2. 构建LLM请求
        auto history = _db.GetSessionMessages(session_id, 10);
        std::vector<ChatMessage> messages;
        
        // 系统提示词 - 六爻专用
        ChatMessage sys_msg;
        sys_msg.role = "system";
        sys_msg.content = BuildLiuyaoSystemPrompt(gua, user_input);
        messages.push_back(sys_msg);
        
        // 添加历史对话
        for (size_t i = 0; i < history.size() && i < 9; i++) {
            ChatMessage msg;
            msg.role = history[i].role;
            msg.content = history[i].content;
            messages.push_back(msg);
        }
        
        // 3. 调用LLM
        std::string reply;
        try {
            reply = _llm.Chat(messages);
        } catch (const std::exception& e) {
            reply = "⚠️ LLM调用失败：" + std::string(e.what()) + "\n\n";
            reply += "以下是六爻排盘基本信息（本地解读）：\n\n";
            reply += "【" + gua.ben_gua_name + "】→ 【" + gua.bian_gua_name + "】\n";
            reply += "所属：" + gua.gong_name + "（" + gua.gong_wuxing + "）\n";
            reply += "起卦时间：" + gua.gua_time + "\n\n";
            for (int i = 0; i < 6; i++) {
                const auto& y = gua.yao[i];
                std::string pos_name[] = {"初爻","二爻","三爻","四爻","五爻","上爻"};
                reply += pos_name[i] + "：";
                reply += y.yinyang ? "阳" : "阴";
                if (y.is_dong) reply += "（动）";
                reply += " " + y.dizhi + y.wuxing;
                reply += " " + y.liuqin;
                reply += " " + y.liushen;
                if (y.is_shi) reply += " 【世】";
                if (y.is_ying) reply += " 【应】";
                reply += "\n";
            }
        }
        
        // 4. 保存assistant回复
        _db.AddMessage(session_id, "assistant", reply);
        
        // 5. 更新会话的最新卦象
        _db.UpdateSessionGua(session_id, gua.gua_json);
        
        // 6. 返回结果JSON
        std::ostringstream result;
        result << "{";
        result << "\"gua\":" << gua.gua_json << ",";
        result << "\"reply\":\"" << JsonBuilder::Escape(reply) << "\",";
        result << "\"session_id\":\"" << session_id << "\",";
        result << "\"mode\":\"liuyao\"";
        result << "}";
        return result.str();
    }

    // 普通对话（不排卦，继续已有对话）
    std::string Chat(const std::string& session_id, const std::string& user_input) {
        // 保存用户消息
        _db.AddMessage(session_id, "user", user_input);
        
        // 获取历史消息
        auto history = _db.GetSessionMessages(session_id, 10);
        
        std::vector<ChatMessage> messages;
        
        // 系统提示词（带知识库上下文）
        ChatMessage sys_msg;
        sys_msg.role = "system";
        {
            std::ostringstream sp;
            sp << "你是一位精通梅花易数、《易经》、五行八卦的AI大师。\n\n";
            std::string core_kb = _kb.BuildCoreKnowledgeContext();
            if (!core_kb.empty()) {
                sp << "# 参考知识库\n\n" << core_kb << "\n";
            }
            sp << "你正在与用户进行对话，请根据之前的卦象和对话上下文，继续为用户解答问题。"
               << "语气要温和、有智慧感，引用易学术语要准确。";
            sys_msg.content = sp.str();
        }
        messages.push_back(sys_msg);
        
        // 添加历史对话
        for (const auto& m : history) {
            ChatMessage msg;
            msg.role = m.role;
            msg.content = m.content;
            messages.push_back(msg);
        }
        
        // 调用LLM
        std::string reply;
        try {
            reply = _llm.Chat(messages);
        } catch (const std::exception& e) {
            reply = "⚠️ LLM调用失败：" + std::string(e.what());
        }
        
        // 保存assistant回复
        _db.AddMessage(session_id, "assistant", reply);
        
        // 返回
        std::ostringstream result;
        result << "{";
        result << "\"reply\":\"" << JsonBuilder::Escape(reply) << "\",";
        result << "\"session_id\":\"" << session_id << "\"";
        result << "}";
        return result.str();
    }
};

} // namespace zhouyi

#endif
