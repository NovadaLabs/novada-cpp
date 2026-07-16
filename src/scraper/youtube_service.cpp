#include "novada/scraper/youtube_service.hpp"

#include "scraper/scrape_exec.hpp"

namespace novada::scraper {

ScrapeResponse YouTubeService::video_post(const YouTubeVideoParams& params) const {
  ScrapeRequest req;
  req.target = Target::kScraperAPI;
  req.scraper_name = "youtube.com";
  req.scraper_id = "youtube_video-post_explore";
  req.params = {{{"url", params.url}}};
  return internal::execute_scrape(*transport_, req);
}

}  // namespace novada::scraper
