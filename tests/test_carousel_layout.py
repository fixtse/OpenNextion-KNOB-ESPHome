"""Prevent carousel content from escaping its animated page into the top layer."""
from pathlib import Path
import re
import unittest

class CarouselParenting(unittest.TestCase):
    def test_values_and_pagination_belong_to_each_page(self):
        root=Path(__file__).resolve().parents[1]
        source=(root/'packages/features/normal_mode/normal_mode_pages.yaml').read_text()
        top,pages=source.split('  pages:',1)
        self.assertNotRegex(top,r'normal_device_.*(?:value|context|pagination|index_text)')
        blocks=re.split(r'\n    - id: ',pages)
        for index in range(10):
            page=next(block for block in blocks if block.startswith(f'device_{index}\n'))
            for role in ['value','context','pagination']:
                self.assertIn(f'                  id: normal_device_{index}_{role}\n',page)
        settings=next(block for block in blocks if block.startswith('device_settings\n'))
        self.assertIn('id: normal_device_settings_pagination',settings)

if __name__=='__main__': unittest.main()
