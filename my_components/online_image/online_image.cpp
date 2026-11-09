// SPDX-License-Identifier: MIT
#include "online_image.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#ifdef USE_RUNTIME_IMAGE_JPEG
#include "esphome/components/runtime_image/jpeg_decoder.h"
#endif
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <new>

namespace esphome::online_image {
static const char *const TAG="online_image";
static constexpr size_t MAX_BYTES=512*1024;
static constexpr uint32_t WORKER_STACK_BYTES=12*1024;
#ifdef USE_RUNTIME_IMAGE_JPEG
// JPEGDEC embeds its MCU/Huffman workspaces in the decoder object. Keep that
// large object out of the fragmented internal heap; default delete must match.
class PsramJpegDecoder final : public runtime_image::JpegDecoder {
 public:
  using runtime_image::JpegDecoder::JpegDecoder;
  static void *operator new(size_t size,const std::nothrow_t &) noexcept {
    return heap_caps_malloc(size,MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  static void operator delete(void *p) noexcept { heap_caps_free(p); }
  static void operator delete(void *p,const std::nothrow_t &) noexcept { heap_caps_free(p); }
};
#endif
bool OnlineImage::memory_ready() {
  // Artwork has a 12 KB worker, unlike the icon rasterizer's 16 KB worker.
  // Compressed data, decoded pixels, JPEG workspace and TLS use PSRAM.
  return heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)>=32*1024 &&
      heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)>=WORKER_STACK_BYTES+2048;
}
OnlineImage::OnlineImage(const std::string &url,int width,int height,runtime_image::ImageFormat format,
                         image::ImageType type,image::Transparency transparency,image::Image *placeholder,
                         uint32_t buffer_size,bool is_big_endian)
    : RuntimeImage(format,type,transparency,placeholder,is_big_endian,width,height) {
  (void) buffer_size;
  set_url(url);
}
bool OnlineImage::validate_url_(const std::string &url) {
  return url.size()<=2048 && (url.rfind("http://",0)==0 || url.rfind("https://",0)==0);
}
bool OnlineImage::cancelled_(uint32_t generation) {
  LockGuard lock(mutex_);
  return generation!=generation_;
}
void OnlineImage::update() {
  if(busy_ || is_decoding()) return;
  last_error_.clear();
  if(!memory_ready()) {last_error_="Waiting for artwork worker memory";failed_.call();return;}
  if(!validate_url_(url_)) { last_error_="Invalid artwork URL";failed_.call(); return; }
  auto *work=new(std::nothrow) Work{this,url_,{},generation_,2500};
  if(!work) { last_error_="Request allocation failed";failed_.call(); return; }
  if(parent_) work->timeout=std::min<uint32_t>(2500,parent_->get_timeout());
  for(auto &header:request_headers_) work->headers.push_back({header.first,header.second.value()});
  busy_=true;
  // All DNS, TCP, TLS, redirects and body reads happen off the ESPHome/UI task.
  if(xTaskCreate(fetch_task_,"onx_artwork",WORKER_STACK_BYTES,work,1,nullptr)!=pdPASS) {
    busy_=false;delete work;last_error_="Worker allocation failed";failed_.call();
  }
}
void OnlineImage::fetch_task_(void *arg) {
  std::unique_ptr<Work> work(static_cast<Work *>(arg));
  uint8_t *data=nullptr;
  size_t used=0,capacity=0;
  std::string error;
  esp_http_client_config_t config{};
  config.url=work->url.c_str();
  config.timeout_ms=work->timeout;
  config.crt_bundle_attach=esp_crt_bundle_attach;
  config.disable_auto_redirect=true;
  config.buffer_size=2048;
  config.user_agent="ONX2424G013/artwork";
  auto client=esp_http_client_init(&config);
  if(!client) error="HTTP allocation failed";
  const uint32_t started=millis();
  bool okay=false;
  if(client) {
    esp_http_client_set_header(client,"Accept","image/jpeg,image/png,image/bmp");
    esp_http_client_set_header(client,"Accept-Encoding","identity");
    for(const auto &header:work->headers) esp_http_client_set_header(client,header.name.c_str(),header.value.c_str());
    bool opened=false;
    int64_t length=-1;
    for(unsigned redirects=0;redirects<=3;++redirects) {
      if(work->owner->cancelled_(work->generation) || millis()-started>=15000) { error="Cancelled or timed out";break; }
      const auto opened_status=esp_http_client_open(client,0);
      if(opened_status!=ESP_OK) {error=std::string("Connection failed: ")+esp_err_to_name(opened_status);break;}
      length=esp_http_client_fetch_headers(client);
      const int status=esp_http_client_get_status_code(client);
      if(status==200) {opened=true;break;}
      if(redirects<3 && (status==301 || status==302 || status==303 || status==307 || status==308)) {
        if(esp_http_client_set_redirection(client)!=ESP_OK) {error="Invalid redirect";break;}
        esp_http_client_close(client);continue;
      }
      error="HTTP "+std::to_string(status);break;
    }
    if(opened) {
      if(length>static_cast<int64_t>(MAX_BYTES)) error="Artwork exceeds 512 KB";
      else {
        capacity=length>0?static_cast<size_t>(length):MAX_BYTES;
        data=static_cast<uint8_t *>(heap_caps_malloc(capacity,MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if(!data) error="Artwork PSRAM allocation failed";
        else {
          while(millis()-started<15000 && !work->owner->cancelled_(work->generation)) {
            if(used==capacity) {
              if(esp_http_client_is_complete_data_received(client)) okay=true;
              else error="Artwork exceeds response bound";
              break;
            }
            const int count=esp_http_client_read(client,reinterpret_cast<char *>(data+used),std::min<size_t>(2048,capacity-used));
            if(count<0) {error="Artwork read failed";break;}
            used+=static_cast<size_t>(count);
            if(esp_http_client_is_complete_data_received(client)) {okay=used>0;break;}
            vTaskDelay(1);
          }
          if(!okay && error.empty()) error="Artwork cancelled or timed out";
        }
      }
    }
    // Destroy the connection before handing bytes or any callback to the UI.
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
  }
  if(!okay) {heap_caps_free(data);data=nullptr;used=0;}
  {
    LockGuard lock(work->owner->mutex_);
    work->owner->completed_generation_=work->generation;
    work->owner->completed_data_=data;
    work->owner->completed_size_=used;
    work->owner->completed_error_=std::move(error);
    work->owner->completed_=true;
  }
  work.reset();
  vTaskDelete(nullptr);
}
void OnlineImage::loop() {
  uint8_t *data;
  size_t size;
  uint32_t generation;
  std::string error;
  {
    LockGuard lock(mutex_);
    if(!completed_) return;
    data=completed_data_;size=completed_size_;generation=completed_generation_;
    error=std::move(completed_error_);
    completed_data_=nullptr;completed_=false;
  }
  busy_=false;
  if(generation!=generation_) {heap_caps_free(data);return;}
  if(!data) {last_error_=error;ESP_LOGW(TAG,"Artwork unavailable: %s",error.c_str());failed_.call();return;}
  runtime_image::ImageFormat format=get_format();
  if(format==runtime_image::AUTO) {
    if(size>=3 && data[0]==0xFF && data[1]==0xD8 && data[2]==0xFF) format=runtime_image::JPEG;
    else if(size>=8 && std::memcmp(data,"\x89PNG\r\n\x1a\n",8)==0) format=runtime_image::PNG;
    else if(size>=2 && data[0]=='B' && data[1]=='M') format=runtime_image::BMP;
  }
#ifdef USE_RUNTIME_IMAGE_JPEG
  if(format==runtime_image::JPEG && (!decoder_ || decoder_->get_format()!=runtime_image::JPEG)) {
    auto *jpeg=new(std::nothrow) PsramJpegDecoder(this);
    if(!jpeg) {
      heap_caps_free(data);last_error_="JPEG workspace PSRAM allocation failed";
      failed_.call();return;
    }
    decoder_.reset(jpeg);
  }
#endif
  bool okay=false;
  if(format!=runtime_image::AUTO && begin_decode(size,format)) {
    size_t offset=0;
    while(offset<size) {
      int consumed=feed_data(data+offset,size-offset);
      if(consumed<=0 || static_cast<size_t>(consumed)>size-offset) break;
      offset+=static_cast<size_t>(consumed);
      App.feed_wdt();
      if(is_decode_finished()) break;
    }
    okay=is_decode_finished() && end_decode();
  }
  heap_caps_free(data);
  if(okay) {ESP_LOGI(TAG,"Artwork loaded (%u bytes)",static_cast<unsigned>(size));finished_.call(false);}
  else {last_error_="Artwork decode failed (unsupported or invalid image)";RuntimeImage::release();ESP_LOGW(TAG,"Artwork decode failed");failed_.call();}
}
void OnlineImage::release() {
  {
    LockGuard lock(mutex_);
    ++generation_;
  }
  // Worker keeps its own request snapshot and discards cancelled generations.
  // Only the UI task owns the decoder, pixels, and LVGL descriptors.
  RuntimeImage::release();
}
}
