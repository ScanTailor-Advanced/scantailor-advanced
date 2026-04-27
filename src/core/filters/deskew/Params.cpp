// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
// Split Params: Zvezdochiot; dev branch implements as requested (see Params.h).

#include "Params.h"

#include <foundation/Utils.h>

#include <QDomDocument>

using namespace foundation;

namespace deskew {
Params::Params(const double deskewAngleDeg, const Dependencies& deps, const AutoManualMode deskewMode)
    : m_rotation{deskewAngleDeg, deskewMode}, m_oblique{0.0, MODE_AUTO}, m_deps(deps) {}

Params::Params(const double deskewAngleDeg,
               const double obliqueDeg,
               const Dependencies& deps,
               const AutoManualMode deskewMode,
               const AutoManualMode obliqueMode)
    : m_rotation{deskewAngleDeg, deskewMode}, m_oblique{obliqueDeg, obliqueMode}, m_deps(deps) {}

Params::Params(const double deskewAngleDeg,
               const double obliqueDeg,
               const Dependencies& deps,
               const AutoManualMode mode)
    : m_rotation{deskewAngleDeg, mode}, m_oblique{obliqueDeg, mode}, m_deps(deps) {}

Params::Params(const QDomElement& deskewEl)
    : m_rotation{deskewEl.attribute("angle").toDouble(),
                 deskewEl.attribute("mode") == "manual" ? MODE_MANUAL : MODE_AUTO},
      m_oblique{deskewEl.attribute("oblique").toDouble(),
                deskewEl.hasAttribute("oblique-mode")
                    ? (deskewEl.attribute("oblique-mode") == "manual" ? MODE_MANUAL : MODE_AUTO)
                    : (deskewEl.attribute("mode") == "manual" ? MODE_MANUAL : MODE_AUTO)},
      m_deps(deskewEl.namedItem("dependencies").toElement()) {}

Params::~Params() = default;

QDomElement Params::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("mode", m_rotation.mode == MODE_AUTO ? "auto" : "manual");
  el.setAttribute("angle", Utils::doubleToString(m_rotation.angle));
  el.setAttribute("oblique", Utils::doubleToString(m_oblique.obliqueAngle));
  el.setAttribute("oblique-mode", m_oblique.mode == MODE_AUTO ? "auto" : "manual");
  el.appendChild(m_deps.toXml(doc, "dependencies"));
  return el;
}
}  // namespace deskew