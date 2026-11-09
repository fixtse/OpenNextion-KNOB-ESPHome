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
  ha_origin=origin;
  requested_url=origin+"/api/media_player_proxy/player"; assert(artwork_needs_auth());
  requested_url="https://external.example/album.png"; assert(!artwork_needs_auth());
  requested_url=origin+".evil.example/cover"; assert(!artwork_needs_auth());
}
