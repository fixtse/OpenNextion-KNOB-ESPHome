#include "../my_components/onx_icons/onx_icons.cpp"
#include <cassert>
struct Downloader : esphome::onx_icons::IconDownloader {
 size_t cached() const{return cache_.size();}
 bool busy() const{return busy_;}
};
int main(){
 uint8_t fallback_pixels[4]={255,255,255,255};
 esphome::image::Image fallback(fallback_pixels,1,1,esphome::image::IMAGE_TYPE_RGB,esphome::image::TRANSPARENCY_ALPHA_CHANNEL);
 lv_obj_t widget;
 Downloader download;
 stall_once=true;
 download.bind(&widget,"mdi:ceiling-fan",&fallback);
 assert(download.busy()&&widget.source==fallback.get_lv_image_dsc());
 finish_icon_task();download.loop();
 assert(download.cached()==1&&!download.busy());
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
 const auto failures=requested_icon_urls.size();
 download.bind(&widget,"mdi:music",&fallback);assert(!download.busy()&&requested_icon_urls.size()==failures);
 download.retry();download.bind(&widget,"mdi:music",&fallback);assert(download.busy());
 finish_icon_task();download.loop();
 download.bind(&widget,"mdi:../../secret",&fallback);assert(!download.busy());
 assert(widget.source==fallback.get_lv_image_dsc());
}
