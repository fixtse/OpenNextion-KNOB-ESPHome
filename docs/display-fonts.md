# Display character support

Text fonts (12–32 px) explicitly include `GF_Latin_Kernel` and `GF_Latin_Core`.
This retains ordinary English text while adding accented Latin letters used in
European and American languages, including `Beyoncé`, `Mañana`, `München`, and
`São Paulo`. Large clock fonts retain their smaller original character set.

All text sizes also explicitly include the right arrow (`→`, U+2192) and bullet
(`●`) from MiSans. Large clock fonts keep their original character set.

Text fonts also include a bounded set of monochrome emoji from Noto Emoji:

❤ ☀ ☁ ☂ ☕ ❄ ⚡ ✅ ❌ ⭐ 🌙 🌞 🌧 🌡 🌈 🎵 🎶 🎧 🎤 🎸 🎹 🎷 🎺 🎻 🥁 📻 💿 📀
🔊 🔇 🔔 🔥 💡 🏠 😀 😊 😂 😍 😎 😢 😭 👍 👏 💙 💚 💛 💜 🎉

Dynamic labels updated through `onx_render::label` show Noto Color Emoji,
including joined families and professions, skin-tone variants, country flags,
subdivision flags, and keycaps. Color emoji are **not stored in the firmware**:
only the list of 5,575 known sequences is. The first time a label contains one,
the knob downloads that emoji's Noto SVG from Google's emoji CDN (`fonts.gstatic.com`),
draws it in color at 48 px and redraws the label. Until then, and when offline,
the text keeps its original characters, so the monochrome set above still
appears. While the text is still on screen, a failed download is retried after
2, 5 and 15 seconds; after that, every 10 minutes.

Matching uses the longest supported UTF-8 sequence before LVGL lays out the
text; emoji presentation selectors are normalized. Unknown sequences retain
their text fallback.

## Font source and license

`fonts/NotoEmoji.ttf` is the monochrome variable Noto Emoji font from Google Fonts,
vendored at commit `8b0a1d0f5983c89bc2b93f1b5fb55f9e252744b5`:
https://github.com/google/fonts/tree/8b0a1d0f5983c89bc2b93f1b5fb55f9e252744b5/ofl/notoemoji

Its SIL Open Font License is retained in `fonts/OFL-NotoEmoji.txt`.

Downloads share the icon worker (one at a time, after icons and album art) and
need Wi-Fi, a synchronized clock and the same free-memory margin as icons. Up to
48 downloaded emoji stay in PSRAM (about 9 KiB each); scaled copies for each
text size use a 512 KiB cache that is retired after the current frame completes.
Emoji are downloaded again after a restart.

Regenerate the sequence list with
`python scripts/generate_color_emoji.py /path/to/NotoColorEmoji.ttf` (fontTools
required). Run `g++ -std=c++17 -Itests/emoji_stubs tests/test_emoji.cpp -o /tmp/test-emoji && /tmp/test-emoji`
for the sequence, download hand-off and scaling tests.

Sequence lookup narrows sorted UTF-8 prefixes with binary searches rather than
scanning the atlas for every character. Identical pixel buffers are shared by
multiple sequence keys. First use decodes and scales a glyph into PSRAM; later
uses share that cached image. The atlas uses 32-color RGBA palettes to preserve
colored artwork while keeping the firmware within the OTA flash partition.
