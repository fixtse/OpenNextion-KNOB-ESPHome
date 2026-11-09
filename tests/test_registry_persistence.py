"""Run production chunk persistence against a backend that tracks queued RAM."""
import pathlib, subprocess, tempfile, re
source=(pathlib.Path(__file__).resolve().parents[1]/'custom_logic.h').read_text()
start=source.index('inline bool save_ha_entities(const StoredHaEntities &entities) {')
end=source.index('\ninline bool save_ha_areas',start)
constants='\n'.join(re.findall(r'^static constexpr size_t (?:MAX_HA_ENTITY_COUNT|HA_ENTITY_PREF_CHUNK_SIZE|MAX_HA_ENTITY_ID_LENGTH|MAX_HA_FRIENDLY_NAME_LENGTH|MAX_HA_AREA_NAME_LENGTH) = .*?;',source,re.M))
structs=source[source.index('struct StoredHaEntity {'):source.index('struct StoredHaArea {')]
code=r'''
#include <array>
#include <memory>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
CONSTANTS
constexpr size_t HA_ENTITY_PREF_CHUNK_COUNT=(MAX_HA_ENTITY_COUNT+HA_ENTITY_PREF_CHUNK_SIZE-1)/HA_ENTITY_PREF_CHUNK_SIZE;
constexpr unsigned HA_ENTITY_CHUNK_MAGIC=42;
STRUCTS
size_t pending=0, peak=0, commits=0;
bool fail_sync=false;
struct Preference {
  template<class T> bool save(const T *) { pending+=sizeof(T); peak=std::max(peak,pending); return true; }
};
struct Preferences { bool sync() { ++commits; pending=0; return !fail_sync; } } prefs;
namespace esphome {
  auto *global_preferences=&prefs;
  struct Application { void feed_wdt() {} } App;
}
std::array<Preference,HA_ENTITY_PREF_CHUNK_COUNT> ha_entity_chunk_prefs;
bool ha_entity_chunk_prefs_ready=true,ha_entities_loaded=false;
void ensure_ha_entity_chunk_prefs_ready() {}
void log_ha_registry_memory(const char *) {}
template<class T> auto make_ram_allocated() { return std::make_unique<T>(); }
StoredHaEntities cache{};
auto &cached_ha_entities() { return cache; }
SAVE
int main() {
  auto entities=std::make_unique<StoredHaEntities>();
  entities->count=MAX_HA_ENTITY_COUNT;
  assert(save_ha_entities(*entities));
  assert(peak<=sizeof(StoredHaEntityChunk));
  assert(commits>=HA_ENTITY_PREF_CHUNK_COUNT);
  assert(cache.count==MAX_HA_ENTITY_COUNT);
  fail_sync=true;
  assert(!save_ha_entities(*entities));
}
'''.replace('CONSTANTS',constants).replace('STRUCTS',structs).replace('SAVE',source[start:end])
with tempfile.TemporaryDirectory() as directory:
    path=pathlib.Path(directory);(path/'test.cpp').write_text(code)
    subprocess.run(['c++','-std=c++17',str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('Production registry persistence bounds pending copies to one chunk')
