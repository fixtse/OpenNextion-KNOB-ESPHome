"""Exercise production recovery scheduling against a captured Native API transport."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class Recovery(unittest.TestCase):
    def test_recovery_reads_are_bounded_and_never_append_subscriptions(self):
        root = Path(__file__).resolve().parents[1]
        source = (root / 'custom_logic_entity_sub.h').read_text()
        source = source[source.index('// Recovery reads use existing callbacks'):source.index('inline void subscribe_standby_weather_field')]
        harness = r'''
#include "onx_recovery_state.h"
#include "onx_sensor_view.h"
#include <array>
#include <vector>
#include <memory>
#include <algorithm>
#include <cassert>
#include <string>
#define USE_API_HOMEASSISTANT_STATES
uint32_t clock_ms=0;
uint32_t millis(){return clock_ms;}
namespace esphome {
struct StringRef {
  std::string value;
  explicit StringRef(const char *s):value(s){}
  StringRef()=default;
};
namespace api {
struct SubscribeHomeAssistantStateResponse {StringRef entity_id,attribute;bool once{false};};
struct Client {
  std::vector<SubscribeHomeAssistantStateResponse> sent;
  bool accept{true};
  bool is_connection_setup(){return true;}
  bool send_message(const SubscribeHomeAssistantStateResponse &message){if(accept)sent.push_back(message);return accept;}
};
struct Sub {const char *entity_id,*attribute;};
struct APIServer {
  std::vector<Sub> subs;
  std::vector<std::shared_ptr<Client>> clients{std::make_shared<Client>()};
  const auto &get_state_subs(){return subs;}
  const auto &active_clients(){return clients;}
};
}}
namespace custom_logic {
std::array<std::string,10> ids;
std::string cached_standby_weather_entity_id;
std::string cached_device_entity_id(size_t i){return ids[i];}
}
struct Detail {bool received{false},available{false};} detail;
Detail *entity_detail(const std::string &entity){return entity==custom_logic::ids[0]?&detail:nullptr;}
bool is_valid_entity_id(const std::string &entity){return entity.find('.')!=std::string::npos;}
unsigned cancelled=0;
void cancel_controls(){++cancelled;}
SOURCE
int main(){
  esphome::api::APIServer api;
  custom_logic::ids[0]="sensor.later";
  api.subs={{"sensor.later",""},{"sensor.later","icon"}};
  recover_entity_states(&api,true);assert(api.clients[0]->sent.empty());
  clock_ms=10000;recover_entity_states(&api,true);assert(api.clients[0]->sent.size()==1);
  assert(api.clients[0]->sent[0].once);assert(api.subs.size()==2);
  api.clients[0]->accept=false;
  clock_ms=10100;recover_entity_states(&api,true);assert(recovery_cursor==1);
  api.clients[0]->accept=true;
  clock_ms=10200;recover_entity_states(&api,true);assert(api.clients[0]->sent.size()==2);
  assert(api.clients[0]->sent[1].attribute.value=="icon");
  clock_ms=10300;recover_entity_states(&api,true);assert(recovering_entity.empty());
  detail.received=detail.available=true;
  clock_ms=11000;recover_entity_states(&api,true);assert(api.clients[0]->sent.size()==2);
  assert(queue_entity_refresh("sensor.later",true));assert(cancelled==1);
  assert(!queue_entity_refresh("sensor.later",true));
  recover_entity_states(&api,true);assert(api.clients[0]->sent.size()==3);
  recover_entity_states(&api,false);assert(recovery_queue.empty()&&recovering_entity.empty());
  clock_ms=12000;recover_entity_states(&api,true);assert(api.clients[0]->sent.size()==3);
  // Changing an entity while its read is queued must not apply old work to the new slot.
  clock_ms=15000;assert(queue_entity_refresh("sensor.later",true));
  custom_logic::ids[0]="sensor.other";
  recover_entity_states(&api,true);assert(api.clients[0]->sent.size()==3);
  assert(api.subs.size()==2);
}
'''.replace('SOURCE', source)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / 'test.cpp').write_text(harness)
            subprocess.run(['c++', '-std=c++17', '-I' + str(root), str(path / 'test.cpp'),
                            '-o', str(path / 'test')], check=True)
            subprocess.run([str(path / 'test')], check=True)


if __name__ == '__main__':
    unittest.main()
