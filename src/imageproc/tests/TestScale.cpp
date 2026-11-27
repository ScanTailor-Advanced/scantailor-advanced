// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <GrayImage.h>
#include <Scale.h>

#include <QImage>
#include <QSize>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(ScaleTestSuite)

BOOST_AUTO_TEST_CASE(test_null_image) {
  const GrayImage nullImg;
  BOOST_CHECK(scaleToGray(nullImg, QSize(1, 1)).isNull());
}

static bool fuzzyCompare(const QImage& img1, const QImage& img2) {
  BOOST_REQUIRE(img1.size() == img2.size());

  const int width = img1.width();
  const int height = img1.height();
  const uint8_t* line1 = img1.bits();
  const uint8_t* line2 = img2.bits();
  const int line1Bpl = img1.bytesPerLine();
  const int line2Bpl = img2.bytesPerLine();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (std::abs(int(line1[x]) - int(line2[x])) > 1) {
        return false;
      }
    }
    line1 += line1Bpl;
    line2 += line2Bpl;
  }
  return true;
}

// The exact mean of the source area under each destination pixel; it does not change with the Qt version.
static GrayImage areaAverage(const GrayImage& src, const QSize& dstSize) {
  const double xscale = double(src.width()) / dstSize.width();
  const double yscale = double(src.height()) / dstSize.height();
  GrayImage dst(dstSize);
  for (int dy = 0; dy < dstSize.height(); ++dy) {
    const double y0 = dy * yscale;
    const double y1 = y0 + yscale;
    for (int dx = 0; dx < dstSize.width(); ++dx) {
      const double x0 = dx * xscale;
      const double x1 = x0 + xscale;
      double sum = 0.0;
      for (int sy = int(std::floor(y0)); sy < std::min(src.height(), int(std::ceil(y1))); ++sy) {
        const double wy = std::min(y1, sy + 1.0) - std::max(y0, double(sy));
        const uint8_t* srcLine = src.data() + sy * src.stride();
        for (int sx = int(std::floor(x0)); sx < std::min(src.width(), int(std::ceil(x1))); ++sx) {
          const double wx = std::min(x1, sx + 1.0) - std::max(x0, double(sx));
          sum += wx * wy * srcLine[sx];
        }
      }
      dst.data()[dy * dst.stride() + dx] = static_cast<uint8_t>(std::lround(sum / (xscale * yscale)));
    }
  }
  return dst;
}

static bool checkScale(const GrayImage& img, const QSize& newSize) {
  const GrayImage scaled1(scaleToGray(img, newSize));
  const GrayImage scaled2(areaAverage(img, newSize));
  return fuzzyCompare(scaled1, scaled2);
}

BOOST_AUTO_TEST_CASE(test_random_image) {
  GrayImage img(QSize(100, 100));
  uint8_t* line = img.data();
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      line[x] = static_cast<uint8_t>(rand() % 256);
    }
    line += img.stride();
  }

  // A larger image gets values interpolated between pixels, so the area mean is a reference only for smaller images.
  BOOST_CHECK(checkScale(img, QSize(50, 50)));
  BOOST_CHECK(checkScale(img, QSize(80, 80)));
  BOOST_CHECK(checkScale(img, QSize(7, 13)));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc