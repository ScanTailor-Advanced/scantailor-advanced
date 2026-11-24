// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ProjectPages.h>
#include <ProjectReader.h>

#include <QDomDocument>
#include <QString>
#include <boost/test/unit_test.hpp>

#include "version.h"

BOOST_AUTO_TEST_SUITE(CoreProjectReaderTestSuite)

BOOST_AUTO_TEST_CASE(test_readable_versions) {
  BOOST_CHECK(ProjectReader::isVersionReadable(QString::number(PROJECT_VERSION)));
  // Projects saved by 1.0.x and 1.1.x must stay readable.
  BOOST_CHECK(ProjectReader::isVersionReadable("3"));
  BOOST_CHECK(!ProjectReader::isVersionReadable("2"));
  BOOST_CHECK(!ProjectReader::isVersionReadable(QString::number(PROJECT_VERSION + 1)));
  BOOST_CHECK(!ProjectReader::isVersionReadable(QString()));
  BOOST_CHECK(!ProjectReader::isVersionReadable("abc"));
}

BOOST_AUTO_TEST_CASE(test_reads_version_3_project) {
  QDomDocument doc;
  doc.setContent(
      QString("<project layoutDirection=\"LTR\" outputDirectory=\"/out\" version=\"3\">"
              "<directories><directory id=\"1\" path=\"/in\"/></directories>"
              "<files><file id=\"2\" dirId=\"1\" name=\"a.tif\"/></files>"
              "<images><image id=\"3\" fileId=\"2\" fileImage=\"0\" subPages=\"1\">"
              "<size width=\"100\" height=\"200\"/><dpi horizontal=\"300\" vertical=\"300\"/></image></images>"
              "<pages><page id=\"4\" imageId=\"3\" subPage=\"single\" selected=\"selected\"/></pages>"
              "<file-name-disambiguation/><filters/></project>"));

  const ProjectReader reader(doc);
  BOOST_REQUIRE(reader.success());
  BOOST_CHECK(reader.outputDirectory() == QString("/out"));
  BOOST_CHECK(reader.pages()->numImages() == 1);
}

BOOST_AUTO_TEST_SUITE_END()
