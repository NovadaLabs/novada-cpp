# 任务：把 novada-go SDK 移植为一个 C++ 库（novada-cpp）

你要在当前空仓库里，参照一份成熟的 Go SDK，实现一个**功能等价、API 风格地道**
的 C++ SDK，用于 Novada API（代理管理 / 抓取 / 钱包）。用 CMake 构建，可通过
vcpkg/Conan/`find_package` 分发。

## 输入材料（必须先读）
1. Go 源码参考实现（权威设计来源），位于 `./reference/novada-go/`：
   - `client.go` `config.go` `transport.go` `errors.go` `envelope.go` `version.go`
   - `internal/transport/transport.go`（子服务依赖的接口）
   - `proxy/*.go` `scraper/*.go` `wallet/*.go`
   - `README.md` `novada-go-sdk-spec.md`（中文设计规范，含实施顺序）
2. OpenAPI 文档（接口字段的权威来源，用来生成参数与必填校验），位于 `./reference/openapi/`：
   - `novada-openapi.json`（51 个 /v1/* 管理接口）
   - `webunblocker_openapi.json`（Web Unblocker /request）
   - `Serpapi_openapi.json`（Google Search 等 scraper 参数）

   先把 Go 源码当作"形状"，把 OpenAPI 当作"字段事实来源"。两者冲突时以 OpenAPI 为准，
   并在代码注释里标注。

## 核心架构事实（不要重新发明）
- 鉴权：每个请求注入 `Authorization: Bearer <API_KEY>`。
- **三个 base URL**，全部可覆盖，默认指向生产：
  - 通用 `https://api-m.novada.com` —— 所有 `/v1/*`（proxy、wallet，以及 scraper 目录下查
    地区/余额/单价的 /v1/* 接口）
  - Web Unblocker `https://webunlocker.novada.com` —— 仅 Web Unblocker 的 `POST /request`
  - Scraper API `https://scraper.novada.com` —— 仅 Scraper API 的 `POST /request`
- **两套传输编码**：
  - `/v1/*` 管理接口：`multipart/form-data`，空值字段省略；响应是统一包裹。
  - 抓取 `/request`：`application/x-www-form-urlencoded`；字段 `scraper_name`、`scraper_id`、
    `scraper_params`（Params 列表序列化后的 JSON 字符串）、可选 `scraper_errors=true`。
- **统一响应包裹** `{code, data, msg, timestamp}`，**只有 `code == 0` 才算成功**。
  解包流程：HTTP 非 2xx → `ApiException(http_status)`；2xx 但 `code != 0` → `ApiException(code, msg)`；
  成功 → 把 `data` 解码并返回。
- **重试**：仅对网络错误（libcurl 传输层错误）和 HTTP 429/5xx 重试（线性退避，
  base 200ms × attempt），绝不对业务 `code != 0` 重试。重试次数 `max_retries` 默认 2。
- 列表接口的 data 形如 `{list:[...], total:N}`（个别还带 page/count）。
- 抓取 `/request` 的响应体是抓取产物本身（JSON/CSV/XLSX），格式不定 → 返回原始字符串，
  不走 envelope 解码。例外：Google Search 走 envelope 并结构化解码（见 sources.go）。

## C++ 化的设计决定（请严格执行）
- 标准：**C++17**，`CMake >= 3.16`，命名空间 `novada`。C++ 标准库没有 HTTP/JSON，
  对应 Go "仅标准库"的精神是：**只引入两个必需运行时依赖**：
  - **libcurl**（HTTP 传输，等价 Go 的 `net/http`；每次请求内部创建独立 `CURL*` easy handle，
    保证 `Client` 本身无可变共享状态、可多线程安全复用）。
  - **nlohmann/json**（JSON 编解码，等价 Jackson/Go 的 `encoding/json`）。
  依赖通过 CMake `find_package`，并提供 `cmake/Dependencies.cmake` 用 `FetchContent` 兜底
  （找不到系统包时自动拉取源码构建）。
- 命名约定（明确执行，避免风格漂移）：**类型用 PascalCase**（`Client`、`WhitelistService`、
  `ApiException`），**函数/方法用 snake_case**（`video_post`、`rotating_isp`、`usage_record`，
  对齐标准库/nlohmann::json 自身风格），**私有成员用尾下划线**（`base_url_`）。
- 顶层入口（`ClientOptions` 用带默认成员初始值的聚合体 + 链式 setter，对应 Go 的 functional options）：

      #include <novada/client.hpp>

      novada::ClientOptions opts;
      opts.set_base_url("...").set_web_unblocker_url("...").set_scraper_url("...")
          .set_timeout(std::chrono::seconds(30)).set_max_retries(2)
          .set_user_agent("...");
      novada::Client client("API_KEY", opts);     // 空字符串则回退读环境变量 NOVADA_API_KEY
      novada::Client client2;                     // 全部走默认值 + 环境变量

  无 key 且环境变量也空 → 抛 `novada::NovadaException`（对应 Go 的 ErrNoAPIKey）。
- 子服务通过访问方法返回引用挂载（`Client` 内部持有各子服务的 `std::unique_ptr` 成员）：

      client.proxy().whitelist().list(...) / .add(...) / .del(...) / .remark(...)
      client.proxy().account() / .residential() / .mobile() / .rotating_isp() / .rotating_dc() /
                     .static_isp() / .dedicated_dc() / .unlimited() / .prohibit_domain()
      client.scraper().do_request(req) / .api().youtube().video_post(...) /
                       .api().google().search(...) / .unblocker().scrape(...) /
                       .unblocker().countries() / .universal().balance() / .universal().unit() /
                       .browser().countries()
      client.wallet().balance() / .wallet().usage_record(...)

  （`do` 不是 C++ 保留字冲突的问题所在，但为跨语言一致性沿用 `do_request` 命名。）
- 每个方法的参数用**不可变（或按值传参）POD 结构体**（与 Go 的 `*Params` struct 一一对应）：
  必填字段用普通类型，可选字段一律 `std::optional<T>`（直接对应 Go 的 `*int`/`*bool` 与
  "零值省略"语义——`std::nullopt` 就是"不发送"）。必填字段缺失（空字符串、`Product` 未设置等）
  在发请求前抛 `ValidationException`（列出所有缺失字段名，对应 Go 的 validator）。
- 枚举：`Product` 用 **`enum class Product : int`**，值固定（1 住宅 / 2 RotatingISP /
  3 RotatingDC / 4 Unlimited / 5 StaticISP / 7 Unblocker / 9 Mobile / 10 BrowserAPI）。
- 返回值：管理接口的 data 反序列化为 POD 结构体，字段名直接对齐 JSON 的 snake_case key
  （C++ 成员天然可用 snake_case，无需再映射）。**不要用 `NLOHMANN_DEFINE_TYPE_INTRUSIVE` 宏**
  （它对缺失字段会抛异常）；手写 `from_json(const json&, T&)`，一律用 `j.value("key", default)`
  取值以兼容字段缺失/新增（向前兼容）。抓取 `do_request()` 返回 `struct ScrapeResponse { std::string raw; };`。
- 异常类型（在 `include/novada/errors.hpp`，全部 `public std::runtime_error` 派生）：

      class NovadaException : public std::runtime_error { ... };            // 基类
      class ApiException : public NovadaException {                        // http_status()/code()/message()
        int http_status_; int code_; std::string message_;
      };
      class AuthException : public ApiException { ... };                   // http 401/403
      class RateLimitException : public ApiException { ... };              // http 429
      class ValidationException : public NovadaException {                 // method()/fields()
        std::string method_; std::vector<std::string> fields_;
      };

  提供辅助函数 `bool is_auth_error(const ApiException&)`、`bool is_rate_limited(const ApiException&)`
  （对应 Go 的 IsAuthError/IsRateLimited；`ApiException::code()` 对应 CodeOf）。

## 目录结构（建议）

    CMakeLists.txt                # project()、C++17、find_package(CURL) / (nlohmann_json)
    cmake/Dependencies.cmake      # find_package 失败时 FetchContent 兜底
    cmake/novada-config.cmake.in  # 供下游 find_package(novada CONFIG) 使用
    README.md
    LICENSE                       # MIT，与 Go 版一致
    include/novada/
      client.hpp                  # Client / ClientOptions
      version.hpp                 # inline constexpr const char* kVersion = "0.1.0";
      errors.hpp
      detail/
        transport.hpp             # do_multipart / do_form_urlencoded（+ _raw 变体）声明
        envelope.hpp               # envelope 解码 + list 解包
      proxy/
        proxy_service.hpp whitelist_service.hpp account_service.hpp
        residential_service.hpp mobile_service.hpp rotating_service.hpp
        static_isp_service.hpp dedicated_dc_service.hpp unlimited_service.hpp
        prohibit_domain_service.hpp
        types.hpp                 # 各 Params / 返回结构体 / Product 枚举
      scraper/
        scraper_service.hpp        # Target 枚举、ScrapeRequest/ScrapeResponse、do_request()
        api_service.hpp youtube_service.hpp google_service.hpp
        unblocker_service.hpp universal_service.hpp browser_service.hpp
        types.hpp
      wallet/
        wallet_service.hpp
    src/                           # 对应 .cpp 实现（含 curl easy handle 的 RAII 封装）
      client.cpp detail/transport.cpp detail/envelope.cpp
      proxy/*.cpp scraper/*.cpp wallet/*.cpp
    tests/                         # GoogleTest
    examples/proxy.cpp examples/scraper.cpp examples/wallet.cpp
    .github/workflows/ci.yml

## 测试与 CI
- 用 **GoogleTest** + 一个**本地 HTTP mock server**：用 **cpp-httplib**（header-only，仅作为
  test-only 依赖，不进运行时依赖）在测试进程内起一个真实的本地 HTTP 服务，模拟
  `{code,data,msg,timestamp}` 响应——这与其它语言版（respx/msw/mock-client/httptest.Server）
  同一思路：在真实 HTTP 语义层面校验编码，而不是 mock 掉传输层抽象。
  必须覆盖：code!=0 → ApiException；HTTP 401/403/429/5xx 映射到对应异常；multipart 与
  urlencoded 的编码正确（含 scraper_params 的 JSON 序列化 + 空值省略）；Bearer 注入；
  list 解包；重试逻辑（429→重试、业务 code!=0→不重试）；ValidationException 在发请求前触发；
  curl easy handle 的 RAII（无泄漏，可用 `-fsanitize=address` 验证）。
- 集成测试用环境变量 `NOVADA_API_KEY` 控制是否运行（缺失则 `GTEST_SKIP()`）。
- CI：GitHub Actions，矩阵 {Linux gcc/clang, macOS clang, Windows MSVC}，跑
  `cmake --build` + `ctest`，并至少一个 job 开 `-fsanitize=address,undefined`。
- 静态检查/格式化：**clang-format**（.clang-format，类比 gofmt）+ **clang-tidy**
  （类比 golangci-lint）。

## 实施顺序（务必分步，每步先跑通再继续；详见 PLAN.md）
1. 脚手架（CMakeLists.txt/Dependencies.cmake/clang-format/clang-tidy/GoogleTest 接入）+
   顶层 `Client`/`ClientOptions` + `detail/transport` + `detail/envelope` + `errors`，
   用 cpp-httplib 起本地 server 跑通一个最简管理接口（white_list/list）验证解包与 Bearer。
2. Proxy WhitelistService + AccountService（最简单 CRUD），配单测，确立 multipart +
   `std::optional` 参数模式。
3. 按 OpenAPI 批量铺开 proxy 其余子服务（用 requestBody properties 生成 Params 与必填校验）。
4. Scraper `do_request` 通用驱动（urlencoded + scraper_params JSON），单测验证编码。
5. Scraper `api().youtube().video_post` / `api().google().search`（结构化）/
   `unblocker().scrape` + 各 `/v1/*` 查询接口。
6. Wallet。
7. README + examples + CI + CMake 安装/导出（`install(EXPORT ...)`、`novada-config.cmake`）
   + vcpkg port 草稿。

## 约束
- 运行时只依赖 **libcurl** + **nlohmann/json**；不要引入 Boost.Beast/cpprestsdk/POCO 等重量级替代。
- 不要把 HTTP 200 当成功——一律以 `code == 0` 判定。
- 保持与 Go 版 README 等价的文档示例（同样的三个 baseURL 表、错误处理示例）。
- 内存/资源管理全程 RAII（`unique_ptr`/自定义删除器包 `CURL*`），不手写裸 `new`/`delete`；
  确保 ASan/UBSan 干净。
- 线程安全：`Client` 与子服务不持有跨请求的可变共享状态（每请求独立 easy handle），
  可安全被多线程复用（对应 Go 的并发安全说明）。
- 每完成一步跑 `clang-format --dry-run --Werror`、`clang-tidy`、`ctest`，全绿再进入下一步。
