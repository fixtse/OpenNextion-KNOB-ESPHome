#define NANOSVG_IMPLEMENTATION
#include "../my_components/onx_icons/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../my_components/onx_icons/nanosvgrast.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
int main() {
  // Exercise the same parser/rasterizer used by downloads, with curves and cut-outs.
  for (auto name : {"fan", "thermostat", "music-note", "window-shutter", "lightbulb"}) {
    std::ifstream input(std::string("assets/entity_icons/") + name + ".svg");
    assert(input);
    std::string svg((std::istreambuf_iterator<char>(input)), {}); svg.push_back(0);
    auto *image = nsvgParse(svg.data(), "px", 96); assert(image && image->shapes);
    auto *rasterizer = nsvgCreateRasterizer(); assert(rasterizer);
    std::vector<unsigned char> pixels(80 * 80 * 4);
    nsvgRasterize(rasterizer, image, 0, 0, 80 / image->width, pixels.data(), 80, 80, 80 * 4);
    size_t visible = 0, clear = 0;
    for (size_t i = 3; i < pixels.size(); i += 4) { visible += pixels[i] > 0; clear += pixels[i] == 0; }
    assert(visible > 100 && clear > 100);
    nsvgDeleteRasterizer(rasterizer); nsvgDelete(image);
  }
}
