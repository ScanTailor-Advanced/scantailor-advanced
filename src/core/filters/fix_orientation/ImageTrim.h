// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FIX_ORIENTATION_IMAGETRIM_H_
#define SCANTAILOR_FIX_ORIENTATION_IMAGETRIM_H_

#include <QRect>

namespace fix_orientation {

/**
 * Inward crop from each edge of the source image, in integer source pixels.
 * When disabled, other fields are ignored.
 */
struct ImageTrim {
  bool enabled = false;
  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;

  QRect toInnerRect(const QSize& imageSize) const;
};

}  // namespace fix_orientation

#endif
