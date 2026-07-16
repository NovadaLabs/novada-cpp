#pragma once

#include "novada/detail/transport.hpp"
#include "novada/scraper/scraper_service.hpp"
#include "novada/scraper/types.hpp"

namespace novada::scraper {

// YouTubeService groups YouTube scrapers (scraper_name "youtube.com"). Each
// method is a thin wrapper over the generic scrape pipeline with a known
// scraper_id.
class YouTubeService {
 public:
  explicit YouTubeService(const detail::Transport& transport) : transport_(&transport) {}

  // video_post scrapes a YouTube video post (scraper_id
  // "youtube_video-post_explore"). url is required. The response body format
  // is scraper-defined, so it is returned raw.
  [[nodiscard]] ScrapeResponse video_post(const YouTubeVideoParams& params) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::scraper
