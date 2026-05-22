// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageId.h>
#include <PageId.h>
#include <PageInfo.h>
#include <PageSequence.h>

#include <boost/test/unit_test.hpp>
#include <set>
#include <vector>

namespace Tests {

static PageInfo makePage(const char* path, const int imageIdx) {
  PageInfo info;
  info.setId(PageId(ImageId(path, imageIdx), PageId::SINGLE_PAGE));
  return info;
}

BOOST_AUTO_TEST_SUITE(PageSequenceTestSuite)

BOOST_AUTO_TEST_CASE(select_this_page_and_following_every_other_from_first) {
  PageSequence seq;
  std::vector<PageInfo> pages;
  for (int i = 0; i < 75; ++i) {
    pages.push_back(makePage("/scan", i));
    seq.append(pages.back());
  }

  const PageId base = pages[0].id();
  const std::set<PageId> result = seq.selectThisPageAndFollowingEveryOther(base);

  BOOST_CHECK_EQUAL(result.size(), 38u);
  for (int i = 0; i < 75; i += 2) {
    BOOST_CHECK(result.count(pages[static_cast<size_t>(i)].id()) == 1);
  }
  for (int i = 1; i < 75; i += 2) {
    BOOST_CHECK(result.count(pages[static_cast<size_t>(i)].id()) == 0);
  }
}

BOOST_AUTO_TEST_CASE(select_this_page_and_following_every_other_from_second) {
  PageSequence seq;
  std::vector<PageInfo> pages;
  for (int i = 0; i < 75; ++i) {
    pages.push_back(makePage("/scan", i));
    seq.append(pages.back());
  }

  const PageId base = pages[1].id();
  const std::set<PageId> result = seq.selectThisPageAndFollowingEveryOther(base);

  BOOST_CHECK_EQUAL(result.size(), 37u);
  for (int i = 1; i < 75; i += 2) {
    BOOST_CHECK(result.count(pages[static_cast<size_t>(i)].id()) == 1);
  }
  for (int i = 0; i < 75; i += 2) {
    BOOST_CHECK(result.count(pages[static_cast<size_t>(i)].id()) == 0);
  }
}

BOOST_AUTO_TEST_CASE(select_this_page_and_following_every_other_partitions_75_pages) {
  PageSequence seq;
  std::vector<PageInfo> pages;
  for (int i = 0; i < 75; ++i) {
    pages.push_back(makePage("/scan", i));
    seq.append(pages.back());
  }

  const std::set<PageId> oddFromFirst = seq.selectThisPageAndFollowingEveryOther(pages[0].id());
  const std::set<PageId> evenFromSecond = seq.selectThisPageAndFollowingEveryOther(pages[1].id());

  for (int i = 0; i < 75; ++i) {
    const bool inOdd = oddFromFirst.count(pages[static_cast<size_t>(i)].id()) != 0;
    const bool inEven = evenFromSecond.count(pages[static_cast<size_t>(i)].id()) != 0;
    BOOST_CHECK(inOdd != inEven);
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Tests
