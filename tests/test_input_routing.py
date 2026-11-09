"""Run the production release handler against entity and settings page contexts."""
from pathlib import Path
import re
import subprocess
import tempfile
import textwrap
import unittest


class InputRouting(unittest.TestCase):
    def test_settings_presses_are_not_consumed_by_entity_controls(self):
        root = Path(__file__).resolve().parents[1]
        source = (root / 'packages/base/input.yaml').read_text()
        start = source.index('        - lambda: |-\n', source.index('    on_release:')) + len('        - lambda: |-\n')
        body = textwrap.dedent(source[start:source.index('\n        - if:', start)])
        header = (root / 'custom_logic.h').read_text()
        start = header.index('enum NormalDeviceDetailKind')
        kinds = header[start:header.index('};', start) + 2]
        code = r'''
#include "onx_media_state.h"
#include <cassert>
#include <string>
#define id(name) name
#define ESP_LOGI(...) ((void)0)
KINDS
uint32_t clock_ms=2000;
uint32_t millis(){return clock_ms;}
bool sleep_mode_state=false,sleep_wakeup_pending=false,standby_wakeup_pending=false;
bool screen_off_state=false,key_long_press_handled=false,detail_long_press_exit_fired=false;
bool normal_clear_wifi_allowed=true,ha_light_active=false,ha_light_selected_slider=false;
bool normal_device_detail_active=true,normal_device_detail_online=true,normal_ha_api_connected=true;
bool normal_device_detail_curtain_adjust_active=false,normal_detail_light_mode_adjusting=false,normal_detail_light_mode_rotated=false;
int normal_device_detail_kind=5,normal_device_detail_index=0,normal_page_index=1,normal_detail_light_mode_count=0;
uint32_t key_press_started_ms=1900,key_last_release_ms=0;
std::string configured_entity="media_player.room",bound_action;
struct UI {
 bool is_normal_mode_active(){return true;} bool is_standby_active(){return false;}
 std::string get_normal_device_entity_id(int){return configured_entity;}
 void notify_user_activity(){}
} ui;auto *ui_presenter=&ui;
struct Mode {bool is_provisioning_mode_enabled(){return false;}} mode;auto *mode_manager=&mode;
struct Script {
 unsigned calls{0};
 template<class... T> void execute(T...){++calls;}
 void turn_off(){}
} exit_standby_mode,ha_screen_off,retry_normal_entity_state,refresh_normal_automation_detail_page,
  emit_normal_media_track,emit_configured_press_action,refresh_normal_light_detail_page;
namespace custom_logic_entity_sub {
struct Detail {bool received{true},available{true};} detail;
std::string control_preview;
Detail *entity_detail(const std::string &){return &detail;}
}
namespace onx_controls {
unsigned presses=0;
bool handles(const std::string &s){return s.rfind("media_player.",0)==0||s.rfind("climate.",0)==0;}
void press(const std::string &){++presses;}
}
namespace onx_entity_config {std::string action_for_ui(int){return bound_action;}}
void release(){
BODY
}
void reset(){
 key_long_press_handled=false;detail_long_press_exit_fired=false;
 retry_normal_entity_state.calls=emit_normal_media_track.calls=emit_configured_press_action.calls=onx_controls::presses=0;
 onx_media::clicks.cancel();bound_action.clear();normal_device_detail_online=true;
 custom_logic_entity_sub::detail.received=custom_logic_entity_sub::detail.available=true;
}
int main(){
 for(int kind : {NORMAL_DEVICE_DETAIL_KIND_SETTINGS,NORMAL_DEVICE_DETAIL_KIND_SETTINGS_WIFI,
                 NORMAL_DEVICE_DETAIL_KIND_SETTINGS_BRIGHTNESS,NORMAL_DEVICE_DETAIL_KIND_SETTINGS_KNOB,
                 NORMAL_DEVICE_DETAIL_KIND_SETTINGS_CONNECTION}){
  normal_device_detail_kind=kind;
  for(bool available : {true,false}){
   reset();normal_device_detail_online=available;custom_logic_entity_sub::detail.available=available;
   configured_entity="media_player.room";bound_action="script.test";
   release();
   assert(!key_long_press_handled&&!onx_media::clicks.pending);
   assert(!retry_normal_entity_state.calls&&!onx_controls::presses&&!emit_configured_press_action.calls);
   reset();configured_entity="climate.room";release();assert(!key_long_press_handled&&!onx_controls::presses);
  }
 }
 reset();normal_device_detail_kind=NORMAL_DEVICE_DETAIL_KIND_AUTOMATION;
 configured_entity="climate.room";release();assert(onx_controls::presses==1&&key_long_press_handled);
 reset();configured_entity="media_player.room";release();assert(onx_media::clicks.pending&&key_long_press_handled);
 reset();normal_device_detail_online=false;release();assert(retry_normal_entity_state.calls==1&&key_long_press_handled);
 reset();configured_entity="sensor.room";bound_action="script.test";release();assert(emit_configured_press_action.calls==1);
}
'''.replace('KINDS', kinds).replace('BODY', body)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / 'test.cpp').write_text(code)
            subprocess.run(['c++', '-std=c++17', '-I' + str(root), str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
            subprocess.run([str(path / 'test')], check=True)


if __name__ == '__main__':
    unittest.main()
