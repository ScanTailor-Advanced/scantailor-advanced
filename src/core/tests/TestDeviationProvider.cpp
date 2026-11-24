// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <DeviationProvider.h>

#include <atomic>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <limits>
#include <thread>
#include <vector>

BOOST_AUTO_TEST_SUITE(CoreDeviationProviderTestSuite)

BOOST_AUTO_TEST_CASE(test_unknown_key) {
  DeviationProvider<int> provider;
  BOOST_CHECK(!provider.isDeviant(1));
  BOOST_CHECK_EQUAL(provider.getDeviationValue(1), -1.0);
}

BOOST_AUTO_TEST_CASE(test_outlier_is_deviant) {
  DeviationProvider<int> provider;
  for (int i = 0; i < 9; ++i) {
    provider.addOrUpdate(i, 10.0);
  }
  provider.addOrUpdate(9, 100.0);

  BOOST_CHECK(provider.isDeviant(9));
  BOOST_CHECK(!provider.isDeviant(0));
  // mean = 19, so the outlier is 81 away and the others 9 away.
  BOOST_CHECK_CLOSE(provider.getDeviationValue(9), 81.0, 1e-9);
  BOOST_CHECK_CLOSE(provider.getDeviationValue(0), 9.0, 1e-9);
}

BOOST_AUTO_TEST_CASE(test_statistics_follow_changes) {
  DeviationProvider<int> provider;
  for (int i = 0; i < 10; ++i) {
    provider.addOrUpdate(i, 10.0);
  }
  BOOST_CHECK(!provider.isDeviant(3));
  BOOST_CHECK_SMALL(provider.getDeviationValue(3), 1e-12);

  // Re-submitting an unchanged value must not disturb the result ...
  provider.addOrUpdate(3, 10.0);
  BOOST_CHECK(!provider.isDeviant(3));

  // ... while a changed value must be picked up.
  provider.addOrUpdate(3, 500.0);
  BOOST_CHECK(provider.isDeviant(3));
  BOOST_CHECK_CLOSE(provider.getDeviationValue(3), 441.0, 1e-9);

  provider.remove(3);
  BOOST_CHECK(!provider.isDeviant(3));
  BOOST_CHECK_SMALL(provider.getDeviationValue(0), 1e-12);

  provider.clear();
  BOOST_CHECK_EQUAL(provider.getDeviationValue(0), -1.0);
}

BOOST_AUTO_TEST_CASE(test_nan_values_are_ignored) {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  DeviationProvider<int> provider;
  provider.addOrUpdate(0, 10.0);
  provider.addOrUpdate(1, 10.0);
  provider.addOrUpdate(2, 10.0);
  provider.addOrUpdate(3, nan);
  provider.addOrUpdate(3, nan);

  BOOST_CHECK(!provider.isDeviant(3));
  BOOST_CHECK(provider.isDeviant(3, 1.0, 0.0, true));
  BOOST_CHECK_EQUAL(provider.getDeviationValue(3), -1.0);
  BOOST_CHECK_SMALL(provider.getDeviationValue(0), 1e-12);
}

BOOST_AUTO_TEST_CASE(test_compute_callback) {
  DeviationProvider<int> provider([](const int& key) { return key == 7 ? 70.0 : 1.0; });
  for (int i = 0; i < 10; ++i) {
    provider.addOrUpdate(i);
  }
  BOOST_CHECK(provider.isDeviant(7));
  BOOST_CHECK(!provider.isDeviant(1));
}

// Workers update values while the GUI thread queries them. Run under ThreadSanitizer
// this fails without internal locking; without it, it still exercises rehashing.
BOOST_AUTO_TEST_CASE(test_concurrent_updates_and_queries) {
  DeviationProvider<int> provider;
  std::atomic<bool> stop(false);

  std::vector<std::thread> writers;
  for (int t = 0; t < 4; ++t) {
    writers.emplace_back([&provider, t] {
      for (int i = 0; i < 20000; ++i) {
        provider.addOrUpdate(t * 100000 + (i % 5000), double(i % 17));
        if (i % 7 == 0) {
          provider.remove(t * 100000 + (i % 5000));
        }
      }
    });
  }
  std::thread reader([&provider, &stop] {
    while (!stop) {
      for (int key = 0; key < 400000; key += 997) {
        provider.isDeviant(key, 1.0, 1.0);
        provider.getDeviationValue(key);
      }
    }
  });

  for (std::thread& writer : writers) {
    writer.join();
  }
  stop = true;
  reader.join();

  // The map must still be coherent afterwards.
  provider.addOrUpdate(-1, 3.0);
  BOOST_CHECK_GE(provider.getDeviationValue(-1), 0.0);
}

BOOST_AUTO_TEST_SUITE_END()
