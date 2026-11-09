#include "../onx_media_state.h"
#include <cassert>
int main() {
  assert(onx_media::volume_percent(0.60000004f) == "60%");
  assert(onx_media::volume_percent(0.59999996f) == "60%");
  assert(onx_media::volume_percent(0.555f) == "56%");
  assert(onx_media::volume_percent(-1) == "0%" && onx_media::volume_percent(2) == "100%");
  assert(onx_media::volume_percent(NAN) == "--");
  using namespace onx_media;
  for (const auto &state : {"playing","paused","buffering"}) assert(has_playback(state));
  for (const auto &state : {"idle","off","on","unknown","unavailable"}) assert(!has_playback(state));
  assert(can_skip("playing",32,false)&&!can_skip("playing",32,true));
  assert(can_skip("paused",16,true)&&!can_skip("idle",48,false));

  TransportFeedback feedback;
  assert(feedback.view("media_player.room","paused",100)==Overlay::PAUSE);
  feedback.show("media_player.room",false,100);
  assert(feedback.view("media_player.room","paused",1099)==Overlay::NEXT);
  assert(feedback.view("media_player.room","paused",1100)==Overlay::PAUSE);
  feedback.show("media_player.room",true,2000);
  assert(feedback.view("media_player.room","playing",2000)==Overlay::PREVIOUS);
  assert(feedback.view("media_player.room","playing",3000)==Overlay::NONE);
  feedback.show("media_player.room",false,4000);feedback.observe("media_player.other",true);
  assert(feedback.view("media_player.room","playing",4001)==Overlay::NONE);
  feedback.show("media_player.room",false,0xffffff00u);
  assert(feedback.remaining_ms("media_player.room",0x100u)==488);
  feedback.observe("media_player.room",false);assert(feedback.view("media_player.room","playing",0x101u)==Overlay::NONE);

  const std::string origin="http://ha.local:8123";
  assert(artwork_url(origin,"/api/media_player_proxy/player?token=example") == origin+"/api/media_player_proxy/player?token=example");
  assert(artwork_url(origin,"https://art.example/album.png")=="https://art.example/album.png");
  for (const auto &missing : {"", "None", "unknown", "unavailable", "//other.example/cover", "file:///etc/passwd"})
    assert(artwork_url(origin,missing).empty());
  Progress p;
  p.set_state("playing", 1000); p.set_duration(200); p.set_position(20, 1000); p.set_updated_at(1000);
  auto view = p.view(2000, 1005.5, true);
  assert(view.visible && std::fabs(view.seconds - 25.5f) < .001f && view.permille == 128);
  p.set_state("paused", 2100); view = p.view(9000, 1012.5, true);
  assert(view.visible && std::fabs(view.seconds - 25.6f) < .001f);
  p.set_state("playing", 9000); view = p.view(10000, 1013.5, true);
  assert(std::fabs(view.seconds - 26.6f) < .001f);
  p.set_position(80, 10000); p.set_updated_at(1013.5); // Seek feedback replaces interpolation.
  view = p.view(10500, 1014, true); assert(std::fabs(view.seconds - 80.5f) < .001f);
  assert(!p.view(11000, 1015, false).visible);
  p.new_track(); assert(!p.view(11000, 1015, true).visible);
  p.set_position(0, 11000); p.set_updated_at(1015);
  view = p.view(11100, 1015.1, true); assert(view.visible && view.seconds < .11f);
  p.new_track(); p.set_updated_at(1016); // Same zero position need not be resent for a new track.
  assert(p.view(12000, 1016.1, true).visible);
  p.set_duration(0); assert(!p.view(12000, 1016.2, true).visible); // Live/unknown-duration streams.
  p.set_duration(NAN); assert(!p.view(12000, 1016.2, true).visible);
  p.set_duration(10); p.set_position(999, 13000);
  assert(p.view(13000, NAN, true).permille == 1000);
  p.set_position(0, UINT32_MAX - 500); p.updated_epoch = NAN;
  assert(std::fabs(p.view(499, NAN, true).seconds - 1) < .01f);
  p.set_state("idle", 500); assert(!p.view(500, NAN, true).visible);
}
