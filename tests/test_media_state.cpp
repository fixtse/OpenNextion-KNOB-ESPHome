#include "../onx_media_state.h"
#include <cassert>
int main() {
  using namespace onx_media;
  ClickDetector click;
  assert(!click.release(1000, "media_player.room"));
  assert(!click.due(1350)); assert(click.due(1351));
  assert(click.release(1300, "media_player.room"));
  assert(!click.pending); assert(!click.due(2000));
  assert(!click.release(3000, "media_player.room"));
  assert(!click.release(3200, "media_player.other"));
  click.cancel(); assert(!click.due(4000));
  assert(!click.release(0xffffff00u, "media_player.room"));
  assert(click.due(0x100u)); // millisecond counter wraparound

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
