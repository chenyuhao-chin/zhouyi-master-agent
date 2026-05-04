#ifndef __KNOWLEDGE_BASE_HPP__
#define __KNOWLEDGE_BASE_HPP__

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <dirent.h>
#include <algorithm>
#include <sys/stat.h>

namespace zhouyi {

// 知识库加载器：在启动时加载所有JSON知识文件，
// 在构建System Prompt时提供丰富的易学知识上下文
class KnowledgeBase {
private:
    std::string _bagua_raw;          // 八卦万物类象原始JSON
    std::string _wuxing_raw;         // 五行生克原始JSON
    std::string _meihua_rules_raw;   // 梅花易数规则原始JSON
    std::string _gua64_raw;          // 六十四卦详解原始JSON
    std::string _liuyao_basics_raw;  // 六爻基础知识
    std::string _liuyao_najia_raw;   // 纳甲规则
    std::string _dizhi_relations_raw; // 地支关系（六合、六冲、三合、三刑）
    std::string _liuyao_advanced_raw; // 六爻进阶规则（世应、六亲、神煞）
    std::string _gua64_yaos_raw;     // 六十四卦爻辞原文
    std::map<std::string, std::string> _extra_data; // 动态加载的额外数据文件

    // --- LobeHub 格式知识库 ---
    std::string _agent_config_raw;   // 智能体主配置 JSON
    std::string _agent_system_role;  // 从配置中提取的 systemRole
    std::string _agent_opening_msg;  // 开场消息
    std::vector<std::string> _agent_opening_questions; // 开场问题
    std::map<std::string, std::string> _knowledge_docs; // Markdown 知识文档 key=filename, value=content

    static bool IsDirectory(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        return S_ISDIR(st.st_mode);
    }

    // 从JSON字符串中提取简单字符串字段值
    static std::string ExtractJsonString(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        // 找到冒号
        size_t colon = json.find(':', pos + search.size());
        if (colon == std::string::npos) return "";
        // 找到引号开始
        size_t q1 = json.find('"', colon + 1);
        if (q1 == std::string::npos) return "";
        size_t q2 = q1 + 1;
        // 找到匹配的结束引号（处理转义）
        while (q2 < json.size()) {
            if (json[q2] == '\\') { q2 += 2; continue; }
            if (json[q2] == '"') break;
            q2++;
        }
        if (q2 >= json.size()) return "";
        std::string val = json.substr(q1 + 1, q2 - q1 - 1);
        // 处理换行转义
        std::string result;
        for (size_t i = 0; i < val.size(); i++) {
            if (val[i] == '\\' && i + 1 < val.size()) {
                if (val[i+1] == 'n') { result += '\n'; i++; }
                else if (val[i+1] == 't') { result += '\t'; i++; }
                else if (val[i+1] == '"') { result += '"'; i++; }
                else if (val[i+1] == '\\') { result += '\\'; i++; }
                else { result += val[i]; }
            } else {
                result += val[i];
            }
        }
        return result;
    }

    static std::string ReadFileContent(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[KB] Warning: cannot open " << path << std::endl;
            return "";
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

public:
    KnowledgeBase() = default;

    // 加载所有知识文件
    bool LoadAll(const std::string& data_dir = "data") {
        // 1. 加载核心文件
        _bagua_raw = ReadFileContent(data_dir + "/bagua.json");
        _wuxing_raw = ReadFileContent(data_dir + "/wuxing.json");
        _meihua_rules_raw = ReadFileContent(data_dir + "/meihua_rules.json");
        _liuyao_basics_raw = ReadFileContent(data_dir + "/liuyao_basics.json");
        _liuyao_najia_raw = ReadFileContent(data_dir + "/liuyao_najia.json");
        _dizhi_relations_raw = ReadFileContent(data_dir + "/dizhi_relations.json");
        _liuyao_advanced_raw = ReadFileContent(data_dir + "/liuyao_advanced.json");
        _gua64_yaos_raw = ReadFileContent(data_dir + "/gua64_yaos.json");
        
        std::string part1 = ReadFileContent(data_dir + "/gua64_part1.json");
        std::string part2 = ReadFileContent(data_dir + "/gua64_part2.json");
        if (!part1.empty() && !part2.empty()) {
            _gua64_raw = part1;
            if (part2.size() > 50) _gua64_raw += "\n" + part2;
        } else if (!part1.empty()) {
            _gua64_raw = part1;
        } else if (!part2.empty()) {
            _gua64_raw = part2;
        }
        
        // 2. 动态扫描 data/ 目录下所有 .json 文件（已加载的跳过）
        std::vector<std::string> known_files = {
            "bagua.json", "wuxing.json", "meihua_rules.json",
            "gua64_part1.json", "gua64_part2.json",
            "liuyao_basics.json", "liuyao_najia.json",
            "dizhi_relations.json", "liuyao_advanced.json", "gua64_yaos.json"
        };
        
        DIR* dir = opendir(data_dir.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string filename(entry->d_name);
                if (filename.size() > 5 && filename.substr(filename.size() - 5) == ".json") {
                    // 检查是否已加载
                    bool already_loaded = false;
                    for (const auto& kf : known_files) {
                        if (filename == kf) { already_loaded = true; break; }
                    }
                    if (!already_loaded) {
                        std::string content = ReadFileContent(data_dir + "/" + filename);
                        if (!content.empty()) {
                            _extra_data[filename] = content;
                            std::cout << "[KB] Auto-loaded " << filename << " (" << content.size() << " bytes)" << std::endl;
                        }
                    }
                }
            }
            closedir(dir);
        }
        
        // 3. 统计
        int loaded = 0;
        if (!_bagua_raw.empty()) { std::cout << "[KB] bagua.json (" << _bagua_raw.size() << " bytes)" << std::endl; loaded++; }
        if (!_wuxing_raw.empty()) { std::cout << "[KB] wuxing.json (" << _wuxing_raw.size() << " bytes)" << std::endl; loaded++; }
        if (!_meihua_rules_raw.empty()) { std::cout << "[KB] meihua_rules.json (" << _meihua_rules_raw.size() << " bytes)" << std::endl; loaded++; }
        if (!_gua64_raw.empty()) { std::cout << "[KB] gua64 files (" << _gua64_raw.size() << " bytes)" << std::endl; loaded++; }
        if (!_liuyao_basics_raw.empty()) { std::cout << "[KB] liuyao_basics.json (" << _liuyao_basics_raw.size() << " bytes)" << std::endl; loaded++; }
        if (!_liuyao_najia_raw.empty()) { std::cout << "[KB] liuyao_najia.json (" << _liuyao_najia_raw.size() << " bytes)" << std::endl; loaded++; }
        if (!_dizhi_relations_raw.empty()) { std::cout << "[KB] dizhi_relations.json (" << _dizhi_relations_raw.size() << " bytes)" << std::endl; loaded++; }
        if (!_liuyao_advanced_raw.empty()) { std::cout << "[KB] liuyao_advanced.json (" << _liuyao_advanced_raw.size() << " bytes)" << std::endl; loaded++; }
        if (!_gua64_yaos_raw.empty()) { std::cout << "[KB] gua64_yaos.json (" << _gua64_yaos_raw.size() << " bytes)" << std::endl; loaded++; }
        
        std::cout << "[KB] Knowledge base loaded: " << loaded << " core files + " 
                  << _extra_data.size() << " extra files" << std::endl;
        return loaded > 0;
    }

    // 加载 LobeHub 格式的知识库
    // source_dir: knowledge_sources 目录路径
    bool LoadLobeHubKnowledge(const std::string& source_dir = "knowledge_sources") {
        // 1. 加载智能体主配置
        std::string config_path = source_dir + "/zhouyi-master.json";
        _agent_config_raw = ReadFileContent(config_path);
        if (!_agent_config_raw.empty()) {
            // systemRole 可能在顶层或嵌套在 config 子对象中
            _agent_system_role = ExtractJsonString(_agent_config_raw, "systemRole");
            if (_agent_system_role.empty()) {
                // 尝试从 "config": { "systemRole": "..." } 中提取
                size_t config_pos = _agent_config_raw.find("\"config\"");
                if (config_pos != std::string::npos) {
                    std::string config_sub = _agent_config_raw.substr(config_pos);
                    _agent_system_role = ExtractJsonString(config_sub, "systemRole");
                }
            }
            _agent_opening_msg = ExtractJsonString(_agent_config_raw, "openingMessage");
            if (_agent_opening_msg.empty()) {
                size_t config_pos = _agent_config_raw.find("\"config\"");
                if (config_pos != std::string::npos) {
                    std::string config_sub = _agent_config_raw.substr(config_pos);
                    _agent_opening_msg = ExtractJsonString(config_sub, "openingMessage");
                }
            }
            if (!_agent_system_role.empty()) {
                std::cout << "[KB-LobeHub] Agent systemRole loaded (" << _agent_system_role.size() << " chars)" << std::endl;
            }
            std::cout << "[KB-LobeHub] Agent config loaded (" << _agent_config_raw.size() << " bytes)" << std::endl;
        }

        // 2. 扫描 docs/ 目录加载 Markdown 知识文档
        std::string docs_dir = source_dir + "/docs";
        if (IsDirectory(docs_dir)) {
            DIR* dir = opendir(docs_dir.c_str());
            if (dir) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr) {
                    std::string filename(entry->d_name);
                    if (filename.size() > 3 && filename.substr(filename.size() - 3) == ".md") {
                        std::string content = ReadFileContent(docs_dir + "/" + filename);
                        if (!content.empty()) {
                            _knowledge_docs[filename] = content;
                            std::cout << "[KB-LobeHub] Doc loaded: " << filename 
                                      << " (" << content.size() << " bytes)" << std::endl;
                        }
                    }
                }
                closedir(dir);
            }
        }

        std::cout << "[KB-LobeHub] Loaded " << _knowledge_docs.size() << " knowledge docs" << std::endl;
        return !_agent_config_raw.empty() || !_knowledge_docs.empty();
    }

    // 获取智能体 systemRole
    std::string GetAgentSystemRole() const { return _agent_system_role; }
    
    // 获取智能体配置原始JSON
    std::string GetAgentConfigRaw() const { return _agent_config_raw; }

    // 获取所有知识文档内容拼接
    std::string GetAllKnowledgeDocs() const {
        if (_knowledge_docs.empty()) return "";
        std::ostringstream ss;
        for (const auto& pair : _knowledge_docs) {
            ss << pair.second << "\n\n---\n\n";
        }
        return ss.str();
    }

    // 获取知识文档列表
    std::vector<std::string> GetKnowledgeDocNames() const {
        std::vector<std::string> names;
        for (const auto& pair : _knowledge_docs) {
            names.push_back(pair.first);
        }
        return names;
    }

    // 根据关键词检索相关知识文档（简易 RAG：关键词匹配）
    std::string RetrieveRelevantDocs(const std::string& query, int max_docs = 3) const {
        if (_knowledge_docs.empty()) return "";
        
        // 正确提取 UTF-8 字符序列作为关键词
        std::vector<std::string> keywords;
        std::string word;
        size_t ci = 0;
        while (ci < query.size()) {
            unsigned char c = query[ci];
            // 检测分隔符
            bool is_separator = false;
            if (c == ' ' || c == ',' || c == '?' || c == '!' || c == '。' || c == '、') {
                is_separator = true;
            } else if (c == 0xEF && ci + 2 < query.size()) {
                // 中文标点：？= EF BC 9F, ！= EF BC 81, ，= EF BC 8C
                unsigned char c2 = query[ci+1];
                unsigned char c3 = query[ci+2];
                if (c2 == 0xBC && (c3 == 0x9F || c3 == 0x81 || c3 == 0x8C)) {
                    is_separator = true;
                }
            }
            if (is_separator) {
                if (!word.empty()) { keywords.push_back(word); word.clear(); }
                ci++;
            } else if (c < 0x80) {
                // ASCII 字符
                word += c;
                ci++;
            } else if (c >= 0xC0) {
                // UTF-8 多字节字符：提取完整序列
                int extra = 0;
                if ((c & 0xE0) == 0xC0) extra = 1;      // 2-byte
                else if ((c & 0xF0) == 0xE0) extra = 2;  // 3-byte (中文)
                else if ((c & 0xF8) == 0xF0) extra = 3;  // 4-byte
                std::string utf8_char = query.substr(ci, 1 + extra);
                word += utf8_char;
                ci += 1 + extra;
            } else {
                // 续字节（不应单独出现），跳过
                ci++;
            }
        }
        if (!word.empty()) keywords.push_back(word);
        
        // 为每个文档计算相关性分数
        std::vector<std::pair<int, std::string>> scored_docs;
        for (const auto& pair : _knowledge_docs) {
            int score = 0;
            for (const auto& kw : keywords) {
                if (kw.size() < 2) continue; // 跳过太短的关键词
                size_t pos = 0;
                while ((pos = pair.second.find(kw, pos)) != std::string::npos) {
                    score++;
                    pos += kw.size();
                }
                // 标题中的关键词权重更高
                size_t title_pos = pair.second.find(kw, 0);
                if (title_pos != std::string::npos && title_pos < 200) score += 3;
            }
            if (score > 0) {
                scored_docs.push_back({score, pair.first});
            }
        }
        
        // 按分数排序
        std::sort(scored_docs.begin(), scored_docs.end(), 
                  [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // 取前 max_docs 个
        std::ostringstream result;
        int count = 0;
        for (const auto& sd : scored_docs) {
            if (count >= max_docs) break;
            auto it = _knowledge_docs.find(sd.second);
            if (it != _knowledge_docs.end()) {
                result << "### [相关知识] " << sd.second << " (相关度:" << sd.first << ")\n\n";
                result << it->second << "\n\n---\n\n";
                count++;
            }
        }
        
        // 如果没有匹配到，返回基础知识摘要
        if (count == 0) {
            return BuildCoreKnowledgeContext();
        }
        
        return result.str();
    }

    // 获取八卦万物类象（精简版）
    std::string GetBaguaKnowledge() const {
        if (_bagua_raw.empty()) return "";
        // 返回原始JSON内容（由LLM自行理解）
        return _bagua_raw;
    }

    // 获取五行知识
    std::string GetWuxingKnowledge() const {
        if (_wuxing_raw.empty()) return "";
        return _wuxing_raw;
    }

    // 获取梅花易数规则
    std::string GetMeihuaRules() const {
        if (_meihua_rules_raw.empty()) return "";
        return _meihua_rules_raw;
    }

    // 获取六十四卦知识
    std::string GetGua64Knowledge() const {
        if (_gua64_raw.empty()) return "";
        return _gua64_raw;
    }

    // 获取六爻基础知识
    std::string GetLiuyaoBasics() const {
        if (_liuyao_basics_raw.empty()) return "";
        return _liuyao_basics_raw;
    }

    // 获取纳甲规则
    std::string GetLiuyaoNajia() const {
        if (_liuyao_najia_raw.empty()) return "";
        return _liuyao_najia_raw;
    }

    // 获取地支关系知识
    std::string GetDizhiRelations() const {
        if (_dizhi_relations_raw.empty()) return "";
        return _dizhi_relations_raw;
    }

    // 获取六爻进阶规则（世应、六亲、神煞）
    std::string GetLiuyaoAdvanced() const {
        if (_liuyao_advanced_raw.empty()) return "";
        return _liuyao_advanced_raw;
    }

    // 获取六十四卦爻辞原文
    std::string GetGua64Yaos() const {
        if (_gua64_yaos_raw.empty()) return "";
        return _gua64_yaos_raw;
    }

    // 获取动态加载的额外数据
    std::string GetExtraData(const std::string& filename) const {
        auto it = _extra_data.find(filename);
        return it != _extra_data.end() ? it->second : "";
    }

    // 获取所有额外数据文件名列表
    std::vector<std::string> GetExtraDataFiles() const {
        std::vector<std::string> files;
        for (const auto& pair : _extra_data) {
            files.push_back(pair.first);
        }
        return files;
    }

    // 获取全部动态数据的摘要（用于System Prompt）
    std::string GetExtraDataSummary() const {
        if (_extra_data.empty()) return "";
        std::ostringstream ss;
        ss << "## 补充知识库（共" << _extra_data.size() << "个文件）\n\n";
        for (const auto& pair : _extra_data) {
            ss << "### " << pair.first << "\n";
            ss << pair.second << "\n\n";
        }
        return ss.str();
    }

    // 获取某个卦的详细信息（从JSON中提取）
    std::string GetGuaDetail(const std::string& gua_name) const {
        if (_gua64_raw.empty()) return "";
        
        // 在JSON中查找该卦名
        std::string search = "\"name\": \"" + gua_name + "\"";
        size_t pos = _gua64_raw.find(search);
        if (pos == std::string::npos) return "";
        
        // 向前找到这个卦对象的开始 {
        size_t start = _gua64_raw.rfind('{', pos);
        if (start == std::string::npos) return "";
        
        // 向后找到匹配的 }  (简单计数)
        int depth = 0;
        size_t end = start;
        for (size_t i = start; i < _gua64_raw.size(); i++) {
            if (_gua64_raw[i] == '{') depth++;
            else if (_gua64_raw[i] == '}') {
                depth--;
                if (depth == 0) { end = i + 1; break; }
            }
        }
        
        if (end > start) {
            return _gua64_raw.substr(start, end - start);
        }
        return "";
    }

    // 构建精简的通用知识上下文（用于System Prompt）
    // 注意：不能把全部知识塞入prompt，要精简
    std::string BuildCoreKnowledgeContext() const {
        std::ostringstream ctx;
        
        // 1. 梅花易数核心规则（精简）
        ctx << "## 梅花易数核心规则\n";
        if (!_meihua_rules_raw.empty()) {
            ctx << "- 体用分辨：动爻所在卦为用卦，不动之卦为体卦。体卦代表求占者，用卦代表所问之事。\n";
            ctx << "- 用生体：大吉，外界有利于自己\n";
            ctx << "- 体生用：小凶（泄气），付出多收获少\n";
            ctx << "- 用克体：大凶，障碍多，事情难成\n";
            ctx << "- 体克用：小吉，可以得到但需努力\n";
            ctx << "- 比和：平吉，事情平稳\n\n";
        }
        
        // 2. 八卦基本象征（精简）
        ctx << "## 八卦基本象征\n";
        ctx << "- 乾（天）：刚健、君、父、领导、西北\n";
        ctx << "- 坤（地）：柔顺、母、众、田野、西南\n";
        ctx << "- 震（雷）：动、长男、东方、起动\n";
        ctx << "- 巽（风）：入、长女、东南、柔和\n";
        ctx << "- 坎（水）：险、中男、北方、智慧\n";
        ctx << "- 离（火）：明、中女、南方、文化\n";
        ctx << "- 艮（山）：止、少男、东北、守静\n";
        ctx << "- 兑（泽）：悦、少女、西方、口舌\n\n";
        
        // 3. 五行生克（精简）
        ctx << "## 五行生克关系\n";
        ctx << "- 相生：木生火、火生土、土生金、金生水、水生木\n";
        ctx << "- 相克：木克土、土克水、水克火、火克金、金克木\n\n";
        
        // 4. 动态加载的知识库内容
        if (!_extra_data.empty()) {
            ctx << "## 补充知识库\n";
            for (const auto& pair : _extra_data) {
                ctx << pair.second << "\n\n";
            }
        }
        
        return ctx.str();
    }

    // 构建六爻知识上下文（用于六爻断卦System Prompt）
    std::string BuildLiuyaoContext() const {
        std::ostringstream ctx;
        
        ctx << "## 六爻预测基础知识\n\n";
        
        if (!_liuyao_basics_raw.empty()) {
            ctx << _liuyao_basics_raw << "\n\n";
        }
        
        if (!_liuyao_najia_raw.empty()) {
            ctx << "## 纳甲规则\n\n";
            ctx << _liuyao_najia_raw << "\n\n";
        }
        
        if (!_dizhi_relations_raw.empty()) {
            ctx << "## 地支关系（六合、六冲、三合、三刑）\n\n";
            ctx << _dizhi_relations_raw << "\n\n";
        }
        
        if (!_liuyao_advanced_raw.empty()) {
            ctx << "## 六爻进阶规则（世应、六亲、神煞）\n\n";
            ctx << _liuyao_advanced_raw << "\n\n";
        }
        
        if (!_gua64_yaos_raw.empty()) {
            ctx << "## 六十四卦爻辞原文\n\n";
            ctx << _gua64_yaos_raw << "\n\n";
        }
        
        return ctx.str();
    }

    // 构建针对特定卦象的详细知识上下文
    std::string BuildGuaSpecificContext(const std::string& gua_name, 
                                         const std::string& bian_gua_name) const {
        std::ostringstream ctx;
        
        // 查找本卦详细信息
        std::string ben_detail = GetGuaDetail(gua_name);
        if (!ben_detail.empty()) {
            ctx << "### 本卦「" << gua_name << "」详细信息\n" << ben_detail << "\n\n";
        }
        
        // 查找变卦详细信息
        std::string bian_detail = GetGuaDetail(bian_gua_name);
        if (!bian_detail.empty()) {
            ctx << "### 变卦「" << bian_gua_name << "」详细信息\n" << bian_detail << "\n\n";
        }
        
        return ctx.str();
    }
};

} // namespace zhouyi

#endif
