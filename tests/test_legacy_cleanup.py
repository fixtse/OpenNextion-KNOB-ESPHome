"""Check that upgrade cleanup deletes the old credential without reading it."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class CredentialCleanup(unittest.TestCase):
    def test_only_the_legacy_credential_is_erased(self):
        root = Path(__file__).resolve().parents[1]
        source = (root / 'custom_logic.h').read_text()
        start = source.index('inline void erase_legacy_ha_token()')
        cleanup = source[start:source.index('\n}', start) + 2]
        code = r'''
#include <cstdint>
#include <cstdio>
#include <string>
#include <cassert>
#define USE_ESP32
using nvs_handle_t = int;
constexpr int ESP_OK=0, ESP_ERR_NVS_NOT_FOUND=1, NVS_READWRITE=2;
constexpr uint32_t LEGACY_HA_TOKEN_PREF_KEY=42;
unsigned opened=0,erased=0,committed=0,closed=0;
int nvs_open(const char *space,int mode,nvs_handle_t *handle){assert(std::string(space)=="esphome"&&mode==NVS_READWRITE);*handle=9;++opened;return ESP_OK;}
int nvs_erase_key(nvs_handle_t handle,const char *key){assert(handle==9&&std::string(key)=="42");return ++erased==1?ESP_OK:ESP_ERR_NVS_NOT_FOUND;}
int nvs_commit(nvs_handle_t handle){assert(handle==9);++committed;return ESP_OK;}
void nvs_close(nvs_handle_t handle){assert(handle==9);++closed;}
CLEANUP
int main(){erase_legacy_ha_token();erase_legacy_ha_token();assert(opened==2&&erased==2&&committed==1&&closed==2);}
'''.replace('CLEANUP', cleanup)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / 'test.cpp').write_text(code)
            subprocess.run(['c++', '-std=c++17', str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
            subprocess.run([str(path / 'test')], check=True)


if __name__ == '__main__':
    unittest.main()
