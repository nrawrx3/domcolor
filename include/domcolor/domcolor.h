#pragma once

#include <domcolor/defs.h>

#include "dkm_parallel.hpp"
#include "stb_image_write.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <inttypes.h>
#include <stdbool.h>
#include <string>
#include <vector>

using f64 = double;
using u8 = uint8_t;

constexpr int NUM_CHANNELS = 3;
constexpr f64 NORMALIZE_COLOR_FACTOR = 1.0 / 255;

using Color = std::array<f64, 3>;

struct alignas(u8) ColorRGB8 {
  u8 r;
  u8 g;
  u8 b;

  static auto from_normalized(Color c) -> ColorRGB8
  {
    return ColorRGB8{ u8(c[0] * 255), u8(c[1] * 255), u8(c[2] * 255) };
  }
};

struct LoadedImage {
  std::string filepath;
  std::vector<Color> pixels;
  int height;
  int width;
  int channels;

  void write_to_file(std::filesystem::path output_file)
  {
    auto unnormalized = std::vector<ColorRGB8>{};
    unnormalized.reserve(height * width);
    for (auto color : pixels) {
      const auto color8 = ColorRGB8::from_normalized(color);
      unnormalized.push_back(color8);
    }

    auto file_path_str = output_file.u8string();

    stbi_write_png(file_path_str.c_str(),
                   width,
                   height,
                   3,
                   reinterpret_cast<void *>(unnormalized.data()),
                   sizeof(ColorRGB8) * width);
  }
};

struct ClusterResult {
  std::vector<Color> mean_color_of_cluster;
  std::vector<uint32_t> cluster_of_pixel;
  std::vector<uint32_t> nth_cluster; // Clusters sorted by population in decreasing order

  uint32_t num_clusters;

  ClusterResult(uint32_t num_clusters)
      : nth_cluster(num_clusters)
      , num_clusters(num_clusters)
  {
    printf("num_clusters: %u", num_clusters);
  }

  auto count_and_sort_by_population(bool do_sort = true)
  {
    const auto population_of_cluster = _count_cluster_populations();

    for (uint32_t n = 0; n < num_clusters; n++) {
      nth_cluster[n] = n;
    }

    if (!do_sort) {
      return;
    }

    std::sort(nth_cluster.begin(), nth_cluster.end(), [&](uint32_t c1, uint32_t c2) {
      return population_of_cluster[c1] > population_of_cluster[c2];
    });
  }

  auto _count_cluster_populations() -> std::vector<size_t>
  {
    std::vector<size_t> population_of_cluster(num_clusters);

#if defined(ENABLE_OPENMP)
#  pragma omp parallel for
#endif
    for (size_t pixel = 0; pixel < cluster_of_pixel.size(); pixel++) {
      population_of_cluster[cluster_of_pixel[pixel]]++;
    }
    return population_of_cluster;
  }
};

inline auto create_normalized_image(const u8 *rgb_bytes, int w, int h) -> std::vector<Color>
{
  std::vector<Color> pixels(w * h);
#pragma omp parallel
  for (int i = 0; i < w * h; i++) {
    pixels[i] = Color{ f64(rgb_bytes[i * NUM_CHANNELS]) * NORMALIZE_COLOR_FACTOR,
                       f64(rgb_bytes[i * NUM_CHANNELS + 1]) * NORMALIZE_COLOR_FACTOR,
                       f64(rgb_bytes[i * NUM_CHANNELS + 2]) * NORMALIZE_COLOR_FACTOR };
  }
  return pixels;
}

inline auto fill_with_color_rgb8(Color color, ColorRGB8 *out_buffer, size_t num_pixels)
{
  auto c = ColorRGB8{
    u8(color[0] * 255),
    u8(color[1] * 255),
    u8(color[2] * 255),
  };
  std::fill(out_buffer, out_buffer + num_pixels, c);
}

inline auto cluster_colors(const LoadedImage &img, uint32_t num_clusters, bool sort = true) -> ClusterResult
{
  dkm::clustering_parameters<f64> params(num_clusters);
  params.set_random_seed(0xdeadc0de);
  params.set_max_iteration(2);

#if defined(ENABLE_OPENMP)
  auto [mean_of_cluster, cluster_of_pixel] = dkm::kmeans_lloyd_parallel(img.pixels, params);
#else
  auto [mean_of_cluster, cluster_of_pixel] = dkm::kmeans_lloyd(img.pixels, params);
#endif

  // Find out which cluster most of the points are assigned to.
  ClusterResult result(num_clusters);
  result.mean_color_of_cluster = std::move(mean_of_cluster);
  result.cluster_of_pixel = std::move(cluster_of_pixel);
  result.count_and_sort_by_population(sort);
  return result;
}

// inline auto create_cluster_strip_image(const ClusterResult &result, uint32_t num_clusters)
//     -> std::vector<ColorRGB8>
// {
//   const size_t num_pixels_per_strip = STRIP_IMAGE_HEIGHT_PER_CLUSTER * STRIP_IMAGE_WIDTH;
//   const size_t img_height = num_clusters * STRIP_IMAGE_HEIGHT_PER_CLUSTER;

//   std::vector<ColorRGB8> cluster_strip_image(num_clusters * STRIP_IMAGE_WIDTH * img_height *
//                                              3); // 3 = num channel
//   for (size_t n = 0; n < num_clusters; n++) {
//     const auto cluster = result.nth_cluster[n];

//     ColorRGB8 *start = cluster_strip_image.data() + (n * num_pixels_per_strip);
//     fill_with_color_rgb8(result.mean_color_of_cluster[cluster], start, num_pixels_per_strip);
//   }

//   return cluster_strip_image;
// }
