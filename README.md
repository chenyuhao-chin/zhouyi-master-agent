# 🎋 梅花易数 AI 大师

基于 C++ 构建的智能梅花易数占卜系统，融合传统易学智慧与现代 AI 技术，提供专业的起卦、解卦服务。

## ✨ 功能特性

- **多种起卦方式** - 数字起卦、时间起卦、随机起卦
- **AI 智能解卦** - 接入大语言模型，结合卦象、爻辞进行深度解读
- **体用分析** - 自动识别体卦、用卦，分析五行生克关系
- **会话管理** - 支持多会话，历史记录持久化存储
- **中国风 UI** - 暗色调古风界面，沉浸式占卜体验
- **知识库支撑** - 六十四卦全文、五行生克、梅花易数经典规则

## 🏗️ 技术架构

| 组件 | 技术 |
|------|------|
| 语言 | C++17 |
| 数据库 | SQLite3 (WAL 模式) |
| HTTP 服务 | 自研轻量级 HTTP Server |
| LLM 接口 | cURL + OpenAI API |
| 前端 | 原生 HTML/CSS/JavaScript |
| 构建 | CMake |

## 📁 目录结构

```
zhouyi-master-agent/
├── include/              # 头文件
│   ├── database.hpp      # SQLite 数据库操作
│   ├── http_server.hpp   # HTTP 服务器
│   ├── llm_gateway.hpp   # LLM 网关（OpenAI API）
│   ├── meihua_engine.hpp # 梅花易数计算引擎
│   └── session_manager.hpp # 会话管理
├── src/
│   └── main.cpp          # 主程序入口
├── data/                 # 易经知识库（JSON）
│   ├── bagua.json        # 八卦基础数据
│   ├── gua64.json        # 六十四卦全文
│   ├── wuxing.json       # 五行生克关系
│   └── meihua_rules.json # 梅花易数规则
├── wwwroot/
│   └── index.html        # Web 前端页面
├── CMakeLists.txt        # 构建配置
├── build.sh              # 一键构建脚本
├── config.json           # 运行时配置（需自行创建）
└── README.md
```

## 🚀 快速开始

### 1. 安装依赖

```bash
# Ubuntu / Debian
sudo apt update
sudo apt install cmake g++ libcurl4-openssl-dev libsqlite3-dev

# CentOS / RHEL
sudo yum install cmake gcc-c++ libcurl-devel sqlite-devel
```

### 2. 配置 API Key

创建 `config.json`：

```json
{
  "api_key": "sk-your-openai-api-key",
  "api_url": "https://api.openai.com/v1/chat/completions",
  "model": "gpt-4o-mini",
  "port": 8080,
  "db_path": "meihua.db",
  "data_dir": "data"
}
```

| 字段 | 说明 |
|------|------|
| `api_key` | OpenAI API Key（或兼容接口的 Key） |
| `api_url` | API 端点地址（支持自定义） |
| `model` | 模型名称 |
| `port` | HTTP 服务端口 |
| `db_path` | SQLite 数据库文件路径 |
| `data_dir` | 知识库 JSON 文件目录 |

### 3. 构建

```bash
# 方式一：使用构建脚本
chmod +x build.sh
./build.sh

# 方式二：手动构建
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

### 4. 运行

```bash
cd build
./zhouyi-master
```

浏览器访问 `http://localhost:8080`

## 📡 API 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| `GET` | `/` | 前端页面 |
| `POST` | `/api/divination` | 起卦解卦（数字起卦） |
| `POST` | `/api/chat` | 对话（追问） |
| `GET` | `/api/sessions` | 获取会话列表 |
| `POST` | `/api/sessions` | 创建新会话 |
| `GET` | `/api/sessions/{id}/messages` | 获取会话消息 |
| `DELETE` | `/api/sessions/{id}` | 删除会话 |

### 起卦示例

```bash
curl -X POST http://localhost:8080/api/divination \
  -H "Content-Type: application/json" \
  -d '{"number1": 5, "number2": 3, "session_id": "abc123"}'
```

## 🔮 梅花易数原理

梅花易数由北宋邵雍（邵康节）所创，以"心易"为核心，通过数字、时间、外应等起卦。

1. **起卦** - 上卦 = 数1 % 8，下卦 = 数2 % 8，动爻 = (数1 + 数2) % 6
2. **装卦** - 确定每卦对应的五行属性
3. **定体用** - 动爻所在卦为用卦，另一卦为体卦
4. **断卦** - 分析体用五行生克关系判断吉凶
   - 用生体 → 吉
   - 体生用 → 泄气，小凶
   - 用克体 → 凶
   - 体克用 → 可得，小吉
   - 比和 → 平

## 📚 知识库

`data/` 目录下包含完整的易经知识库，以 JSON 格式存储：

- **bagua.json** - 八卦基础属性（五行、方位、象义）
- **gua64.json** - 六十四卦卦辞、爻辞全文
- **wuxing.json** - 五行生克制化关系
- **meihua_rules.json** - 梅花易数起卦断卦规则

这些数据同时被用于 LLM 的 System Prompt，确保 AI 解读基于权威易学典籍。

## 📝 后续规划

- [ ] 微信小程序版本
- [ ] 流式输出（SSE）支持
- [ ] 更多起卦方式（报数、外应、时间起卦）
- [ ] 卦象图形化展示
- [ ] 占断案例库积累

## 📄 许可证

MIT License
