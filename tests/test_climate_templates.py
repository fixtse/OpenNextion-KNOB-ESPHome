"""Exercise the actual HA action templates with thermostat feedback and limits.

Run in the ESPHome container: python -m unittest discover -s tests
"""
import unittest
from pathlib import Path

import yaml
from jinja2 import Environment


class Loader(yaml.SafeLoader):
    pass


Loader.add_constructor('!lambda', lambda loader, node: loader.construct_scalar(node))


class ClimateTemplates(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = Path('packages/features/normal_mode/normal_mode_scripts.yaml')
        scripts = yaml.load(source.read_text(), Loader=Loader)['script']
        script = next(item for item in scripts if item['id'] == 'adjust_normal_climate_temperature')
        branch = script['then'][0]['if']['then'][0]['if']
        cls.single = branch['then'][0]['homeassistant.service']['data_template']
        cls.dual = branch['else'][0]['homeassistant.service']['data_template']

    def render(self, templates, attrs, direction):
        env = Environment()
        env.globals['state_attr'] = lambda entity, key: attrs.get(key)
        return {key: float(env.from_string(value).render(entity='climate.test', direction=direction))
                for key, value in templates.items() if key != 'entity_id'}

    def test_single_temperature_respects_both_limits(self):
        attrs = dict(temperature=29.5, min_temp=16, max_temp=30, target_temp_step=1)
        self.assertEqual(self.render(self.single, attrs, 1)['temperature'], 30)
        attrs['temperature'] = 16
        self.assertEqual(self.render(self.single, attrs, -1)['temperature'], 16)

    def test_fractional_and_fahrenheit_steps(self):
        attrs = dict(temperature=20, min_temp=16, max_temp=30, target_temp_step=0.5)
        self.assertEqual(self.render(self.single, attrs, -1)['temperature'], 19.5)
        attrs.update(temperature=72, min_temp=60, max_temp=85, target_temp_step=1)
        self.assertEqual(self.render(self.single, attrs, 1)['temperature'], 73)

    def test_missing_or_invalid_step_uses_default(self):
        attrs = dict(temperature=20, min_temp=16, max_temp=30)
        for step in (None, 0, -1):
            attrs['target_temp_step'] = step
            self.assertEqual(self.render(self.single, attrs, 1)['temperature'], 20.5)

    def test_range_shifts_together(self):
        attrs = dict(target_temp_low=19, target_temp_high=23, min_temp=16, max_temp=30,
                     target_temp_step=0.5)
        self.assertEqual(self.render(self.dual, attrs, 1),
                         dict(target_temp_low=19.5, target_temp_high=23.5))

    def test_range_preserves_width_at_upper_limit(self):
        attrs = dict(target_temp_low=25.8, target_temp_high=29.8, min_temp=16, max_temp=30,
                     target_temp_step=0.5)
        result = self.render(self.dual, attrs, 1)
        self.assertEqual(result['target_temp_high'], 30)
        self.assertAlmostEqual(result['target_temp_high'] - result['target_temp_low'], 4)

    def test_range_preserves_width_at_lower_limit(self):
        attrs = dict(target_temp_low=16, target_temp_high=20, min_temp=16, max_temp=30,
                     target_temp_step=0.5)
        self.assertEqual(self.render(self.dual, attrs, -1),
                         dict(target_temp_low=16, target_temp_high=20))


if __name__ == '__main__':
    unittest.main()
