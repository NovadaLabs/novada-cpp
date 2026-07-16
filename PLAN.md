# novada-cpp 开发计划

将 Go SDK `novada-go` 移植为功能等价、风格地道的 C++ 库，用 CMake 构建并可通过
vcpkg/Conan/`find_package` 分发。本计划与 `PROMPT.md` 配套：PROMPT 给 Claude Code
当系统级指令，PLAN 给人看进度与验收。

## 关键设计映射（从 Go 源码提取）

| Go 设计 | C++ 对应 |
|---|---|
| 顶层 `Client`，三个 baseURL + Bearer 注入 + 重试 | `novada::Client(api_key, ClientOptions{...})` |
| Functional Options (`WithTimeout`...) | `ClientOptions` 聚合体 + 链式 setter |
| 可替换 `http.Client` | 内部用 libcurl，每请求独立 `CURL*` easy handle（RAII 封装），无跨请求可变状态 |
| `context.Context`（取消/超时） | `CURLOPT_TIMEOUT_MS`；C++ 版不做取消原语，仅超时 |
| 子包 + `transport.Doer` 接口解耦 | `detail::Transport`（内部实现细节，非公开可替换点），子服务持有其引用 |
| 参数 struct + 必填校验 → `ValidationError` | POD 结构体，`std::optional<T>` 表达可选字段，校验抛 `ValidationException` |
| 统一响应包 `{code,data,msg,timestamp}`，仅 `code==0` 成功 | `detail::envelope::decode`（nlohmann::json），`code!=0` → `ApiException` |
| `APIError` + `IsAuthError`/`IsRateLimited`/`CodeOf` | `NovadaException → ApiException → AuthException/RateLimitException` + `is_auth_error`/`is_rate_limited`/`ApiException::code()` |
| 两套编码：multipart / x-www-form-urlencoded | libcurl `curl_mime_*` API / 手写 `curl_easy_escape` 拼接（含 `_raw` 变体） |
| 重试只针对网络错误 + 429/5xx，绝不重试业务 code | 同语义（libcurl `CURLE_*` 传输错误 + 429/5xx） |
| 抓取响应格式不定，返回原始文本 | `do_request()` 返回 `ScrapeResponse{raw}`；Google 走结构化解码 |
| 仅标准库 | 仅两个运行时依赖：libcurl（HTTP）+ nlohmann/json（JSON） |

## 准备工作
- `reference/novada-go/`：Go 源码（`*.go`、`README.md`、`novada-go-sdk-spec.md`）。
- `reference/openapi/`：`novada-openapi.json`、`webunblocker_openapi.json`、`Serpapi_openapi.json`。
  （用仓库根的 `scripts/sync_reference.sh` 同步，见文末。）
- 确认目标平台矩阵（Linux/macOS/Windows）、最低编译器版本（GCC 9+/Clang 10+/MSVC 2019+）、
  分发方式（vcpkg port / Conan recipe / 纯 CMake `find_package`）。

## 里程碑

### M0 — 脚手架与基础设施（1 天，含依赖打通耗时）
- `CMakeLists.txt`（C++17、`find_package(CURL REQUIRED)`、`find_package(nlohmann_json REQUIRED)`，
  `cmake/Dependencies.cmake` 做 `FetchContent` 兜底）、`.clang-format`、`.clang-tidy`、
  `LICENSE`(MIT)、`include/novada/version.hpp`。
- `errors.hpp`（+ `.cpp` 如需要）：`NovadaException / ApiException / AuthException /
  RateLimitException / ValidationException` + `is_auth_error`/`is_rate_limited`。
- `detail/envelope.hpp/.cpp`：envelope 解码 + `code != 0` 转 `ApiException` + list 解包 helper。
- `detail/transport.hpp/.cpp`：`do_multipart` / `do_form_urlencoded`（含 `_raw` 变体）、
  RAII 包 `CURL*`（`curl_easy_init`/`curl_easy_cleanup`）、Bearer 注入、`Accept`/`User-Agent` 头、
  线性退避重试（仅传输层错误 + 429/5xx）、`join_url`、非 2xx → 对应异常。
  `curl_global_init(CURL_GLOBAL_ALL)` 在库加载时执行一次（如用 `std::once_flag`）。
- `client.hpp/.cpp` + `ClientOptions`：构造、三 baseURL、env 回退（`std::getenv("NOVADA_API_KEY")`）、
  子服务装配（先留空）。
- **验收**：GoogleTest + cpp-httplib 本地 server mock 一个 `white_list/list`，断言 Bearer 头、
  multipart body、envelope 解包、`code != 0` 抛 `ApiException`。`clang-format`/`clang-tidy`/
  `ctest` 全绿；ASan/UBSan 构建无告警。

### M1 — Proxy 最小闭环（半天）
- `detail/form_builder.hpp`（req/opt 系列，`std::optional` 判空）、`detail/validator.hpp`、分页默认值。
- `proxy/whitelist_service.hpp/.cpp` + `proxy/account_service.hpp/.cpp`，对应 `proxy/types.hpp`
  的 Params/返回结构体 + `Product`（`enum class`）。
- **验收**：每个方法成功路径 + `ValidationException`（必填缺失）单测；确立"POD Params +
  校验 + multipart 编码 + `from_json` 解码"的可复制模式。

### M2 — Proxy 全量铺开（1.5–2 天）
- 依据 `novada-openapi.json` 的 requestBody properties，批量生成
  `residential / mobile / rotating_isp / rotating_dc / static_isp / dedicated_dc /
  unlimited / prohibit_domain` 的方法、Params、返回类型与必填校验。
- 注意：导出类（静态 IP export）返回文件流 → 用 `_raw` 变体不走 envelope（对应 Go 的
  `DoMultipartRaw`），返回 `std::string`（原始字节）。
- 共享类型：`TimeRange`、`FlowBalance`、`FlowConsumeLog` 等（`proxy/types.hpp`）。
- **验收**：每个子服务至少一个编码 + 解包单测；`clang-tidy` 无新增警告。

### M3 — Scraper 通用驱动（半天）
- `scraper/scraper_service.hpp/.cpp`：`Target`（`enum class`）、`ScrapeRequest`/`ScrapeResponse`、
  `do_request()`（`scraper_params = json(params).dump()`，`curl_easy_escape` 做 urlencode，
  按 `target` 选 host，返回 `ScrapeResponse{raw}`）、必填校验抛 `ValidationException`。
- **验收**：单测断言 urlencoded body、`scraper_params` JSON、host 路由
  （ScraperAPI vs WebUnblocker）、`scraper_errors`。

### M4 — Scraper 强类型 + 查询接口（半天）
- `api().youtube().video_post`（薄封装 `do_request`）、`api().google().search`
  （**扁平 form 字段 + envelope 结构化解码**，对应 `GoogleSearchResult/Data`，`json` 字段保留为
  `nlohmann::json`/原始字符串）。
- `unblocker().scrape`（发 `target_url`/`response_format`/`js_render`/`country`/`wait_ms`，
  解码 `UnblockerResult`：html/code/msg/msg_detail/use_balance）。
- 走通用 host 的查询：`unblocker().countries()`、`browser().countries()`、`universal().balance()`、
  `universal().unit()`。
- **验收**：Google 结构化解码单测；unblocker scrape 字段单测；查询接口走通用 host 验证。

### M5 — Wallet（半天）
- `wallet().balance()`、`wallet().usage_record(...)`（分页默认 1/10）。
- **验收**：单测覆盖分页默认值 + 解码。

### M6 — 文档、示例、CI、打包分发（1 天）
- `README.md`：CMake 集成方式（`find_package(novada CONFIG REQUIRED)` /
  `FetchContent_Declare` / vcpkg）、quick start、三 baseURL 表、proxy/scraper/wallet 各一例、
  错误处理示例（对齐 Go 版）。
- `examples/`：proxy / scraper / wallet 各一个可运行示例（读 `NOVADA_API_KEY`），加进
  `NOVADA_BUILD_EXAMPLES` CMake 选项。
- `.github/workflows/ci.yml`：矩阵 {ubuntu-latest gcc/clang, macos-latest clang,
  windows-latest msvc}，跑 `cmake --build` + `ctest`；额外一个 job 开 ASan/UBSan。
- CMake 安装/导出：`install(TARGETS novada EXPORT novada-targets ...)`、
  `novada-config.cmake.in` + `write_basic_package_version_file`，供下游
  `find_package(novada CONFIG)`；起草 vcpkg port（`portfile.cmake` + `vcpkg.json`）。
- **验收**：全平台 CI 绿；`cmake --install` 后一个独立示例工程能 `find_package(novada)` 成功链接。

## 风险与注意点
- **OpenAPI 是字段真相**：表单字段以 `novada-openapi.json` 的 requestBody 为准，逐字段核对必填/可选。
- **`code == 0` 才成功**：最容易踩的坑，不要用 HTTP 200 判断。
- **空值省略语义**：可选字段用 `std::optional`，`std::nullopt` 即不发送；避免用魔法值（0/空串）
  兼职"未设置"，除非该字段本身语义就是"0/空即未设置"（参照 Go 源码逐一核对）。
- **抓取响应不解 envelope**（除 Google Search 外），格式不定，原样返回。
- **重试边界**：业务 `code != 0` 永不重试；只重试 libcurl 传输层错误与 429/5xx。
- **libcurl 生命周期**：`curl_global_init` 全局只调一次且非线程安全（在 `main` 之前/库加载时
  用 `std::call_once` 保证）；每个请求用独立 easy handle，避免共享 `CURL*` 带来的线程安全问题。
- **multipart 编码**：优先用 `curl_mime_*`（libcurl 7.56+）而非手写 `CURLFORM_*`（已废弃）；
  空值字段整体跳过不加入 mime part。
- **依赖可得性**：`nlohmann_json`/`CURL` 在部分平台包管理器里名字或 CMake target 名不同
  （如 `nlohmann_json::nlohmann_json` vs `nlohmann_json`），`Dependencies.cmake` 需做兼容探测
  + FetchContent 兜底，避免下游 `find_package` 失败。
- **ABI/版本稳定性**：作为可分发库，公开头文件避免暴露 nlohmann::json 具体类型到 API 边界
  （只在内部使用，公开接口用 POD 结构体），减少下游 ABI 耦合。

## 参考同步脚本
仓库根的 `scripts/sync_reference.sh` 会把 Go 项目源码与 OpenAPI 拷入 `reference/`。
如 Go 项目路径不同，编辑脚本顶部的 `GO_SRC` 变量。
