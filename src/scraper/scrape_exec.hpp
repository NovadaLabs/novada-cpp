#pragma once

#include "novada/scraper/scraper_service.hpp"

namespace novada::detail {
class Transport;
}  // namespace novada::detail

namespace novada::scraper::internal {

// execute_scrape runs the generic scrape pipeline shared by
// ScraperService::do_request and the strongly typed helpers (e.g.
// YouTubeService::video_post): it validates the required fields, serializes
// params into scraper_params, URL-encodes the form, routes to the host
// selected by req.target, and returns the raw response body without envelope
// decoding.
ScrapeResponse execute_scrape(const novada::detail::Transport& transport, const ScrapeRequest& req);

}  // namespace novada::scraper::internal
