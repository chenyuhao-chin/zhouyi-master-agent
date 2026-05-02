#ifndef __LIUYAO_ENGINE_HPP__
#define __LIUYAO_ENGINE_HPP__

#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <map>
#include <array>

namespace zhouyi {

// ==================== 六爻排卦结果 ====================
struct LiuyaoYao {
    int position;           // 爻位 1-6（1=初爻，6=上爻）
    int coin_result;        // 掷铜钱结果：6=老阴, 7=少阳, 8=少阴, 9=老阳
    int yinyang;            // 阴=0, 阳=1
    bool is_dong;           // 是否为动爻
    int bian_yinyang;       // 变后阴阳
    std::string dizhi;      // 地支
    std::string wuxing;     // 地支五行
    std::string liuqin;     // 六亲
    std::string liushen;    // 六神
    bool is_shi;            // 是否为世爻
    bool is_ying;           // 是否为应爻
};

struct LiuyaoResult {
    // 本卦信息
    std::string ben_gua_name;       // 本卦名
    int ben_upper;                   // 本卦上卦八卦编号
    int ben_lower;                   // 本卦下卦八卦编号
    std::string gong_name;          // 所属宫名
    std::string gong_wuxing;        // 宫五行
    
    // 变卦信息
    std::string bian_gua_name;      // 变卦名
    int bian_upper;
    int bian_lower;
    
    // 六爻数据
    std::array<LiuyaoYao, 6> yao;   // 六爻（index 0=初爻, 5=上爻）
    
    // 时间信息
    std::string gua_time;           // 起卦时间
    std::string ganzhi_year;        // 年干支
    std::string ganzhi_month;       // 月干支
    std::string ganzhi_day;         // 日干支
    std::string ganzhi_hour;        // 时干支
    
    // JSON
    std::string gua_json;
};

// ==================== 纳甲数据 ====================
struct NajiaData {
    std::string tiangan;           // 天干
    std::array<std::string, 3> neiguo_dizhi;  // 内卦三爻地支
    std::array<std::string, 3> waiguo_dizhi;  // 外卦三爻地支
    std::string wuxing;            // 宫五行
};

class LiuyaoEngine {
private:
    // 八卦纳甲数据
    static const std::map<int, NajiaData>& GetNajiaMap() {
        static const std::map<int, NajiaData> najia = {
            // num -> {天干, 内卦地支[初,二,三], 外卦地支[四,五,上], 宫五行}
            {1, {"甲", {"子","寅","辰"}, {"辰","午","申"}, "金"}},  // 乾
            {2, {"丁", {"巳","卯","丑"}, {"亥","酉","未"}, "金"}},  // 兑
            {3, {"己", {"卯","丑","亥"}, {"酉","未","巳"}, "火"}},  // 离
            {4, {"庚", {"子","寅","辰"}, {"辰","午","申"}, "木"}},  // 震
            {5, {"辛", {"丑","亥","酉"}, {"未","巳","卯"}, "木"}},  // 巽
            {6, {"戊", {"寅","辰","午"}, {"申","戌","子"}, "水"}},  // 坎
            {7, {"丙", {"辰","午","申"}, {"戌","子","寅"}, "土"}},  // 艮
            {8, {"乙", {"未","巳","卯"}, {"丑","亥","酉"}, "土"}}   // 坤
        };
        return najia;
    }

    // 地支五行
    static std::string DizhiWuxing(const std::string& dz) {
        static const std::map<std::string, std::string> m = {
            {"子","水"},{"丑","土"},{"寅","木"},{"卯","木"},
            {"辰","土"},{"巳","火"},{"午","火"},{"未","土"},
            {"申","金"},{"酉","金"},{"戌","土"},{"亥","水"}
        };
        auto it = m.find(dz);
        return it != m.end() ? it->second : "未知";
    }

    // 五行生克定六亲
    static std::string GetLiuqin(const std::string& gong_wx, const std::string& yao_wx) {
        if (gong_wx == yao_wx) return "兄弟";
        // 生我者为父母
        if ((gong_wx=="金"&&yao_wx=="土")||(gong_wx=="木"&&yao_wx=="水")||
            (gong_wx=="水"&&yao_wx=="金")||(gong_wx=="火"&&yao_wx=="木")||
            (gong_wx=="土"&&yao_wx=="火")) return "父母";
        // 我生者为子孙
        if ((gong_wx=="金"&&yao_wx=="水")||(gong_wx=="木"&&yao_wx=="火")||
            (gong_wx=="水"&&yao_wx=="木")||(gong_wx=="火"&&yao_wx=="土")||
            (gong_wx=="土"&&yao_wx=="金")) return "子孙";
        // 克我者为官鬼
        if ((gong_wx=="金"&&yao_wx=="火")||(gong_wx=="木"&&yao_wx=="金")||
            (gong_wx=="水"&&yao_wx=="土")||(gong_wx=="火"&&yao_wx=="水")||
            (gong_wx=="土"&&yao_wx=="木")) return "官鬼";
        // 我克者为妻财
        if ((gong_wx=="金"&&yao_wx=="木")||(gong_wx=="木"&&yao_wx=="土")||
            (gong_wx=="水"&&yao_wx=="火")||(gong_wx=="火"&&yao_wx=="金")||
            (gong_wx=="土"&&yao_wx=="水")) return "妻财";
        return "兄弟";
    }

    // 世应位置映射（根据卦在宫中的位置）
    // 八宫：本宫卦(6,3), 一世(1,4), 二世(2,5), 三世(3,6), 四世(4,1), 五世(5,2), 游魂(4,1), 归魂(3,6)
    static std::pair<int,int> GetShiYing(int gua_upper, int gua_lower) {
        // 查找该卦在哪个宫、哪个位置
        struct GuaInfo { int gong; int pos; }; // gong=宫卦编号, pos=0-7
        static const std::map<std::pair<int,int>, GuaInfo> gua_to_gong = {
            // 乾宫
            {{1,1},{1,0}}, {{1,5},{1,1}}, {{1,7},{1,2}}, {{1,8},{1,3}},
            {{5,8},{1,4}}, {{7,8},{1,5}}, {{3,8},{1,6}}, {{3,1},{1,7}},
            // 坤宫
            {{8,8},{8,0}}, {{8,4},{8,1}}, {{8,2},{8,2}}, {{8,1},{8,3}},
            {{4,1},{8,4}}, {{2,1},{8,5}}, {{6,1},{8,6}}, {{6,8},{8,7}},
            // 震宫
            {{4,4},{4,0}}, {{4,8},{4,1}}, {{4,6},{4,2}}, {{4,5},{4,3}},
            {{8,5},{4,4}}, {{6,5},{4,5}}, {{2,5},{4,6}}, {{2,4},{4,7}},
            // 巽宫
            {{5,5},{5,0}}, {{5,1},{5,1}}, {{5,3},{5,2}}, {{5,4},{5,3}},
            {{4,1},{5,4}}, // note: duplicate with 坤宫, using first found
            {{3,4},{5,5}}, {{7,4},{5,6}}, {{7,5},{5,7}},
            // 坎宫
            {{6,6},{6,0}}, {{6,2},{6,1}}, {{6,4},{6,2}}, {{6,3},{6,3}},
            {{2,3},{6,4}}, {{4,3},{6,5}}, {{8,3},{6,6}}, {{8,6},{6,7}},
            // 离宫
            {{3,3},{3,0}}, {{3,7},{3,1}}, {{3,5},{3,2}}, {{3,6},{3,3}},
            {{7,6},{3,4}}, {{5,6},{3,5}}, {{1,6},{3,6}}, {{1,3},{3,7}},
            // 艮宫
            {{7,7},{7,0}}, {{7,3},{7,1}}, {{7,1},{7,2}}, {{7,2},{7,3}},
            {{3,2},{7,4}}, {{1,2},{7,5}}, {{5,2},{7,6}}, {{5,7},{7,7}},
            // 兑宫
            {{2,2},{2,0}}, {{2,6},{2,1}}, {{2,8},{2,2}}, {{2,7},{2,3}},
            {{6,7},{2,4}}, {{8,7},{2,5}}, {{4,7},{2,6}}, {{4,2},{2,7}}
        };
        
        auto it = gua_to_gong.find({gua_upper, gua_lower});
        if (it == gua_to_gong.end()) return {3, 6}; // 默认三世卦
        
        int pos = it->second.pos;
        // 世应表
        static const int shi_pos[] = {6, 1, 2, 3, 4, 5, 4, 3}; // 本宫,一世~五世,游魂,归魂
        static const int ying_pos[] = {3, 4, 5, 6, 1, 2, 1, 6};
        
        return {shi_pos[pos], ying_pos[pos]};
    }

    // 六神配日干
    static std::string GetLiushen(const std::string& ri_gan, int yao_pos) {
        // 六神顺序
        static const std::string liushen_order[] = {"青龙","朱雀","勾陈","螣蛇","白虎","玄武"};
        // 日干对应的初爻六神索引
        int start = 0;
        if (ri_gan == "甲" || ri_gan == "乙") start = 0;
        else if (ri_gan == "丙" || ri_gan == "丁") start = 1;
        else if (ri_gan == "戊") start = 2;
        else if (ri_gan == "己") start = 3;
        else if (ri_gan == "庚" || ri_gan == "辛") start = 4;
        else if (ri_gan == "壬" || ri_gan == "癸") start = 5;
        
        int idx = (start + yao_pos - 1) % 6;
        return liushen_order[idx];
    }

    // 求两个八卦组合的卦名
    static std::string GetGuaName(int upper, int lower) {
        static const std::map<std::pair<int,int>, std::string> name_map = {
            {{1,1},"乾为天"},{{2,2},"兑为泽"},{{3,3},"离为火"},{{4,4},"震为雷"},
            {{5,5},"巽为风"},{{6,6},"坎为水"},{{7,7},"艮为山"},{{8,8},"坤为地"},
            {{1,8},"天地否"},{{8,1},"地天泰"},{{1,5},"天风姤"},{{5,1},"风天小畜"},
            {{1,6},"天水讼"},{{6,1},"水天需"},{{2,7},"泽山咸"},{{7,2},"山泽损"},
            {{2,4},"泽雷随"},{{4,2},"雷泽归妹"},{{3,7},"火山旅"},{{7,3},"山火贲"},
            {{3,4},"火雷噬嗑"},{{4,3},"雷火丰"},{{5,6},"风水涣"},{{6,5},"水风井"},
            {{5,7},"风山渐"},{{7,5},"山风蛊"},{{6,7},"水山蹇"},{{7,6},"山水蒙"},
            {{6,8},"水地比"},{{8,6},"地水师"},{{3,8},"火地晋"},{{8,3},"地火明夷"},
            {{5,8},"风地观"},{{8,5},"地风升"},{{4,7},"雷山小过"},{{7,4},"山雷颐"},
            {{4,8},"雷地豫"},{{8,4},"地雷复"},{{4,6},"雷水解"},{{6,4},"水雷屯"},
            {{4,5},"雷风恒"},{{5,4},"风雷益"},{{3,5},"火风鼎"},{{5,3},"风火家人"},
            {{3,6},"火水未济"},{{6,3},"水火既济"},{{2,6},"泽水困"},{{6,2},"水泽节"},
            {{2,3},"泽火革"},{{3,2},"火泽睽"},{{2,5},"泽风大过"},{{5,2},"风泽中孚"},
            {{2,1},"泽天夬"},{{1,2},"天泽履"},{{3,1},"天火同人"},{{1,3},"火天大有"},
            {{4,1},"天雷无妄"},{{1,4},"雷天大壮"},{{2,8},"泽地萃"},{{8,2},"地泽临"},
            {{7,8},"山地剥"},{{8,7},"地山谦"},{{7,1},"山天大畜"},{{1,7},"天山遁"}
        };
        auto it = name_map.find({upper, lower});
        return it != name_map.end() ? it->second : "未知卦";
    }

    static std::string EscapeJson(const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:   result += c;
            }
        }
        return result;
    }

public:
    // 随机掷铜钱起卦（模拟三枚铜钱投掷6次）
    static LiuyaoResult RandomGua() {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        std::array<int, 6> coins;
        for (int i = 0; i < 6; i++) {
            // 三枚铜钱：正面=3，背面=2
            // 结果范围：6(老阴), 7(少阳), 8(少阴), 9(老阳)
            int sum = 0;
            for (int j = 0; j < 3; j++) {
                sum += (std::rand() % 2 == 0) ? 3 : 2;  // 3=字面, 2=花面
            }
            coins[i] = sum;
        }
        return BuildFromCoins(coins);
    }

    // 从指定铜钱结果起卦
    static LiuyaoResult FromCoins(const std::array<int, 6>& coins) {
        return BuildFromCoins(coins);
    }

    // 核心排卦逻辑
    static LiuyaoResult BuildFromCoins(const std::array<int, 6>& coins) {
        LiuyaoResult result;
        
        // 1. 解析每爻
        int ben_bits[6];   // 本卦6爻：0=阴, 1=阳
        int bian_bits[6];  // 变卦6爻
        
        for (int i = 0; i < 6; i++) {
            int c = coins[i];
            result.yao[i].position = i + 1;
            result.yao[i].coin_result = c;
            result.yao[i].is_shi = false;
            result.yao[i].is_ying = false;
            
            switch (c) {
                case 6: // 老阴 -> 变阳
                    ben_bits[i] = 0;
                    bian_bits[i] = 1;
                    result.yao[i].yinyang = 0;
                    result.yao[i].is_dong = true;
                    result.yao[i].bian_yinyang = 1;
                    break;
                case 7: // 少阳 -> 不变
                    ben_bits[i] = 1;
                    bian_bits[i] = 1;
                    result.yao[i].yinyang = 1;
                    result.yao[i].is_dong = false;
                    result.yao[i].bian_yinyang = 1;
                    break;
                case 8: // 少阴 -> 不变
                    ben_bits[i] = 0;
                    bian_bits[i] = 0;
                    result.yao[i].yinyang = 0;
                    result.yao[i].is_dong = false;
                    result.yao[i].bian_yinyang = 0;
                    break;
                case 9: // 老阳 -> 变阴
                    ben_bits[i] = 1;
                    bian_bits[i] = 0;
                    result.yao[i].yinyang = 1;
                    result.yao[i].is_dong = true;
                    result.yao[i].bian_yinyang = 0;
                    break;
                default:
                    ben_bits[i] = 1;
                    bian_bits[i] = 1;
                    result.yao[i].yinyang = 1;
                    result.yao[i].is_dong = false;
                    result.yao[i].bian_yinyang = 1;
                    break;
            }
        }
        
        // 2. 根据六爻确定上下卦
        // 下卦(内卦): 初爻(0),二爻(1),三爻(2)  上卦(外卦): 四爻(3),五爻(4),上爻(5)
        // 八卦二进制编码: 从下到上读
        auto bits_to_trigram = [](int b0, int b1, int b2) -> int {
            // 标准先天八卦数对应：
            // 111=乾1, 110=兑2, 101=离3, 100=震4, 011=巽5, 010=坎6, 001=艮7, 000=坤8
            int val = b0 + b1 * 2 + b2 * 4;
            static const int trigram_map[] = {8, 7, 6, 5, 4, 3, 2, 1}; // 0->坤,1->艮,...,7->乾
            return trigram_map[val];
        };
        
        result.ben_lower = bits_to_trigram(ben_bits[0], ben_bits[1], ben_bits[2]);
        result.ben_upper = bits_to_trigram(ben_bits[3], ben_bits[4], ben_bits[5]);
        result.bian_lower = bits_to_trigram(bian_bits[0], bian_bits[1], bian_bits[2]);
        result.bian_upper = bits_to_trigram(bian_bits[3], bian_bits[4], bian_bits[5]);
        
        result.ben_gua_name = GetGuaName(result.ben_upper, result.ben_lower);
        result.bian_gua_name = GetGuaName(result.bian_upper, result.bian_lower);
        
        // 3. 纳甲 - 装地支
        const auto& najia = GetNajiaMap();
        // 下卦纳甲
        auto lower_it = najia.find(result.ben_lower);
        if (lower_it != najia.end()) {
            result.gong_name = lower_it->second.wuxing; // 暂存，后面用宫属性
            for (int i = 0; i < 3; i++) {
                result.yao[i].dizhi = lower_it->second.neiguo_dizhi[i];
                result.yao[i].wuxing = DizhiWuxing(result.yao[i].dizhi);
            }
        }
        // 上卦纳甲
        auto upper_it = najia.find(result.ben_upper);
        if (upper_it != najia.end()) {
            for (int i = 0; i < 3; i++) {
                result.yao[i + 3].dizhi = upper_it->second.waiguo_dizhi[i];
                result.yao[i + 3].wuxing = DizhiWuxing(result.yao[i + 3].dizhi);
            }
        }
        
        // 4. 确定所属宫 - 通过查找八宫归属表
        // 查找本卦在哪个宫
        struct GuaInGong { int gong_num; };
        static const std::map<std::pair<int,int>, int> gua_gong = {
            {{1,1},1},{{1,5},1},{{1,7},1},{{1,8},1},{{5,8},1},{{7,8},1},{{3,8},1},{{3,1},1},
            {{8,8},8},{{8,4},8},{{8,2},8},{{8,1},8},{{4,1},8},{{2,1},8},{{6,1},8},{{6,8},8},
            {{4,4},4},{{4,8},4},{{4,6},4},{{4,5},4},{{8,5},4},{{6,5},4},{{2,5},4},{{2,4},4},
            {{5,5},5},{{5,1},5},{{5,3},5},{{5,4},5},{{3,4},5},{{7,4},5},{{7,5},5},{{7,5},5},
            {{6,6},6},{{6,2},6},{{6,4},6},{{6,3},6},{{2,3},6},{{4,3},6},{{8,3},6},{{8,6},6},
            {{3,3},3},{{3,7},3},{{3,5},3},{{3,6},3},{{7,6},3},{{5,6},3},{{1,6},3},{{1,3},3},
            {{7,7},7},{{7,3},7},{{7,1},7},{{7,2},7},{{3,2},7},{{1,2},7},{{5,2},7},{{5,7},7},
            {{2,2},2},{{2,6},2},{{2,8},2},{{2,7},2},{{6,7},2},{{8,7},2},{{4,7},2},{{4,2},2}
        };
        
        int gong_num = 1; // 默认乾宫
        auto git = gua_gong.find({result.ben_upper, result.ben_lower});
        if (git != gua_gong.end()) {
            gong_num = git->second;
        }
        
        auto gong_it = najia.find(gong_num);
        if (gong_it != najia.end()) {
            result.gong_wuxing = gong_it->second.wuxing;
        }
        static const std::map<int, std::string> gong_names = {
            {1,"乾宫"},{2,"兑宫"},{3,"离宫"},{4,"震宫"},
            {5,"巽宫"},{6,"坎宫"},{7,"艮宫"},{8,"坤宫"}
        };
        auto gn_it = gong_names.find(gong_num);
        result.gong_name = gn_it != gong_names.end() ? gn_it->second : "未知宫";
        
        // 5. 装六亲
        for (int i = 0; i < 6; i++) {
            result.yao[i].liuqin = GetLiuqin(result.gong_wuxing, result.yao[i].wuxing);
        }
        
        // 6. 定世应
        auto shi_ying = GetShiYing(result.ben_upper, result.ben_lower);
        result.yao[shi_ying.first - 1].is_shi = true;
        result.yao[shi_ying.second - 1].is_ying = true;
        
        // 7. 配六神（默认用甲子日）
        std::string ri_gan = "甲";
        for (int i = 0; i < 6; i++) {
            result.yao[i].liushen = GetLiushen(ri_gan, i + 1);
        }
        
        // 8. 起卦时间
        std::time_t now = std::time(nullptr);
        std::tm* t = std::localtime(&now);
        {
            std::ostringstream oss;
            oss << (t->tm_year + 1900) << "-" 
                << std::setfill('0') << std::setw(2) << (t->tm_mon + 1) << "-"
                << std::setfill('0') << std::setw(2) << t->tm_mday << " "
                << std::setfill('0') << std::setw(2) << t->tm_hour << ":"
                << std::setfill('0') << std::setw(2) << t->tm_min;
            result.gua_time = oss.str();
        }
        result.ganzhi_year = "甲子年";  // 简化处理
        result.ganzhi_month = "正月";
        result.ganzhi_day = "甲子日";
        result.ganzhi_hour = "子时";
        
        // 9. 生成JSON
        result.gua_json = ToJson(result);
        
        return result;
    }

    // JSON序列化
    static std::string ToJson(const LiuyaoResult& r) {
        std::ostringstream oss;
        oss << std::fixed;
        oss << "{";
        oss << "\"type\":\"liuyao\",";
        oss << "\"ben_gua\":\"" << EscapeJson(r.ben_gua_name) << "\",";
        oss << "\"bian_gua\":\"" << EscapeJson(r.bian_gua_name) << "\",";
        oss << "\"gong\":\"" << EscapeJson(r.gong_name) << "\",";
        oss << "\"gong_wuxing\":\"" << EscapeJson(r.gong_wuxing) << "\",";
        oss << "\"gua_time\":\"" << EscapeJson(r.gua_time) << "\",";
        oss << "\"ganzhi\":{\"year\":\"" << EscapeJson(r.ganzhi_year) 
             << "\",\"month\":\"" << EscapeJson(r.ganzhi_month)
             << "\",\"day\":\"" << EscapeJson(r.ganzhi_day)
             << "\",\"hour\":\"" << EscapeJson(r.ganzhi_hour) << "\"},";
        
        oss << "\"yao\":[";
        for (int i = 0; i < 6; i++) {
            if (i > 0) oss << ",";
            const auto& y = r.yao[i];
            oss << "{";
            oss << "\"pos\":" << y.position << ",";
            oss << "\"coin\":" << y.coin_result << ",";
            oss << "\"yy\":" << (y.yinyang ? "\"阳\"" : "\"阴\"") << ",";
            oss << "\"dong\":" << (y.is_dong ? "true" : "false") << ",";
            oss << "\"bian_yy\":" << (y.bian_yinyang ? "\"阳\"" : "\"阴\"") << ",";
            oss << "\"dz\":\"" << EscapeJson(y.dizhi) << "\",";
            oss << "\"wx\":\"" << EscapeJson(y.wuxing) << "\",";
            oss << "\"lq\":\"" << EscapeJson(y.liuqin) << "\",";
            oss << "\"ls\":\"" << EscapeJson(y.liushen) << "\",";
            oss << "\"shi\":" << (y.is_shi ? "true" : "false") << ",";
            oss << "\"ying\":" << (y.is_ying ? "true" : "false");
            oss << "}";
        }
        oss << "],";
        
        // 动爻列表
        oss << "\"dong_yao\":[";
        bool first = true;
        for (int i = 0; i < 6; i++) {
            if (r.yao[i].is_dong) {
                if (!first) oss << ",";
                oss << (i + 1);
                first = false;
            }
        }
        oss << "]";
        
        oss << "}";
        return oss.str();
    }
};

} // namespace zhouyi

#endif
