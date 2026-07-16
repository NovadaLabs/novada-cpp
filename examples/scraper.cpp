// scraper.cpp — run a Google search scrape and a Web Unblocker scrape.
//
// Reads the API key from the NOVADA_API_KEY environment variable. Build with
// -DNOVADA_BUILD_EXAMPLES=ON.

#include <iostream>

#include <novada/client.hpp>
#include <novada/errors.hpp>
#include <novada/scraper/api_service.hpp>
#include <novada/scraper/scraper_service.hpp>
#include <novada/scraper/unblocker_service.hpp>

int main() {
  try {
    novada::Client client;

    // Structured Google Search scrape.
    novada::scraper::GoogleSearchParams google;
    google.query = "novada proxy";
    google.country = "us";
    auto result = client.scraper().api().google().search(google);
    std::cout << "google code=" << result.code << " cost_time=" << result.cost_time << "ms\n";
    std::cout << "  task_id=" << result.data.task_id << "\n";

    // Web Unblocker scrape — returns the rendered HTML.
    novada::scraper::UnblockerParams unblock;
    unblock.target_url = "https://example.com";
    unblock.js_render = true;
    auto page = client.scraper().unblocker().scrape(unblock);
    std::cout << "unblocker code=" << page.code << " html_bytes=" << page.html.size()
              << " use_balance=" << page.use_balance << "\n";

    // Generic scrape via do_request (any scraper_id).
    novada::scraper::ScrapeRequest req;
    req.target = novada::scraper::Target::kScraperAPI;
    req.scraper_name = "youtube.com";
    req.scraper_id = "youtube_video-post_explore";
    req.params = {{{"url", "https://youtu.be/dQw4w9WgXcQ"}}};
    auto raw = client.scraper().do_request(req);
    std::cout << "youtube raw_bytes=" << raw.raw.size() << "\n";
    return 0;
  } catch (const novada::ApiException& e) {
    std::cerr << "API error (http=" << e.http_status() << ", code=" << e.code()
              << "): " << e.message() << "\n";
    return 1;
  } catch (const novada::NovadaException& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
}
