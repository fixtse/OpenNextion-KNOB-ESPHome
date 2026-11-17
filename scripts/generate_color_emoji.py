"""Generate the emoji sequence table from Noto Color Emoji (OFL).

Only the sequences ship in flash; the knob downloads each emoji's Noto SVG on first use.
Usage: python3 scripts/generate_color_emoji.py NotoColorEmoji.ttf
"""
import sys
from pathlib import Path
from fontTools.ttLib import TTFont

font_path = sys.argv[1]
source = TTFont(font_path)
reverse = {name: chr(code) for code, name in source.getBestCmap().items()}
sequences = set()
for lookup in source['GSUB'].table.LookupList.Lookup:
    for table in lookup.SubTable:
        for first, ligatures in getattr(table, 'ligatures', {}).items():
            for ligature in ligatures:
                names = [first, *ligature.Component]
                if all(name in reverse for name in names):
                    sequences.add(''.join(reverse[name] for name in names))
# Only emoji scalars, excluding controls, joiners, selectors and tag characters.
sequences.update(chr(c) for c in source.getBestCmap() if
                 0x1F000 <= c <= 0x1FAFF or 0x2300 <= c <= 0x2BFF or c in (0xA9, 0xAE, 0x203C, 0x2049, 0x2122, 0x2139, 0x3030, 0x303D, 0x3297, 0x3299))
sequences = sorted({sequence.replace('\ufe0f', '') for sequence in sequences})

entries = sorted(sequences, key=lambda sequence: sequence.encode())
literal = lambda sequence: '"' + ''.join(f'\\x{v:02x}' for v in sequence.encode()) + '"'
lines = ['// Emoji sequences from Noto Color Emoji (SIL OFL 1.1), sorted by UTF-8 bytes.',
         '// Pixels are downloaded on demand (onx_emoji.h); only the lookup table is in flash.',
         '#pragma once', 'namespace onx_emoji {',
         'inline const char *const SEQUENCES[] = {' + ',\n'.join(map(literal, entries)) + '};', '}']
Path('onx_emoji_table.h').write_text('\n'.join(lines) + '\n')
print(f'{len(entries)} sequences')
