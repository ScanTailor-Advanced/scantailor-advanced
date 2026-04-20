// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageTrim.h"

#include <algorithm>

namespace fix_orientation {

namespace {

const int kMinInnerSide = 32;

}  // namespace

QRect ImageTrim::toInnerRect(const QSize& imageSize) const {
  if (!enabled || imageSize.width() <= 0 || imageSize.height() <= 0) {
    return QRect(0, 0, imageSize.width(), imageSize.height());
  }

  int l = std::max(0, left);
  int t = std::max(0, top);
  int r = std::max(0, right);
  int b = std::max(0, bottom);

  int w = imageSize.width() - l - r;
  int h = imageSize.height() - t - b;
  w = std::max(kMinInnerSide, w);
  h = std::max(kMinInnerSide, h);

  if (l + w > imageSize.width()) {
    l = std::max(0, imageSize.width() - w);
  }
  if (t + h > imageSize.height()) {
    t = std::max(0, imageSize.height() - h);
  }

  return QRect(l, t, w, h);
}

}  // namespace fix_orientation
