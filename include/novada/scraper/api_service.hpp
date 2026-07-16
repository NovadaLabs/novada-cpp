#pragma once

#include "novada/detail/transport.hpp"
#include "novada/scraper/google_service.hpp"
#include "novada/scraper/youtube_service.hpp"

namespace novada::scraper {

// APIService groups strongly typed Scraper API scrapers (target =
// kScraperAPI). Reach the per-site scrapers through its accessor methods,
// e.g. client.scraper().api().youtube().video_post(...).
class APIService {
 public:
  explicit APIService(const detail::Transport& transport)
      : youtube_(transport), google_(transport) {}

  // youtube exposes YouTube scrapers.
  [[nodiscard]] const YouTubeService& youtube() const { return youtube_; }
  // google exposes Google scrapers (SerpApi).
  [[nodiscard]] const GoogleService& google() const { return google_; }

 private:
  YouTubeService youtube_;
  GoogleService google_;
};

}  // namespace novada::scraper
