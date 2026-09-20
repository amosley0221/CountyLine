"""Regression tests for Scripts/draw_town_layout.py.

The generator reads the native town source and draws footprints, so a shop that
is rotated, renamed, or written differently can vanish from the map or be drawn
facing a wall. These tests run the real generator inside a temporary copy of the
tree, so the repository's committed map is never rewritten.

Run: <engine>/Engine/Binaries/ThirdParty/Python3/Win64/python.exe Scripts/Tests/test_town_layout_map.py
Standard library only, like the generator itself.
"""
import math
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GENERATOR = ROOT / 'Scripts/draw_town_layout.py'
TOWN_SOURCE = ROOT / 'Source/CountyLine/World/CLPecosBend.cpp'
SCALE = .062  # Drawing units per centimetre, from the generator.

STORE_PATTERN = re.compile(
    r'Store\(TEXT\("([^"]+)"\),TEXT\("([^"]+)"\),FVector\(([^)]+)\),([\d.]+),([\d.]+),(-?[\d.]+)\)')
RECT_PATTERN = re.compile(
    r'<rect x="(-?[\d.]+)" y="(-?[\d.]+)" width="([\d.]+)" height="([\d.]+)"')


def run_generator(source_text):
    """Draw a map from the given town source inside a throwaway tree."""
    sandbox = Path(tempfile.mkdtemp(prefix='cl_town_map_'))
    try:
        (sandbox / 'Scripts').mkdir()
        (sandbox / 'Source/CountyLine/World').mkdir(parents=True)
        shutil.copy2(GENERATOR, sandbox / 'Scripts/draw_town_layout.py')
        (sandbox / 'Source/CountyLine/World/CLPecosBend.cpp').write_text(source_text, encoding='utf-8')
        result = subprocess.run([sys.executable, str(sandbox / 'Scripts/draw_town_layout.py')],
                                capture_output=True, text=True)
        svg_path = sandbox / 'Docs/Maps/PecosBend-layout.svg'
        svg = svg_path.read_text(encoding='utf-8') if svg_path.exists() else ''
        return result, svg
    finally:
        shutil.rmtree(sandbox, ignore_errors=True)


def expected_facing(yaw):
    """Local front is -Y: +90 faces east, -90 west, 0 south."""
    angle = math.radians(yaw)
    return round(math.sin(angle)), round(-math.cos(angle))


def to_drawing(x, y):
    return 50 + (x + 8000) * SCALE, 110 + (7800 - y) * SCALE


class TownMapTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = TOWN_SOURCE.read_text(encoding='utf-8')
        cls.result, cls.svg = run_generator(cls.source)
        cls.shops = STORE_PATTERN.findall(cls.source)

    def test_generator_runs_on_the_current_town(self):
        self.assertEqual(self.result.returncode, 0, self.result.stderr)
        self.assertTrue(self.svg.startswith('<svg'), 'no map was drawn')

    def test_every_shop_in_the_source_reaches_the_map(self):
        """A shop the regex misses is silently absent from the map."""
        self.assertEqual(len(self.shops), self.source.count('Store(TEXT('),
                         'a Store call is written in a form the generator cannot read')
        self.assertGreaterEqual(len(self.shops), 5)
        for _name, title, *_ in self.shops:
            self.assertIn(title.title(), self.svg, f'{title} is missing from the map legend')

    def test_rotated_shops_are_drawn_turned(self):
        """A quarter-turned shop is 700 deep along X and its width along Y."""
        for name, title, position, width, _height, yaw in self.shops:
            x, y, _z = (float(v) for v in position.split(','))
            facing = expected_facing(float(yaw))
            with self.subTest(shop=name):
                self.assertEqual(abs(facing[0]) + abs(facing[1]), 1, 'shop is not cardinal')
                w, h = (700.0, float(width)) if facing[0] else (float(width), 700.0)
                left, top = to_drawing(x - w / 2, y + h / 2)
                rect = f'<rect x="{left:.1f}" y="{top:.1f}" width="{w * SCALE:.1f}" height="{h * SCALE:.1f}"'
                self.assertIn(rect, self.svg,
                              f'{title} is not drawn with its turned footprint')

    def test_front_arrows_point_at_the_street(self):
        """The arrow starts on the shop's front edge and points outward."""
        for name, title, position, width, _height, yaw in self.shops:
            x, y, _z = (float(v) for v in position.split(','))
            fx, fy = expected_facing(float(yaw))
            w, h = (700.0, float(width)) if fx else (float(width), 700.0)
            start = to_drawing(x + fx * w / 2, y + fy * h / 2)
            with self.subTest(shop=name):
                self.assertIn(f'<path d="M {start[0]} {start[1]} L ', self.svg,
                              f'{title} has no front arrow on its street edge')

    def test_west_and_east_ranks_face_each_other(self):
        """The two commercial ranks look across Court Street, not away."""
        facings = {name: expected_facing(float(yaw)) for name, _t, _p, _w, _h, yaw in self.shops}
        for name in ('DryGoods', 'Grocer', 'ClosedShop'):
            self.assertEqual(facings[name], (1, 0), f'{name} should face east onto Market Street')
        for name in ('PostOffice', 'Drugs'):
            self.assertEqual(facings[name], (-1, 0), f'{name} should face west onto its walk')

    def test_every_building_gets_its_own_front_arrow(self):
        """Fronts are keyed by label, so two shops sharing a title lose one."""
        numbered = len(re.findall(r'<circle cx="[\d.]+" cy="[\d.]+" r="14"', self.svg))
        arrows = len(re.findall(r'<path d="M [\d.]+ [\d.]+ L ', self.svg))
        benches = 4
        self.assertEqual(arrows - benches, numbered,
                         'a building is missing its front arrow; check for duplicate titles')

    def test_a_non_cardinal_shop_is_refused(self):
        """The map has no polygon for an angled shop, so it must fail loudly."""
        angled = self.source.replace(
            'Store(TEXT("DryGoods"),TEXT("DRY GOODS"),FVector(-6600,2300,0),950,520,90)',
            'Store(TEXT("DryGoods"),TEXT("DRY GOODS"),FVector(-6600,2300,0),950,520,35)')
        self.assertNotEqual(angled, self.source, 'the DryGoods call moved; update this test')
        result, _svg = run_generator(angled)
        self.assertNotEqual(result.returncode, 0, 'an angled shop was drawn as a straight box')
        self.assertIn('polygon for non-cardinal shops', result.stderr)

    def test_a_renamed_ground_shape_is_refused(self):
        """Street and ground boxes are looked up by name; a rename must fail."""
        renamed = self.source.replace('Shape(TEXT("WestServiceLane")', 'Shape(TEXT("WestBackLane")')
        self.assertNotEqual(renamed, self.source, 'WestServiceLane moved; update this test')
        result, _svg = run_generator(renamed)
        self.assertNotEqual(result.returncode, 0, 'a missing street was drawn anyway')
        self.assertIn('Source geometry changed', result.stderr)

    def test_the_committed_map_matches_the_current_source(self):
        """The drawing in Docs is regenerated when the town changes."""
        committed = (ROOT / 'Docs/Maps/PecosBend-layout.svg').read_text(encoding='utf-8')
        self.assertEqual(committed.strip(), self.svg.strip(),
                         'Docs/Maps/PecosBend-layout.svg is stale; run Scripts/draw_town_layout.py')


if __name__ == '__main__':
    # Report on stdout: PowerShell 5.1 turns a native command's stderr into an
    # error record, which would fail the runner even on a passing suite.
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(TownMapTests)
    outcome = unittest.TextTestRunner(stream=sys.stdout, verbosity=2).run(suite)
    sys.exit(0 if outcome.wasSuccessful() else 1)
