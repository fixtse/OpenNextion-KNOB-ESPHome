#include "../my_components/onx_icons/onx_icons.cpp"
#include <cassert>
static uint32_t emoji_done_index=0;static uint8_t *emoji_done_pixels=nullptr;static int emoji_done_calls=0;
static void emoji_done(uint32_t index,uint8_t *pixels){emoji_done_index=index;emoji_done_pixels=pixels;emoji_done_calls++;}
struct Downloader : esphome::onx_icons::IconDownloader {
 size_t cached() const{return cache_.size();}
 bool busy() const{return busy_;}
};
int main(){
 uint8_t fallback_pixels[4]={255,255,255,255};
 esphome::image::Image fallback(fallback_pixels,1,1,esphome::image::IMAGE_TYPE_RGB,esphome::image::TRANSPARENCY_ALPHA_CHANNEL);
 lv_obj_t widget;
 Downloader download;
 assert(download.status("") == "missing_icon_metadata");
 assert(download.status("mdi:weather-fog") == "awaiting_display");
 stall_once=true;
 download.bind(&widget,"mdi:ceiling-fan",&fallback);
 assert(download.busy()&&widget.source==fallback.get_lv_image_dsc());
 finish_icon_task();download.loop();
 assert(download.cached()==1&&!download.busy());
 // The parser scratch allocation exceeds the typical available internal block.
 assert(largest_external_allocation > 32768);
 assert(external_allocations.size()==1); // Only the cached RGBA bitmap survives.
 assert(download.status("mdi:ceiling-fan") == "ready");
 download.bind(&widget,"mdi:ceiling-fan",&fallback);
 assert(widget.source!=fallback.get_lv_image_dsc());
 const auto count=requested_icon_urls.size();
 download.bind(&widget,"mdi:ceiling-fan",&fallback);assert(requested_icon_urls.size()==count);
 fail_primary=true;
 download.bind(&widget,"mdi:radiator",&fallback);finish_icon_task();download.loop();
 assert(download.cached()==2);
 assert(requested_icon_urls.back()==onx_icon_state::backup_url("mdi:radiator"));
 fail_all=true;
 download.bind(&widget,"mdi:music",&fallback);finish_icon_task();download.loop();
 assert(download.status("mdi:music") == "retry_wait"&&!download.error("mdi:music").empty());
 const auto failures=requested_icon_urls.size();
 download.bind(&widget,"mdi:music",&fallback);assert(!download.busy()&&requested_icon_urls.size()==failures);
 download.retry();download.bind(&widget,"mdi:music",&fallback);assert(download.busy());
 finish_icon_task();download.loop();
 download.bind(&widget,"mdi:../../secret",&fallback);assert(!download.busy());
 assert(widget.source==fallback.get_lv_image_dsc());
 // Emoji are fetched through the same worker after icons, and rendered in color.
 fail_all=false;fail_primary=false;
 onx_emoji_queue::completed=emoji_done;
 onx_emoji_queue::pending.push_back({7,"1f600"});
 download.loop();assert(download.busy()&&onx_emoji_queue::pending.empty());
 finish_icon_task();
 assert(requested_icon_urls.back()=="https://fonts.gstatic.com/s/e/notoemoji/latest/1f600/emoji.svg");
 download.loop();
 assert(!download.busy()&&emoji_done_calls==1&&emoji_done_index==7&&emoji_done_pixels);
 // Opaque white source survives the RGBA->BGRA swap.
 {const auto *c=emoji_done_pixels+(24*48+24)*4;assert(c[0]==255&&c[1]==255&&c[2]==255&&c[3]==255);}
 heap_caps_free(emoji_done_pixels);
 onx_emoji_queue::pending.push_back({8,"1f1f5_1f1ea"});
 download.loop();fail_all=true;finish_icon_task();
 assert(requested_icon_urls[requested_icon_urls.size()-2]=="https://fonts.gstatic.com/s/e/notoemoji/latest/1f1f5_1f1ea/emoji.svg"&&requested_icon_urls.back().find("waved-svg/emoji_u1f1f5_1f1ea.svg")!=std::string::npos);
 download.loop();
 assert(emoji_done_calls==2&&emoji_done_pixels==nullptr);
 onx_emoji_queue::pending.push_back({9,"../x"});
 download.loop();finish_icon_task();download.loop();
 assert(emoji_done_calls==3&&emoji_done_pixels==nullptr);
}
