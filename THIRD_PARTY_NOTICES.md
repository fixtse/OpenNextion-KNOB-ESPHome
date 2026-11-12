# Licensing and third-party notices

This project is licensed under the GNU Affero General Public License version 3
only (`AGPL-3.0-only`), except for third-party material described below or carrying
its own license notice. The full project license is in [LICENSE](LICENSE).
Original contributions by Sergio Romero are copyright (c) 2026 Sergio Romero.
This notice does not replace third-party copyright notices or relicense their
original material.

## OpenNextion example

This project is based on
[OpenNextion ESPHome Knob Control](https://github.com/OpenNextion/OpenNextion-Example-ESPHome_KNOB_Control).
OpenNextion's original material is copyright (c) 2026 OpenNextion and is available
under the MIT license. Its full notice is preserved in
[LICENSES/OpenNextion-MIT.txt](LICENSES/OpenNextion-MIT.txt), obtained from upstream
commit `4fa4264894588f3f66f4bcedba902bc44785ecfe` on 2026-11-12.
The inherited application code and configuration have been modified and extended
for this project; this project is not an official OpenNextion firmware release.
The MIT grant does not replace the licenses of third-party material bundled by
OpenNextion.

## ESPHome

[ESPHome](https://github.com/esphome/esphome/blob/2026.10.2/LICENSE) licenses its
C++/runtime code under GPLv3 and its Python code and other parts under MIT.
This includes ESPHome-derived display driver code in `my_components/ili9xxx/`
and adapted code/API material in `my_components/online_image/`. The ESPHome
license text is retained in
[my_components/online_image/LICENSE](my_components/online_image/LICENSE).
The build-time API batching patch also contains ESPHome source snippets.
GPLv3 portions retain GPLv3; combination with AGPLv3 code is permitted by section
13 of those licenses. Do not interpret the project license as relicensing
ESPHome's original code.

## Icons, SVG rendering and fonts

- Pictogrammers icons: retain [assets/entity_icons/LICENSE](assets/entity_icons/LICENSE)
  and [NOTICE.md](assets/entity_icons/NOTICE.md); icon terms are Apache 2.0 as
  described there. That notice applies to that collection, not every asset.
- NanoSVG: retain the copyright and license text in `nanosvg.h` and
  `nanosvgrast.h` and [NANOSVG_NOTICE.md](my_components/onx_icons/NANOSVG_NOTICE.md).
- Noto Emoji: SIL Open Font License 1.1; retain
  [fonts/OFL-NotoEmoji.txt](fonts/OFL-NotoEmoji.txt).
- MiSans: Xiaomi's font license applies, not AGPLv3 or the OpenNextion MIT grant.
  See [Xiaomi's terms](https://hyperos.mi.com/font/en/download/).
  The inherited font files still require separate redistribution review.
- Other inherited artwork remains subject to any original third-party terms;
  OpenNextion's MIT notice does not establish ownership of external artwork.

This licensing update records the project license and known third-party terms;
it does not claim that every bundled asset has received a complete rights audit.
When distributing compiled firmware, provide its corresponding source and
required build/install information under the applicable GPL/AGPL terms. Modified
versions supporting remote network interaction must meet AGPL section 13.
