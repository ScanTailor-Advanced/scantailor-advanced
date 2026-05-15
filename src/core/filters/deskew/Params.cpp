// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
// Split Params: Zvezdochiot; dev branch implements as requested (see Params.h).

#include "Params.h"

#include <foundation/Utils.h>

#include <QDomDocument>

using namespace foundation;

namespace deskew {
Params::Params(const double deskewAngleDeg, const Dependencies& deps, const AutoManualMode mode)
    : m_rotation{deskewAngleDeg, mode}, m_oblique{0.0, true}, m_deps(deps) {}

Params::Params(const double deskewAngleDeg,
               const double obliqueDeg,
               const Dependencies& deps,
               const AutoManualMode mode,
               const bool autoOblique)
    : m_rotation{deskewAngleDeg, mode}, m_oblique{obliqueDeg, autoOblique}, m_deps(deps) {}

Params::Params(const QDomElement& deskewEl)
    : m_rotation{deskewEl.attribute("angle").toDouble(),
                 deskewEl.attribute("mode") == "manual" ? MODE_MANUAL : MODE_AUTO},
      m_oblique{deskewEl.attribute("oblique").toDouble(), deskewEl.attribute("autoOblique", "1") != "0"},
      m_deps(deskewEl.namedItem("dependencies").toElement()) {}

Params::~Params() = default;

QDomElement Params::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("mode", m_rotation.mode == MODE_AUTO ? "auto" : "manual");
  el.setAttribute("angle", Utils::doubleToString(m_rotation.angle));
  el.setAttribute("oblique", Utils::doubleToString(m_oblique.obliqueAngle));
  el.setAttribute("autoOblique", m_oblique.autoOblique ? "1" : "0");
  el.appendChild(m_deps.toXml(doc, "dependencies"));
  return el;
}
}  // namespace deskew