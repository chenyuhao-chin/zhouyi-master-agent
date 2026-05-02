#ifndef __MEIHUA_ENGINE_HPP__
#define __MEIHUA_ENGINE_HPP__

#include <string>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>

namespace zhouyi {

// ==================== 64卦数据 ====================
struct GuaInfo {
    int number;          // 卦序 1-64
    std::string name;    // 卦名
    std::string symbol;  // 卦符 Unicode
    int upper;           // 上卦数 1-8
    int lower;           // 下卦数 1-8
    std::string judgment; // 卦辞
    std::string image;    // 象辞
};

// 八卦基础数据
struct TrigramInfo {
    int number;       // 1-8
    std::string name; // 乾兑离震巽坎艮坤
    std::string element; // 五行
    std::string nature;
};

// ==================== 64卦完整数据库 ====================
static const std::map<int, TrigramInfo> TRIGRAMS = {
    {1, {1, "乾", "金", "天"}},
    {2, {2, "兑", "金", "泽"}},
    {3, {3, "离", "火", "火"}},
    {4, {4, "震", "木", "雷"}},
    {5, {5, "巽", "木", "风"}},
    {6, {6, "坎", "水", "水"}},
    {7, {7, "艮", "土", "山"}},
    {8, {8, "坤", "土", "地"}}
};

// 64卦映射表: (上卦, 下卦) -> 卦信息
static const std::map<std::pair<int,int>, GuaInfo> GUA_64 = {
    {{1,1}, {1,  "乾为天",     "☰☰", 1, 1, "元亨利贞", "天行健，君子以自强不息"}},
    {{2,2}, {2,  "兑为泽",     "☱☱", 2, 2, "亨，利贞", "丽泽，君子以朋友讲习"}},
    {{3,3}, {3,  "离为火",     "☲☲", 3, 3, "利贞，亨", "明两作，大人以继明照于四方"}},
    {{4,4}, {4,  "震为雷",     "☳☳", 4, 4, "亨，震来虩虩", "洊雷，君子以恐惧修省"}},
    {{5,5}, {5,  "巽为风",     "☴☴", 5, 5, "小亨，利有攸往", "随风，君子以申命行事"}},
    {{6,6}, {6,  "坎为水",     "☵☵", 6, 6, "习坎，有孚", "水洊至，君子以常德行习教事"}},
    {{7,7}, {7,  "艮为山",     "☶☶", 7, 7, "艮其背，不获其身", "兼山，君子以思不出其位"}},
    {{8,8}, {8,  "坤为地",     "☷☷", 8, 8, "元亨，利牝马之贞", "地势坤，君子以厚德载物"}},
    {{1,8}, {9,  "天地否",     "☰☷", 1, 8, "否之匪人，不利君子贞", "天地不交，否"}},
    {{8,1}, {10, "地天泰",     "☷☰", 8, 1, "小往大来，吉亨", "天地交，泰"}},
    {{1,5}, {11, "天风姤",     "☰☴", 1, 5, "女壮，勿用取女", "天下有风，姤"}},
    {{5,1}, {12, "风天小畜",   "☴☰", 5, 1, "亨，密云不雨", "风行天上，小畜"}},
    {{1,6}, {13, "天水讼",     "☰☵", 1, 6, "有孚窒惕，中吉", "天与水违行，讼"}},
    {{6,1}, {14, "水天需",     "☵☰", 6, 1, "有孚，光亨", "云上于天，需"}},
    {{2,7}, {15, "泽山咸",     "☱☶", 2, 7, "亨，利贞", "山上有泽，咸"}},
    {{7,2}, {16, "山泽损",     "☶☱", 7, 2, "有孚，元吉", "山下有泽，损"}},
    {{2,4}, {17, "泽雷随",     "☱☳", 2, 4, "元亨利贞，无咎", "泽中有雷，随"}},
    {{4,2}, {18, "雷泽归妹",   "☳☱", 4, 2, "征凶，无攸利", "泽上有雷，归妹"}},
    {{3,7}, {19, "火山旅",     "☲☶", 3, 7, "小亨，旅贞吉", "山上有火，旅"}},
    {{7,3}, {20, "山火贲",     "☶☲", 7, 3, "亨，小利有攸往", "山下有火，贲"}},
    {{3,4}, {21, "火雷噬嗑",   "☲☳", 3, 4, "亨，利用狱", "雷电噬嗑"}},
    {{4,3}, {22, "雷火丰",     "☳☲", 4, 3, "亨，王假之", "雷电皆至，丰"}},
    {{5,6}, {23, "风水涣",     "☴☵", 5, 6, "亨，王假有庙", "风行水上，涣"}},
    {{6,5}, {24, "水风井",     "☵☴", 6, 5, "改邑不改井", "木上有水，井"}},
    {{5,7}, {25, "风山渐",     "☴☶", 5, 7, "女归吉，利贞", "山上有木，渐"}},
    {{7,5}, {26, "山风蛊",     "☶☴", 7, 5, "元亨，利涉大川", "山下有风，蛊"}},
    {{6,7}, {27, "水山蹇",     "☵☶", 6, 7, "利西南", "山上有水，蹇"}},
    {{7,6}, {28, "山水蒙",     "☶☵", 7, 6, "亨，匪我求童蒙", "山下出泉，蒙"}},
    {{6,8}, {29, "水地比",     "☵☷", 6, 8, "吉，原筮元永贞", "地上有水，比"}},
    {{8,6}, {30, "地水师",     "☷☵", 8, 6, "贞丈人吉", "地中有水，师"}},
    {{3,8}, {31, "火地晋",     "☲☷", 3, 8, "康侯用锡马蕃庶", "明出地上，晋"}},
    {{8,3}, {32, "地火明夷",   "☷☲", 8, 3, "利艰贞", "明入地中，明夷"}},
    {{5,8}, {33, "风地观",     "☴☷", 5, 8, "盥而不荐", "风行地上，观"}},
    {{8,5}, {34, "地风升",     "☷☴", 8, 5, "元亨，用见大人", "地中生木，升"}},
    {{4,7}, {35, "雷山小过",   "☳☶", 4, 7, "亨，利贞", "山上有雷，小过"}},
    {{7,4}, {36, "山雷颐",     "☶☳", 7, 4, "贞吉", "山下有雷，颐"}},
    {{4,8}, {37, "雷地豫",     "☳☷", 4, 8, "利建侯行师", "雷出地奋，豫"}},
    {{8,4}, {38, "地雷复",     "☷☳", 8, 4, "亨，出入无疾", "雷在地中，复"}},
    {{4,6}, {39, "雷水解",     "☳☵", 4, 6, "利西南", "雷雨作，解"}},
    {{6,4}, {40, "水雷屯",     "☵☳", 6, 4, "元亨利贞", "云雷屯"}},
    {{4,5}, {41, "雷风恒",     "☳☴", 4, 5, "亨，无咎，利贞", "雷风恒"}},
    {{5,4}, {42, "风雷益",     "☴☳", 5, 4, "利有攸往", "风雷益"}},
    {{3,5}, {43, "火风鼎",     "☲☴", 3, 5, "元吉，亨", "木上有火，鼎"}},
    {{5,3}, {44, "风火家人",   "☴☲", 5, 3, "利女贞", "风自火出，家人"}},
    {{3,6}, {45, "火水未济",   "☲☵", 3, 6, "亨，小狐汔济", "火在水上，未济"}},
    {{6,3}, {46, "水火既济",   "☵☲", 6, 3, "亨小，利贞", "水在火上，既济"}},
    {{2,6}, {47, "泽水困",     "☱☵", 2, 6, "亨，贞大人吉", "泽无水，困"}},
    {{6,2}, {48, "水泽节",     "☵☱", 6, 2, "亨，苦节不可贞", "泽上有水，节"}},
    {{2,3}, {49, "泽火革",     "☱☲", 2, 3, "已日乃孚，元亨", "泽中有火，革"}},
    {{3,2}, {50, "火泽睽",     "☲☱", 3, 2, "小事吉", "上火下泽，睽"}},
    {{2,5}, {51, "泽风大过",   "☱☴", 2, 5, "栋桡，利有攸往", "泽灭木，大过"}},
    {{5,2}, {52, "风泽中孚",   "☴☱", 5, 2, "豚鱼吉，利涉大川", "泽上有风，中孚"}},
    {{2,1}, {53, "泽天夬",     "☱☰", 2, 1, "扬于王庭", "泽上于天，夬"}},
    {{1,2}, {54, "天泽履",     "☰☱", 1, 2, "履虎尾，不咥人", "上天下泽，履"}},
    {{3,1}, {55, "天火同人",   "☰☲", 3, 1, "同人于野，亨", "天与火，同人"}},
    {{1,3}, {56, "火天大有",   "☲☰", 1, 3, "元亨", "火在天上，大有"}},
    {{4,1}, {57, "天雷无妄",   "☰☳", 4, 1, "元亨利贞", "天下雷行，无妄"}},
    {{1,4}, {58, "雷天大壮",   "☳☰", 1, 4, "利贞", "雷在天上，大壮"}},
    {{2,8}, {59, "泽地萃",     "☱☷", 2, 8, "亨，王假有庙", "泽上于地，萃"}},
    {{8,2}, {60, "地泽临",     "☷☱", 8, 2, "元亨利贞", "泽上有地，临"}},
    {{7,8}, {61, "山地剥",     "☶☷", 7, 8, "不利有攸往", "山附于地，剥"}},
    {{8,7}, {62, "地山谦",     "☷☶", 8, 7, "亨，君子有终", "地中有山，谦"}},
    {{7,1}, {63, "山天大畜",   "☶☰", 7, 1, "利贞，不家食吉", "天在山中，大畜"}},
    {{1,7}, {64, "天山遁",     "☰☶", 1, 7, "亨，小利贞", "天下有山，遁"}}
};

// ==================== 梅花易数排卦结果 ====================
struct MeihuaResult {
    int upper_num;      // 上卦数
    int lower_num;      // 下卦数
    int dong_yao;       // 动爻位置 1-6
    int bian_upper;     // 变卦上卦
    int bian_lower;     // 变卦下卦
    int ti_num;         // 体卦
    int yong_num;       // 用卦
    std::string gua_name;      // 本卦名
    std::string bian_gua_name; // 变卦名
    std::string wuxing_ti;     // 体卦五行
    std::string wuxing_yong;   // 用卦五行
    std::string gua_json;      // 完整JSON
};

class MeihuaEngine {
public:
    // 梅花易数起卦：随机时间起卦
    static MeihuaResult RandomGua() {
        std::time_t now = std::time(nullptr);
        std::tm* t = std::localtime(&now);
        
        // 用当前时间的年月日时分秒起卦
        int year = t->tm_year + 1900;
        int month = t->tm_mon + 1;
        int day = t->tm_mday;
        int hour = t->tm_hour;
        int minute = t->tm_min;
        int second = t->tm_sec;
        
        // 上卦 = (年+月+日) % 8
        int upper = (year + month + day) % 8;
        if (upper == 0) upper = 8;
        
        // 下卦 = (年+月+日+时+分+秒) % 8
        int lower = (year + month + day + hour + minute + second) % 8;
        if (lower == 0) lower = 8;
        
        // 动爻 = (年+月+日+时+分+秒) % 6
        int dong = (year + month + day + hour + minute + second) % 6;
        if (dong == 0) dong = 6;
        
        return BuildResult(upper, lower, dong);
    }
    
    // 用户输入数字起卦
    static MeihuaResult NumberGua(int num1, int num2, int num3 = 0) {
        int upper, lower, dong;
        
        if (num3 > 0) {
            // 三个数字：上卦=num1%8, 下卦=num2%8, 动爻=num3%6
            upper = num1 % 8;
            if (upper == 0) upper = 8;
            lower = num2 % 8;
            if (lower == 0) lower = 8;
            dong = num3 % 6;
            if (dong == 0) dong = 6;
        } else {
            // 两个数字：上卦=num1%8, 下卦=num2%8, 动爻=(num1+num2)%6
            upper = num1 % 8;
            if (upper == 0) upper = 8;
            lower = num2 % 8;
            if (lower == 0) lower = 8;
            dong = (num1 + num2) % 6;
            if (dong == 0) dong = 6;
        }
        
        return BuildResult(upper, lower, dong);
    }
    
    // 汉字笔画起卦
    static MeihuaResult TextGua(const std::string& text) {
        // 用UTF-8字节数之和作为起卦数字
        int total = 0;
        for (unsigned char c : text) {
            total += c;
        }
        int len = 0;
        // Count characters (rough UTF-8 count)
        for (size_t i = 0; i < text.size(); ) {
            unsigned char c = text[i];
            int bytes = 1;
            if (c >= 0xF0) bytes = 4;
            else if (c >= 0xE0) bytes = 3;
            else if (c >= 0xC0) bytes = 2;
            i += bytes;
            len++;
        }
        
        int upper = total % 8;
        if (upper == 0) upper = 8;
        int lower = (total + len) % 8;
        if (lower == 0) lower = 8;
        int dong = (total + len) % 6;
        if (dong == 0) dong = 6;
        
        return BuildResult(upper, lower, dong);
    }
    
    // 获取卦信息
    static GuaInfo GetGuaInfo(int upper, int lower) {
        auto it = GUA_64.find({upper, lower});
        if (it != GUA_64.end()) {
            return it->second;
        }
        return {0, "未知卦", "?", upper, lower, "", ""};
    }
    
    // 获取八卦五行
    static std::string GetWuxing(int trigram_num) {
        auto it = TRIGRAMS.find(trigram_num);
        if (it != TRIGRAMS.end()) {
            return it->second.element;
        }
        return "未知";
    }
    
    // 生成完整JSON结果
    static std::string ToJson(const MeihuaResult& r) {
        std::ostringstream oss;
        oss << "{";
        oss << "\"upper_num\":" << r.upper_num << ",";
        oss << "\"lower_num\":" << r.lower_num << ",";
        oss << "\"dong_yao\":" << r.dong_yao << ",";
        oss << "\"bian_upper\":" << r.bian_upper << ",";
        oss << "\"bian_lower\":" << r.bian_lower << ",";
        oss << "\"ti_num\":" << r.ti_num << ",";
        oss << "\"yong_num\":" << r.yong_num << ",";
        oss << "\"gua_name\":\"" << EscapeJson(r.gua_name) << "\",";
        oss << "\"bian_gua_name\":\"" << EscapeJson(r.bian_gua_name) << "\",";
        oss << "\"wuxing_ti\":\"" << EscapeJson(r.wuxing_ti) << "\",";
        oss << "\"wuxing_yong\":\"" << EscapeJson(r.wuxing_yong) << "\",";
        oss << "\"ti_nature\":\"" << EscapeJson(GetTrigramNature(r.ti_num)) << "\",";
        oss << "\"yong_nature\":\"" << EscapeJson(GetTrigramNature(r.yong_num)) << "\"";
        oss << "}";
        return oss.str();
    }

private:
    static MeihuaResult BuildResult(int upper, int lower, int dong) {
        MeihuaResult r;
        r.upper_num = upper;
        r.lower_num = lower;
        r.dong_yao = dong;
        
        // 动爻变化：动爻所在卦的对应爻取反
        // 上卦是4,5,6爻，下卦是1,2,3爻
        r.bian_upper = upper;
        r.bian_lower = lower;
        
        // 动爻对应的八卦数异或变化
        // 梅花易数中，动爻所在的卦发生变化
        int changed_bit = (dong <= 3) ? lower : upper;
        changed_bit = changed_bit ^ 1; // 简化变化
        if (changed_bit == 0) changed_bit = 8;
        if (changed_bit > 8) changed_bit = changed_bit % 8;
        if (changed_bit == 0) changed_bit = 8;
        
        if (dong <= 3) {
            r.bian_lower = changed_bit;
        } else {
            r.bian_upper = changed_bit;
        }
        
        // 体用确定：动爻在下卦则下卦为用（动），上卦为体（静）
        if (dong <= 3) {
            r.ti_num = r.upper_num;  // 上卦为体
            r.yong_num = r.lower_num; // 下卦为用
        } else {
            r.ti_num = r.lower_num;  // 下卦为体
            r.yong_num = r.upper_num; // 上卦为用
        }
        
        // 卦名
        auto gua = GetGuaInfo(r.upper_num, r.lower_num);
        r.gua_name = gua.name;
        
        auto bian_gua = GetGuaInfo(r.bian_upper, r.bian_lower);
        r.bian_gua_name = bian_gua.name;
        
        // 五行
        r.wuxing_ti = GetWuxing(r.ti_num);
        r.wuxing_yong = GetWuxing(r.yong_num);
        
        // JSON
        r.gua_json = ToJson(r);
        
        return r;
    }
    
    static std::string GetTrigramNature(int num) {
        auto it = TRIGRAMS.find(num);
        if (it != TRIGRAMS.end()) {
            return it->second.nature;
        }
        return "未知";
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
};

} // namespace zhouyi

#endif
