"""Remote packages resolve local assets beside their declaring YAML document."""
import re
import unittest
from pathlib import Path


class PackageAssets(unittest.TestCase):
    def test_assets_exist_beside_the_declaring_package(self):
        for document in Path('packages').rglob('*.yaml'):
            for relative in re.findall(r'^\s*- file: "([^"]+)"', document.read_text(), re.MULTILINE):
                with self.subTest(document=str(document), asset=relative):
                    self.assertTrue((document.parent / relative).is_file())
