"""Exercise the production selected-metadata reducer, including stale removal."""
import pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parents[1]
source=(root/'onx_entity_config.h').read_text()
start=source.index('inline bool save_selected_metadata(');end=source.index('\n}  // namespace onx_entity_config',start)
code=r'''
#include <array>
#include <memory>
#include <string>
#include <cstring>
#include <cassert>
#include "onx_config_rules.h"
namespace esphome::onx_config_web { struct Query {std::string entity,action,name,area,action_name;}; }
namespace custom_logic {
constexpr size_t MAX_HA_ENTITY_COUNT=21;
constexpr unsigned HA_ENTITIES_MAGIC=42;
struct StoredHaEntity {char entity_id[129]{},friendly_name[65]{},area_name[65]{};};
struct StoredHaEntities {unsigned magic{};unsigned count{};StoredHaEntity entities[MAX_HA_ENTITY_COUNT]{};};
StoredHaEntities cache;
std::array<std::string,10> ids;
std::string cached_standby_weather_entity_id;
void load_ha_entities_if_needed(){}
void load_device_config_if_needed(size_t){}
void load_standby_weather_config_if_needed(){}
const auto &cached_ha_entities(){return cache;}
const auto &cached_device_entity_id(size_t i){return ids[i];}
template<class T> auto make_ram_allocated(){return std::make_unique<T>();}
template<size_t N> void copy_string_to_fixed_buffer(const std::string &s,char (&out)[N]){std::strncpy(out,s.c_str(),N-1);out[N-1]=0;}
bool save_ha_entities(const StoredHaEntities &next){cache=next;return true;}
}
std::array<std::string,10> actions;
std::string action_for_slot(size_t i){return actions[i];}
REDUCER
int main(){
 using namespace custom_logic;
 for(size_t i=0;i<10;++i){ids[i]="sensor.selected_"+std::to_string(i);actions[i]="script.action_"+std::to_string(i);}
 cached_standby_weather_entity_id="weather.home";
 cache.count=1;copy_string_to_fixed_buffer("sensor.unselected",cache.entities[0].entity_id);
 esphome::onx_config_web::Query query{ids[0],actions[0],"Living room","Lounge","Comfort"};
 assert(save_selected_metadata(query));assert(cache.count==21);
 bool found=false;
 for(size_t i=0;i<cache.count;++i){
  assert(std::string(cache.entities[i].entity_id)!="sensor.unselected");
  if(query.entity==cache.entities[i].entity_id){assert(std::string(cache.entities[i].friendly_name)=="Living room");found=true;}
 }
 assert(found);
 ids[0]="climate.calentador";actions[0]="none";query={ids[0],"none","Calentador","Kitchen",""};
 assert(save_selected_metadata(query));assert(cache.count==20);
 for(size_t i=0;i<cache.count;++i){
  assert(std::string(cache.entities[i].entity_id)!="sensor.selected_0");
  assert(std::string(cache.entities[i].entity_id)!="script.action_0");
  assert(std::string(cache.entities[i].entity_id)!="none");
 }
}
'''.replace('REDUCER',source[start:end])
with tempfile.TemporaryDirectory() as directory:
 p=pathlib.Path(directory);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++17','-I'+str(root),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('Only selected IDs and metadata are retained; deselected items are removed')
