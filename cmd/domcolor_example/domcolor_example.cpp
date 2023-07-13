#define STB_IMAGE_IMPLEMENTATION 1
#define STB_IMAGE_WRITE_IMPLEMENTATION 1

#define FLAG_NUM_CLUSTERS 3

#include <domcolor/argparse.hpp>
#include <domcolor/dkm_parallel.hpp>
#include <domcolor/domcolor.h>
#include <domcolor/stb_image.h>
#include <domcolor/stb_image_write.h>
#include <filesystem>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>

namespace fs = std::filesystem;

int main(int ac, const char **av)
{
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::verbose, &consoleAppender);

  argparse::ArgumentParser ap("domcolor_example");
  ap.add_argument("-f", "--file").help("path to image file");
  // TODO: allow cutout

  ap.parse_args(ac, av);

  auto file = fs::path(ap.get<std::string>("file"));

  auto cluster_strip_file = std::string("clusters_") + file.filename().u8string();

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

  auto result = cluster_colors(loaded_image, NUM_CLUSTERS);

  for (size_t cluster = 0; cluster < result.mean_color_of_cluster.size(); cluster++) {
    auto mean = ColorRGB8::from_normalized(result.mean_color_of_cluster[cluster]);
    printf("Cluster: %zu, Mean color: %u, %u, %u\n", cluster, mean.r, mean.g, mean.b);
  }

  PLOGI.printf("Creating cluster strip image at %s", cluster_strip_file.c_str());
  auto cluster_strip_image = create_cluster_strip_image(result);

  stbi_write_png(cluster_strip_file.c_str(),
                 STRIP_IMAGE_WIDTH,
                 STRIP_IMAGE_HEIGHT,
                 3,
                 reinterpret_cast<void *>(cluster_strip_image.data()),
                 sizeof(ColorRGB8) * STRIP_IMAGE_WIDTH);

  PLOGI.printf("DONE creating cluster strip image at %s", cluster_strip_file.c_str());
}