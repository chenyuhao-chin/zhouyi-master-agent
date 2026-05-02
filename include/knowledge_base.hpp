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
    std::map<std::string, std::string> _extra_data; // 动态加载的额外数据文件

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
            "liuyao_basics.json", "liuyao_najia.json"
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
        
        std::cout << "[KB] Knowledge base loaded: " << loaded << " core files + " 
                  << _extra_data.size() << " extra files" << std::endl;
        return loaded > 0;
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
