"""Compile and exercise the production discovery parser with real ArduinoJson.

Usage: python3 tests/test_entity_discovery.py /path/to/ArduinoJson/src
The small host harness substitutes only storage/logging and ESPHome allocation.
"""
import pathlib
import re
import subprocess
import sys
import tempfile

source = (pathlib.Path(__file__).resolve().parents[1] / 'custom_logic.h').read_text()
start = source.index('inline bool store_ha_entities_from_entity_registry_array(JsonArrayConst array, bool merge) {')
end = source.index('\ninline const char *get_ha_entity_display_name', start)
predicate_start = source.index('inline bool is_supported_normal_device_entity_id(const char *entity_id) {')
predicate_end = source.index('\ninline bool is_local_hmi_gpio_switch_entity_id', predicate_start)
predicate = source[predicate_start:predicate_end]
capacity = re.search(r'MAX_HA_ENTITY_COUNT = (\d+)', source)[1]
structs = source[source.index('struct StoredHaEntity {'):source.index('struct StoredHaEntityChunk {')]
harness = r'''
#include <ArduinoJson.h>
#include <algorithm>
#include <memory>
#include <cstring>
#include <string>
#include <cassert>
#include <iostream>
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
constexpr size_t MAX_HA_ENTITY_COUNT = CAPACITY;
constexpr size_t MAX_HA_ENTITY_ID_LENGTH = 128;
constexpr size_t MAX_HA_FRIENDLY_NAME_LENGTH = 64;
constexpr size_t MAX_HA_AREA_NAME_LENGTH = 64;
constexpr unsigned HA_ENTITIES_MAGIC = 42;
STRUCTS
static StoredHaEntities cache{};
static bool ha_entities_changed_since_last_parse_flag;
template<class T> auto make_ram_allocated() { return std::make_unique<T>(); }
void log_ha_registry_memory(const char *) {}
void load_ha_entities_if_needed() {}
auto &cached_ha_entities() { return cache; }
bool has_same_ha_entities(const StoredHaEntities &a, const StoredHaEntities &b) {
  return a.count == b.count && !memcmp(a.entities, b.entities, sizeof(a.entities));
}
bool save_ha_entities(const StoredHaEntities &next) { cache = next; return true; }
std::string ha_json_variant_to_string(JsonVariantConst v) { return v.is<const char *>() ? v.as<const char *>() : ""; }
bool is_local_hmi_gpio_switch_preset_option(const char *) { return false; }
PREDICATE
bool is_supported_ha_entity_id(const char *id) { return is_supported_normal_device_entity_id(id); }
const char *get_ha_area_name_by_id(const char *id) { return id && !strcmp(id,"kitchen") ? "Kitchen" : ""; }
template<size_t N> void copy_string_to_fixed_buffer(const std::string &s, char (&dest)[N]) {
  std::strncpy(dest,s.c_str(),N-1); dest[N-1]=0;
}
PARSER
int main() {
  JsonDocument registry;
  auto entries = registry.to<JsonArray>();
  for (unsigned i=0;i<250;++i) {
    auto item=entries.add<JsonObject>();
    item["ei"]="sensor.test_"+std::to_string(i); item["en"]="Registry name"; item["ai"]="kitchen";
  }
  assert(store_ha_entities_from_entity_registry_array(entries,false));
  assert(cache.count==250); // the old 200-entry ceiling must not return
  JsonDocument states;
  assert(!deserializeJson(states,R"([
    {"entity_id":"climate.calentador","attributes":{"friendly_name":"Calentador"}},
    {"entity_id":"sensor.test_1","attributes":{"friendly_name":"Live name"}},
    {"entity_id":"climate.calentador","attributes":{"friendly_name":"Calentador"}},
    {"entity_id":"binary_sensor.door","attributes":{}},
    {"entity_id":"media_player.living_room","attributes":{"friendly_name":"Living room"}},
    {"entity_id":"unsupported.example","attributes":{}}
  ])"));
  assert(store_ha_entities_from_entity_registry_array(states.as<JsonArrayConst>(),true));
  assert(cache.count==253);
  assert(!strcmp(cache.entities[252].entity_id,"media_player.living_room"));
  assert(!strcmp(cache.entities[250].entity_id,"climate.calentador"));
  assert(!strcmp(cache.entities[250].friendly_name,"Calentador"));
  assert(!strcmp(cache.entities[1].friendly_name,"Live name"));
  assert(!strcmp(cache.entities[1].area_name,"Kitchen"));
  // A new registry snapshot replaces stale entries, rather than accumulating them.
  JsonDocument empty;
  assert(store_ha_entities_from_entity_registry_array(empty.to<JsonArray>(),false));
  assert(cache.count==0);
  // Bound writes even if HA returns more entities than the flash-backed capacity.
  JsonDocument many;
  auto list=many.to<JsonArray>();
  for (size_t i=0;i<MAX_HA_ENTITY_COUNT+5;++i) list.add<JsonObject>()["ei"]="sensor.bound_"+std::to_string(i);
  assert(store_ha_entities_from_entity_registry_array(list,false));
  assert(cache.count==MAX_HA_ENTITY_COUNT);
  std::cout << "Production entity discovery regression passed\n";
}
'''.replace('CAPACITY', capacity).replace('STRUCTS', structs).replace('PARSER', source[start:end]).replace('PREDICATE', predicate)
with tempfile.TemporaryDirectory() as directory:
    target = pathlib.Path(directory)
    (target / 'test.cpp').write_text(harness)
    subprocess.run(['c++', '-std=c++17', '-I'+sys.argv[1], str(target/'test.cpp'), '-o', str(target/'test')], check=True)
    subprocess.run([str(target/'test')], check=True)
