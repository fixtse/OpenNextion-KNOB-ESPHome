"""Exercise the production UI standby predicate with a page-dependent guard."""
from pathlib import Path
import subprocess
import tempfile
import unittest

class StandbyScope(unittest.TestCase):
    def test_production_guard(self):
        root=Path(__file__).resolve().parents[1]
        source=(root/'my_components/onx_ui/onx_ui.cpp').read_text()
        start=source.index('bool OnxUi::is_standby_active() const {')
        body=source[start:source.index('\nvoid OnxUi::notify_user_activity()',start)]
        code=r'''
#include "onx_display_settings.h"
#include <cassert>
#include <functional>
uint32_t clock_ms=10000;
uint32_t millis(){return clock_ms;}
struct OnxUi {
 bool normal=true;
 std::function<bool()> standby_guard_;
 uint32_t last_user_activity_ms_=0,standby_timeout_ms_=1000;
 bool is_normal_mode_active()const{return normal;}
 bool is_standby_active()const;
};
BODY
int main(){
 OnxUi ui;bool detail=false;uint8_t scope=0;
 ui.standby_guard_=[&]{return onx_display_settings::standby_allowed(scope,detail);};
 assert(ui.is_standby_active());detail=true;assert(!ui.is_standby_active());
 scope=1;assert(ui.is_standby_active());scope=2;detail=false;assert(!ui.is_standby_active());
 scope=0;ui.last_user_activity_ms_=clock_ms;assert(!ui.is_standby_active());
 clock_ms+=1000;assert(ui.is_standby_active());ui.normal=false;assert(!ui.is_standby_active());
}
'''.replace('BODY',body)
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)
            (path/'test.cpp').write_text(code)
            subprocess.run(['c++','-std=c++17','-I'+str(root),str(path/'test.cpp'),'-o',str(path/'test')],check=True)
            subprocess.run([str(path/'test')],check=True)

if __name__=='__main__': unittest.main()
