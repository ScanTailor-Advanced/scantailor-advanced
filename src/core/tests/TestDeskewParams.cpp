// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <DefaultParams.h>
#include <Dpi.h>
#include <ImageTransformation.h>
#include <filters/deskew/Dependencies.h>
#include <filters/deskew/Params.h>

#include <QDomDocument>
#include <QPolygonF>
#include <QRectF>
#include <QTransform>
#include <boost/test/unit_test.hpp>

namespace Tests {
using namespace deskew;

BOOST_AUTO_TEST_SUITE(DeskewParamsTestSuite)

BOOST_AUTO_TEST_CASE(params_oblique_roundtrip_xml) {
  const double deskewDeg = 1.5;
  const double obliqueDeg = 2.25;
  const Dependencies deps;
  const Params original(deskewDeg, obliqueDeg, deps, MODE_MANUAL, MODE_MANUAL);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  const Params restored(doc.documentElement());

  BOOST_CHECK_CLOSE(restored.deskewAngle(), deskewDeg, 1e-6);
  BOOST_CHECK_CLOSE(restored.obliqueAngle(), obliqueDeg, 1e-6);
  BOOST_CHECK(restored.mode() == MODE_MANUAL);
  BOOST_CHECK(restored.obliqueMode() == MODE_MANUAL);
}

BOOST_AUTO_TEST_CASE(params_zero_oblique_roundtrip_xml) {
  const double deskewDeg = -0.5;
  const Dependencies deps;
  const Params original(deskewDeg, 0.0, deps, MODE_AUTO, MODE_AUTO);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  const Params restored(doc.documentElement());

  BOOST_CHECK_CLOSE(restored.deskewAngle(), deskewDeg, 1e-6);
  BOOST_CHECK_SMALL(restored.obliqueAngle(), 1e-9);
  BOOST_CHECK(restored.mode() == MODE_AUTO);
  BOOST_CHECK(restored.obliqueMode() == MODE_AUTO);
}

BOOST_AUTO_TEST_CASE(params_independent_oblique_mode_xml) {
  const Dependencies deps;
  const Params original(1.0, 2.0, deps, MODE_MANUAL, MODE_AUTO);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  const Params restored(doc.documentElement());

  BOOST_CHECK(restored.mode() == MODE_MANUAL);
  BOOST_CHECK(restored.obliqueMode() == MODE_AUTO);
}

BOOST_AUTO_TEST_CASE(params_oblique_mode_attribute_roundtrip_xml) {
  const Dependencies deps;
  const Params original(-1.25, 0.75, deps, MODE_AUTO, MODE_MANUAL);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  BOOST_CHECK(el.hasAttribute("oblique-mode"));
  BOOST_CHECK(el.attribute("oblique-mode") == "manual");

  const Params restored(doc.documentElement());
  BOOST_CHECK(restored.mode() == MODE_AUTO);
  BOOST_CHECK(restored.obliqueMode() == MODE_MANUAL);
  BOOST_CHECK_CLOSE(restored.deskewAngle(), -1.25, 1e-6);
  BOOST_CHECK_CLOSE(restored.obliqueAngle(), 0.75, 1e-6);
}

/** Selective Apply To… merge semantics (issue #117): deskew-only must not overwrite oblique. */
BOOST_AUTO_TEST_CASE(apply_merge_deskew_only_preserves_oblique) {
  const Dependencies deps;
  const Params existing(1.0, 3.0, deps, MODE_MANUAL, MODE_MANUAL);
  const Params current(2.0, 4.0, deps, MODE_AUTO, MODE_AUTO);

  const Params merged(current.deskewAngle(), existing.obliqueAngle(), deps, current.mode(), existing.obliqueMode());

  BOOST_CHECK_CLOSE(merged.deskewAngle(), 2.0, 1e-6);
  BOOST_CHECK_CLOSE(merged.obliqueAngle(), 3.0, 1e-6);
  BOOST_CHECK(merged.mode() == MODE_AUTO);
  BOOST_CHECK(merged.obliqueMode() == MODE_MANUAL);
}

/** Selective Apply To… merge semantics (issue #117): oblique-only must not overwrite deskew. */
BOOST_AUTO_TEST_CASE(apply_merge_oblique_only_preserves_deskew) {
  const Dependencies deps;
  const Params existing(1.0, 3.0, deps, MODE_MANUAL, MODE_MANUAL);
  const Params current(2.0, 4.0, deps, MODE_AUTO, MODE_AUTO);

  const Params merged(existing.deskewAngle(), current.obliqueAngle(), deps, existing.mode(), current.obliqueMode());

  BOOST_CHECK_CLOSE(merged.deskewAngle(), 1.0, 1e-6);
  BOOST_CHECK_CLOSE(merged.obliqueAngle(), 4.0, 1e-6);
  BOOST_CHECK(merged.mode() == MODE_MANUAL);
  BOOST_CHECK(merged.obliqueMode() == MODE_AUTO);
}

BOOST_AUTO_TEST_CASE(params_auto_oblique_false_roundtrip_xml) {
  const Dependencies deps;
  const Params original(1.0, 0.25, deps, MODE_AUTO, MODE_MANUAL);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  const Params restored(doc.documentElement());

  BOOST_CHECK(restored.obliqueMode() == MODE_MANUAL);
  BOOST_CHECK(!restored.autoOblique());
  BOOST_CHECK_CLOSE(restored.obliqueAngle(), 0.25, 1e-6);
}

BOOST_AUTO_TEST_CASE(params_missing_autoOblique_attribute_defaults_true) {
  QDomDocument doc;
  QDomElement el(doc.createElement("params"));
  el.setAttribute("mode", "auto");
  el.setAttribute("angle", "0");
  el.setAttribute("oblique", "0");
  const Dependencies deps;
  el.appendChild(deps.toXml(doc, "dependencies"));
  doc.appendChild(el);

  const Params restored(doc.documentElement());
  BOOST_CHECK(restored.autoOblique());
}

BOOST_AUTO_TEST_CASE(default_params_deskew_auto_oblique_off_by_default) {
  const DefaultParams::DeskewParams deskew;
  BOOST_CHECK(!deskew.isAutoOblique());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ImageTransformationObliqueTestSuite)

BOOST_AUTO_TEST_CASE(post_oblique_applied_and_stored) {
  const QRectF rect(0, 0, 100, 100);
  const Dpi dpi(300, 300);
  ImageTransformation xform(rect, dpi);

  QPolygonF preCrop;
  preCrop << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft();
  xform.setPreCropArea(preCrop);

  xform.setPostOblique(0.0);
  const QTransform transformZeroOblique = xform.transform();
  const double obliqueDeg = 2.0;
  xform.setPostOblique(obliqueDeg);

  BOOST_CHECK_CLOSE(xform.postOblique(), obliqueDeg, 1e-6);
  BOOST_CHECK(transformZeroOblique != xform.transform());
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Tests
