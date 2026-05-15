// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
//
// Split Params (Issue #1): refactor into RotationParams and ObliqueParams as requested by Zvezdochiot;
// in the dev branch this is implemented (single <params> XML preserved for compatibility).

#ifndef SCANTAILOR_DESKEW_PARAMS_H_
#define SCANTAILOR_DESKEW_PARAMS_H_

#include <QString>
#include <algorithm>
#include <cmath>

#include "AutoManualMode.h"
#include "Dependencies.h"

class QDomDocument;
class QDomElement;

namespace deskew {

/** Rotation (deskew) angle and auto/manual mode. */
struct RotationParams {
  double angle = 0.0;
  AutoManualMode mode = MODE_AUTO;
};

/** Oblique (shear) angle in degrees and whether to run automatic oblique detection. */
struct ObliqueParams {
  double obliqueAngle = 0.0;
  bool autoOblique = true;
};

class Params {
 public:
  // Member-wise copying is OK.

  Params(double deskewAngleDeg, const Dependencies& deps, AutoManualMode mode);

  Params(double deskewAngleDeg, double obliqueDeg, const Dependencies& deps, AutoManualMode mode, bool autoOblique);

  explicit Params(const QDomElement& deskewEl);

  ~Params();

  double deskewAngle() const;

  double obliqueAngle() const;

  bool autoOblique() const;

  const Dependencies& dependencies() const;

  AutoManualMode mode() const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  RotationParams m_rotation;
  ObliqueParams m_oblique;
  Dependencies m_deps;
};


inline double Params::deskewAngle() const {
  return m_rotation.angle;
}

inline double Params::obliqueAngle() const {
  return m_oblique.obliqueAngle;
}

inline bool Params::autoOblique() const {
  return m_oblique.autoOblique;
}

inline const Dependencies& Params::dependencies() const {
  return m_deps;
}

inline AutoManualMode Params::mode() const {
  return m_rotation.mode;
}
}  // namespace deskew

#endif  // ifndef SCANTAILOR_DESKEW_PARAMS_H_
