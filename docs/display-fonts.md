# Display character support

Text fonts (12–32 px) explicitly include `GF_Latin_Kernel` and `GF_Latin_Core`.
This retains ordinary English text while adding accented Latin letters used in
European and American languages, including `Beyoncé`, `Mañana`, `München`, and
`São Paulo`. Large clock fonts retain their smaller original character set.

Text fonts also include a bounded set of monochrome emoji from Noto Emoji:

❤ ☀ ☁ ☂ ☕ ❄ ⚡ ✅ ❌ ⭐ 🌙 🌞 🌧 🌡 🌈 🎵 🎶 🎧 🎤 🎸 🎹 🎷 🎺 🎻 🥁 📻 💿 📀
🔊 🔇 🔔 🔥 💡 🏠 😀 😊 😂 😍 😎 😢 😭 👍 👏 💙 💚 💛 💜 🎉

Media titles and artist names discard the invisible U+FE0E/U+FE0F presentation
selectors, so a heart followed by an emoji presentation selector uses the same
monochrome heart glyph. Other UTF-8 text is preserved.

This is a selected glyph set, not full Unicode or color-emoji rendering. Complex
joined emoji, flags, skin-tone modifiers, and arbitrary combining-character
sequences are not supported. Additional single-codepoint emoji can be added to
the `text_emoji` glyph list in `packages/base/display.yaml` if the source font
contains them. Each extra glyph and font size increases firmware size.

## Font source and license

`fonts/NotoEmoji.ttf` is the monochrome variable Noto Emoji font from Google Fonts,
vendored at commit `8b0a1d0f5983c89bc2b93f1b5fb55f9e252744b5`:
https://github.com/google/fonts/tree/8b0a1d0f5983c89bc2b93f1b5fb55f9e252744b5/ofl/notoemoji

Its SIL Open Font License is retained in `fonts/OFL-NotoEmoji.txt`.
