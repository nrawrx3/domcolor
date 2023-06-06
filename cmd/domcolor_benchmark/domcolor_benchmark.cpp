#define STB_IMAGE_IMPLEMENTATION 1
#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#define ANKERL_NANOBENCH_IMPLEMENT

#define FLAG_NUM_CLUSTERS 3

#include "nanobench.h"

#include <domcolor/argparse.hpp>
#include <domcolor/domcolor.h>
#include <domcolor/stb_image.h>
#include <filesystem>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>

#include <atomic>

namespace fs = std::filesystem;

int main(int ac, const char **av)
{
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::verbose, &consoleAppender);

  argparse::ArgumentParser ap("domcolor_benchmark");
  ap.add_argument("-f", "--file").help("path to image file");

  ap.parse_args(ac, av);

  auto file = fs::path(ap.get<std::string>("file"));

  auto loaded_image = LoadedImage{
    .filepath = file.u8string(),
  };

  auto loaded_bytes =
      stbi_load(file.c_str(), &loaded_image.width, &loaded_image.height, &loaded_image.channels, STBI_rgb);

  if (loaded_bytes == nullptr) {
    PLOGE.printf("failed to load image from given file: %s", file.c_str());
    return 1;
  }

  if (loaded_image.channels != 3) {
    PLOGE.printf("expected (hardcoded) %d channels, have %d channels in image file",
                 NUM_CHANNELS,
                 loaded_image.channels);
  }

  loaded_image.pixels = create_normalized_image(loaded_bytes, loaded_image.width, loaded_image.height);

  free(loaded_bytes);
  loaded_bytes = nullptr;

  ankerl::nanobench::Bench().run("cluster_colors_no_sort", [&] {
    auto result = cluster_colors(loaded_image, false);
    ankerl::nanobench::doNotOptimizeAway(result);
  });

  ankerl::nanobench::Bench().run("cluster_colors_sort", [&] {
    auto result = cluster_colors(loaded_image, true);
    ankerl::nanobench::doNotOptimizeAway(result);
  });
}