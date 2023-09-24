#ifndef STREAMCLEANER_HPP
#define STREAMCLEANER_HPP

#include <sstream>

class StreamCleaner {
 private:
  std::stringstream &rss, &wss;

 public:
  StreamCleaner(std::stringstream& rss, std::stringstream& wss)
      : rss(rss), wss(wss) {}
  ~StreamCleaner() {
    if (rss.bad() || wss.bad()) return;
    if (rss.eof()) {
      Log::debug("rss.eof(), so clear rss");
      rss.str("");
    }
    if (wss.eof()) {
      Log::debug("wss.eof(), so clear wss");
      wss.str("");
    }
    rss.clear();
    wss.clear();
  }
};

#endif
