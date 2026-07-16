# novada-cpp

[![CI](https://github.com/NovadaLabs/novada-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/NovadaLabs/novada-cpp/actions/workflows/ci.yml)

C++ SDK for the [Novada](https://novada.com) API — proxy management, scraping, and
wallet endpoints. C++17, built with CMake. Two runtime dependencies only:
[libcurl](https://curl.se/libcurl/) (HTTP) and
[nlohmann/json](https://github.com/nlohmann/json) (JSON).

This is a faithful port of the Go SDK [`novada-go`](https://github.com/NovadaLabs/novada-go):
same three-host routing, the same uniform envelope, and the same
"`code == 0` is the only success" contract.

## Install

### CMake `find_package`

Install the library (see [Building](#building-from-source)) and consume it with:

```cmake
find_package(novada CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE novada::novada)
```

### FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
  novada
  GIT_REPOSITORY https://github.com/NovadaLabs/novada-cpp.git
  GIT_TAG v0.1.0)
FetchContent_MakeAvailable(novada)
target_link_libraries(your_target PRIVATE novada::novada)
```

### vcpkg

A port draft lives in [`packaging/vcpkg/`](packaging/vcpkg/) (`vcpkg.json` +
`portfile.cmake`). Point an [overlay port](https://learn.microsoft.com/vcpkg/concepts/overlay-ports)
at it, then `vcpkg install novada`.

## Quick start

```cpp
#include <iostream>
#include <novada/client.hpp>
#include <novada/errors.hpp>
#include <novada/proxy/proxy_service.hpp>

int main() {
  novada::Client client("YOUR_API_KEY");  // or "" to read NOVADA_API_KEY

  novada::proxy::ListWhitelistParams params;
  params.product = novada::proxy::Product::kResidential;
  auto list = client.proxy().whitelist().list(params);
  std::cout << "whitelist total=" << list.total << "\n";
}
```

The API key is resolved from the constructor argument first, then the
`NOVADA_API_KEY` environment variable; if both are empty the constructor throws
`novada::NovadaException`.

## The three base URLs

Novada serves requests from **three** hosts. All are configurable and default to
production:

| Purpose       | Default                          | Used by |
|---------------|----------------------------------|---------|
| General       | `https://api-m.novada.com`       | Every `/v1/*` endpoint (proxy, wallet, and the scraper area/balance/unit queries) |
| Web Unblocker | `https://webunlocker.novada.com` | `scraper().unblocker().scrape(...)`, or `scraper().do_request(...)` with `Target::kWebUnblocker` |
| Scraper API   | `https://scraper.novada.com`     | `scraper().do_request(...)` / `scraper().api().*` with `Target::kScraperAPI` |

Only the scrape `POST /request` calls go to the Web Unblocker / Scraper API
hosts; everything else (`/v1/*`) uses the general host.

```cpp
novada::ClientOptions opts;
opts.set_base_url("https://api-m.novada.com")
    .set_web_unblocker_url("https://webunlocker.novada.com")
    .set_scraper_url("https://scraper.novada.com")
    .set_timeout(std::chrono::seconds(30))
    .set_max_retries(2)
    .set_user_agent("my-app/1.0");
novada::Client client("API_KEY", opts);
```

The Bearer API key is injected on every request. Retries apply only to network
errors and HTTP 429/5xx — **never** to a non-zero business code. Each request
uses its own libcurl handle, so a `Client` is safe to share across threads.

## Proxy

```cpp
#include <novada/proxy/proxy_service.hpp>

// Sub-accounts
novada::proxy::CreateAccountParams create;
create.product = novada::proxy::Product::kResidential;
create.account = "account11";
create.password = "pass11";
create.status = 1;
client.proxy().account().create(create);

novada::proxy::ListAccountParams list_accounts;
list_accounts.product = novada::proxy::Product::kResidential;
auto accounts = client.proxy().account().list(list_accounts);

// Whitelist
client.proxy().whitelist().add(
    novada::proxy::AddWhitelistParams{novada::proxy::Product::kResidential, "10.10.10.1", "test"});

// Residential areas & traffic
client.proxy().residential().countries();
client.proxy().residential().balance();
client.proxy().residential().consume_log(
    novada::proxy::TimeRange{"2025-01-01 00:00:00", "2025-01-31 23:59:59"});

// Static ISP / dedicated datacenter
client.proxy().static_isp().open(
    novada::proxy::OpenStaticISPParams{"normal", "hk:1|us-va:2", "week", 3});
auto dc = client.proxy().dedicated_dc().list(novada::proxy::ListStaticParams{});
```

Sub-services: `account()`, `whitelist()`, `residential()`, `mobile()`,
`rotating_isp()`, `rotating_dc()`, `static_isp()`, `dedicated_dc()`,
`unlimited()`, `prohibit_domain()`. Required parameters are validated
client-side and reported as a `novada::ValidationException` before any request
is sent.

## Scraper

```cpp
#include <novada/scraper/api_service.hpp>
#include <novada/scraper/scraper_service.hpp>
#include <novada/scraper/unblocker_service.hpp>

// Strongly typed (auto-selects the Scraper API host)
auto yt = client.scraper().api().youtube().video_post(
    novada::scraper::YouTubeVideoParams{"https://www.youtube.com/watch?v=HAwTwmzgNc4"});
// yt.raw is the raw scrape body (format varies by scraper)

// Google Search (SerpApi) — typed params, structured result
novada::scraper::GoogleSearchParams gs_params;
gs_params.query = "apple";
gs_params.country = "us";
auto gs = client.scraper().api().google().search(gs_params);
// gs.code, gs.cost_time, gs.data.json (raw result array as JSON text)

// Generic driver — any scraper_id, choose the host explicitly
novada::scraper::ScrapeRequest req;
req.target = novada::scraper::Target::kScraperAPI;  // or kWebUnblocker
req.scraper_name = "youtube.com";
req.scraper_id = "youtube_video-post_explore";
req.params = {{{"url", "https://www.youtube.com/watch?v=HAwTwmzgNc4"}}};
req.return_errors = true;
auto raw = client.scraper().do_request(req);  // raw.raw is the scrape body

// Web Unblocker — typed scrape; returns a structured result
novada::scraper::UnblockerParams unb_params;
unb_params.target_url = "https://www.google.com";  // required
unb_params.country = "us";                          // response_format defaults to "html"
auto unb = client.scraper().unblocker().scrape(unb_params);
// unb.code, unb.html, unb.use_balance

// Query endpoints on the general host
client.scraper().universal().balance();    // /v1/capture/get_balance
client.scraper().universal().unit();       // /v1/capture/unit
client.scraper().unblocker().countries();  // /v1/proxy/unblocker_area
client.scraper().browser().countries();    // /v1/proxy/browser_area
```

`do_request()` serializes `params` to JSON, places it in the `scraper_params`
form field, URL-encodes the body, and routes to the host selected by `target`.
Scrape responses are returned raw because their format varies by scraper.
`unblocker().scrape()` is the dedicated Web Unblocker call: it sends the
endpoint's own fields (`target_url`, `response_format`, `js_render`, `country`,
`wait_ms`, …) and decodes the JSON envelope into a `UnblockerResult`.

## Wallet

```cpp
#include <novada/wallet/wallet_service.hpp>

auto balance = client.wallet().balance();
auto usage = client.wallet().usage_record(/*page=*/1, /*limit=*/20);
```

## Error handling

Management endpoints return a uniform envelope `{code, data, msg, timestamp}`;
**only `code == 0` is success**. A non-zero code or a non-2xx HTTP status
becomes a `novada::ApiException` (or one of its subclasses `AuthException` /
`RateLimitException`). Missing required parameters raise a
`novada::ValidationException` before any request is sent. All of these derive
from `novada::NovadaException`.

```cpp
try {
  auto list = client.proxy().whitelist().list(params);
} catch (const novada::ValidationException& e) {
  std::cerr << "missing fields for " << e.method() << "\n";
} catch (const novada::AuthException& e) {  // HTTP 401/403
  std::cerr << "invalid API key\n";
} catch (const novada::RateLimitException& e) {  // HTTP 429
  std::cerr << "rate limited\n";
} catch (const novada::ApiException& e) {
  std::cerr << "api error code=" << e.code() << " http=" << e.http_status()
            << ": " << e.message() << "\n";
}
```

The free helpers `novada::is_auth_error(e)` and `novada::is_rate_limited(e)` are
also available for callers that catch `ApiException` broadly.

## Building from source

Requires CMake ≥ 3.16, a C++17 compiler, libcurl, and nlohmann/json. When the
system packages are missing, `cmake/Dependencies.cmake` fetches them via
`FetchContent`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure   # runs the test suite
cmake --install build --prefix /your/prefix   # installs headers + CMake package
```

Options: `NOVADA_BUILD_TESTS`, `NOVADA_BUILD_EXAMPLES`, `NOVADA_INSTALL`
(all default ON for a top-level build), and `NOVADA_ENABLE_SANITIZERS`
(ASan/UBSan; Linux/macOS).

## Examples

Runnable examples live in [`examples/`](examples/): `proxy`, `scraper`,
`wallet`. Configure with `-DNOVADA_BUILD_EXAMPLES=ON`, set `NOVADA_API_KEY`, and
run the built binaries (`build/examples/proxy`, etc.).

## License

[MIT](LICENSE)
