#define STB_IMAGE_IMPLEMENTATION 1
#define STB_IMAGE_WRITE_IMPLEMENTATION 1

#include "median_filter.h"

#include <domcolor/argparse.hpp>
#include <domcolor/dkm_parallel.hpp>
#include <domcolor/domcolor.h>
#include <domcolor/stb_image.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>

#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

template <int32_t filter_size>
auto apply_gaussian_filter(const LoadedImage &source) -> std::optional<LoadedImage> {
  
}

template <int32_t filter_size>
auto apply_median_filter(const LoadedImage &source) -> std::optional<LoadedImage>
{
  // W - F + 2P + 1
  // We want the output image to be the same dimension. Then
  //      W - F + 2P + 1 = W
  //      P = (F - 1) / 2

  if (filter_size % 2 == 0) {
    return {};
  }

  int32_t dest_width = source.width - (filter_size - 1);
  int32_t dest_height = source.height - (filter_size - 1);

  LoadedImage dest = source;

  int32_t odd_half_size = (filter_size - 1) / 2;

#pragma omp parallel
  for (int32_t y = odd_half_size; y < dest_height; y++) {
    for (int32_t x = odd_half_size; x < dest_width; x++) {
      auto pixels = std::array<Color, filter_size * filter_size>{};
      auto intensities = std::array<f64, filter_size * filter_size>{};
      auto indices = std::array<int32_t, filter_size * filter_size>{};

      int32_t i = 0;
      for (int32_t x_diff = -odd_half_size; x_diff <= odd_half_size; x_diff++) {
        for (int32_t y_diff = -odd_half_size; y_diff <= odd_half_size; y_diff++) {
          auto sample_x = x + x_diff;
          auto sample_y = y + y_diff;
          auto sample_index = sample_y * source.width + sample_x;
          const auto pixel = source.pixels[sample_index];
          pixels[i] = pixel;
          intensities[i] = (0.2126 * pixel[0]) + (0.7152 * pixel[1]) + (0.0722 * pixel[2]);
          indices[i] = i;
          i++;
        }
      }

      std::sort(
          indices.begin(), indices.end(), [&](auto i, auto j) { return intensities[i] < intensities[j]; });

      dest.pixels[y * source.width + x] = pixels[indices[odd_half_size]];
    }
  }

  return dest;
}

auto load_image(fs::path file) -> LoadedImage
{
  auto loaded_image = LoadedImage{
    .filepath = file.u8string(),
  };

  auto loaded_bytes = stbi_load(loaded_image.filepath.c_str(),
                                &loaded_image.width,
                                &loaded_image.height,
                                &loaded_image.channels,
                                STBI_rgb);

  if (loaded_bytes == nullptr) {
    PLOGE.printf("failed to load image from given file: %s", loaded_image.filepath.c_str());
    exit(1);
  }

  if (loaded_image.channels != 3) {
    PLOGE.printf("expected (hardcoded) %d channels, have %d channels in image file",
                 NUM_CHANNELS,
                 loaded_image.channels);
    exit(1);
  }

  loaded_image.pixels = create_normalized_image(loaded_bytes, loaded_image.width, loaded_image.height);

  free(loaded_bytes);
  loaded_bytes = nullptr;

  return loaded_image;
}

auto create_palette_from_file(fs::path file, uint32_t num_clusters) -> ClusterResult
{
  auto loaded_image = load_image(file);
  PLOGI.printf("loaded image file: %s", loaded_image.filepath.c_str());

  auto result = cluster_colors(loaded_image, num_clusters);
  PLOGI.printf("Created clusters: %s", loaded_image.filepath.c_str());

  result.count_and_sort_by_population();
  PLOGI.printf("Sorted clusters: %s", loaded_image.filepath.c_str());
  return result;
}

int main(int ac, const char **av)
{
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::verbose, &consoleAppender);

  argparse::ArgumentParser ap("palette_transfer");
  ap.add_argument("-s", "--palette-source")
      .help("path to source image from which the palette will be created")
      .required();
  ap.add_argument("-t", "--palette-target")
      .help("path to destination image which will be converted")
      .required();
  ap.add_argument("-o", "--output-file").help("output file path").required();
  ap.add_argument("-k", "--num-clusters").help("number of clusters in the palette").required();

  ap.parse_args(ac, av);

  auto num_clusters_str = ap.get<std::string>("num-clusters");

  auto num_clusters = std::stoi(num_clusters_str);

  PLOGI.printf("num_clusters = %u", num_clusters);

  auto source_cluster =
      create_palette_from_file(fs::path(ap.get<std::string>("palette-source")), num_clusters);
  auto destination_cluster =
      create_palette_from_file(fs::path(ap.get<std::string>("palette-target")), num_clusters);

  // Load the destination image again and map clusters.
  auto destination_image = load_image(fs::path(ap.get<std::string>("palette-target")));

#pragma omp parallel
  for (int y = 0; y < destination_image.height; y++) {
    for (int x = 0; x < destination_image.width; x++) {
      int pixel_index = y * destination_image.width + x;
      auto dest_pixel_cluster = destination_cluster.cluster_of_pixel[pixel_index];
      auto new_pixel_color = source_cluster.mean_color_of_cluster[dest_pixel_cluster];
      destination_image.pixels[pixel_index] = new_pixel_color;
    }
  }

  // Apply median filter
  auto median_filter_output = apply_median_filter<3>(destination_image);
  median_filter_output.value().write_to_file(fs::path(ap.get<std::string>("output-file")));

  PLOGI.printf("DONE creating paletted image");
}
