"""Exercise the actual service templates with ESPHome's string variables."""
import pathlib
import re
import unittest
from jinja2 import StrictUndefined
from jinja2.nativetypes import NativeEnvironment

source = (pathlib.Path(__file__).resolve().parents[1] / 'packages/features/normal_mode/normal_mode_scripts.yaml').read_text()
script = source[source.index('  - id: adjust_normal_climate_temperature'):]

def template(key):
    match = re.search(r'^( +)' + key + r': >-\n((?:\1  .*\n)+)', script, re.M)
    assert match, key
    return NativeEnvironment(undefined=StrictUndefined).from_string(match[2])

def render(key, direction, **attrs):
    return float(template(key).render(entity='climate.test', direction=str(direction), state_attr=lambda entity, key: attrs.get(key)))

class Controls(unittest.TestCase):
    def test_single_target_and_bounds(self):
        attrs = dict(temperature=22, min_temp=16, max_temp=30, target_temp_step=.5)
        self.assertEqual(render('temperature',1,**attrs),22.5)
        self.assertEqual(render('temperature',-1,**attrs),21.5)
        attrs['temperature']=30
        self.assertEqual(render('temperature',1,**attrs),30)
        attrs['temperature']=16
        self.assertEqual(render('temperature',-1,**attrs),16)
        attrs['target_temp_step']=0
        self.assertEqual(render('temperature',1,**attrs),16.5)
        attrs['target_temp_step']=1
        self.assertEqual(render('temperature',1,**attrs),17)

    def test_range_preserves_width_at_bounds(self):
        attrs = dict(target_temp_low=20,target_temp_high=24,min_temp=16,max_temp=30,target_temp_step=.5)
        for direction in (1,-1):
            self.assertEqual(render('target_temp_low',direction,**attrs),20+direction*.5)
            self.assertEqual(render('target_temp_high',direction,**attrs),24+direction*.5)
        attrs.update(target_temp_low=26,target_temp_high=30)
        self.assertEqual(render('target_temp_low',1,**attrs),26)
        self.assertEqual(render('target_temp_high',1,**attrs),30)
        attrs.update(target_temp_low=16,target_temp_high=20)
        self.assertEqual(render('target_temp_low',-1,**attrs),16)
        self.assertEqual(render('target_temp_high',-1,**attrs),20)

    def test_media_volume_and_bounds(self):
        self.assertAlmostEqual(render('volume_level',1,volume_level=.5),.52)
        self.assertAlmostEqual(render('volume_level',-1,volume_level=.5),.48)
        self.assertEqual(render('volume_level',1,volume_level=.99),1)
        self.assertEqual(render('volume_level',-1,volume_level=.01),0)

if __name__=='__main__': unittest.main()
