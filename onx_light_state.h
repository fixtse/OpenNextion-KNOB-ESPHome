#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
namespace onx_light {
inline std::string array_json(std::string value) {
  const auto first=value.find_first_not_of(" \r\n\t"),last=value.find_last_not_of(" \r\n\t");
  if(first!=std::string::npos && value[first]=='(' && value[last]==')') {value[first]='[';value[last]=']';}
  return value;
}
inline uint32_t hsv(float hue,float saturation) {
  hue=std::fmod(std::max(0.0f,hue),360.0f)/60.0f;
  const float s=std::clamp(saturation/100.0f,0.0f,1.0f),x=s*(1-std::fabs(std::fmod(hue,2.0f)-1)),m=1-s;
  float r=0,g=0,b=0;
  switch(static_cast<int>(hue)) {
    case 0:r=s;g=x;break;case 1:r=x;g=s;break;case 2:g=s;b=x;break;
    case 3:g=x;b=s;break;case 4:r=x;b=s;break;default:r=s;b=x;break;
  }
  return (uint32_t(std::lround((r+m)*255))<<16)|(uint32_t(std::lround((g+m)*255))<<8)|uint32_t(std::lround((b+m)*255));
}
inline uint32_t kelvin(float value) {
  const float t=std::clamp(value,1000.0f,40000.0f)/100;
  const float r=t<=66?255:329.698727446f*std::pow(t-60,-0.1332047592f);
  const float g=t<=66?99.4708025861f*std::log(t)-161.1195681661f:288.1221695283f*std::pow(t-60,-0.0755148492f);
  const float b=t>=66?255:t<=19?0:138.5177312231f*std::log(t-10)-305.0447927307f;
  auto channel=[](float c){return uint32_t(std::lround(std::clamp(c,0.0f,255.0f)));};
  return (channel(r)<<16)|(channel(g)<<8)|channel(b);
}
inline std::string rgb_text(uint32_t color) {
  char text[8];std::snprintf(text,sizeof(text),"#%06X",unsigned(color&0xFFFFFF));return text;
}
}
