#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome::online_image {
// Bound decoder work before it allocates buffers for the source dimensions.
inline bool source_pixels_within_limit(const uint8_t *data,size_t size,uint32_t limit) {
  if(!limit)return true;
  auto fits=[&](uint32_t width,uint32_t height) {
    return width && height && static_cast<uint64_t>(width)*height<=limit;
  };
  auto be32=[](const uint8_t *p) {return uint32_t(p[0])<<24 | uint32_t(p[1])<<16 | uint32_t(p[2])<<8 | p[3];};
  auto le32=[](const uint8_t *p) {return uint32_t(p[3])<<24 | uint32_t(p[2])<<16 | uint32_t(p[1])<<8 | p[0];};
  if(size>=24 && std::memcmp(data,"\x89PNG\r\n\x1a\n",8)==0 && std::memcmp(data+12,"IHDR",4)==0)
    return fits(be32(data+16),be32(data+20));
  if(size>=26 && data[0]=='B' && data[1]=='M') {
    const uint32_t dib=le32(data+14);
    if(dib==12)return fits(uint32_t(data[18]) | uint32_t(data[19])<<8,uint32_t(data[20]) | uint32_t(data[21])<<8);
    if(dib<40 || size<54)return false;
    const int32_t width=static_cast<int32_t>(le32(data+18)),height=static_cast<int32_t>(le32(data+22));
    return width>0 && fits(width,height<0?uint32_t(-int64_t(height)):uint32_t(height));
  }
  if(size>=4 && data[0]==0xFF && data[1]==0xD8) {
    size_t at=2;
    while(at<size) {
      if(data[at++]!=0xFF)return false;
      while(at<size && data[at]==0xFF)++at;
      if(at>=size)return false;
      const uint8_t marker=data[at++];
      if(marker==0xDA || marker==0xD9)return false;
      if(marker==0x01 || (marker>=0xD0 && marker<=0xD7))continue;
      if(size-at<2)return false;
      const size_t length=size_t(data[at])<<8 | data[at+1];
      if(length<2 || length>size-at)return false;
      if(marker>=0xC0 && marker<=0xCF && marker!=0xC4 && marker!=0xC8 && marker!=0xCC)
        return length>=8 && fits(uint32_t(data[at+5])<<8 | data[at+6],uint32_t(data[at+3])<<8 | data[at+4]);
      at+=length;
    }
  }
  return false;
}
}
