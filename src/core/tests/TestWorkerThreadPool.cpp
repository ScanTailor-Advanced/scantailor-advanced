// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BackgroundTask.h>
#include <FilterResult.h>
#include <WorkerThreadPool.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <stdexcept>
#include <vector>

namespace tests {
namespace {

class DummyResult : public FilterResult {
 public:
  void updateUI(FilterUiInterface*) override {}

  std::shared_ptr<AbstractFilter> filter() override { return nullptr; }
};

enum class Outcome { RESULT, STD_EXCEPTION, OTHER_EXCEPTION };

class Task : public BackgroundTask {
 public:
  explicit Task(const Outcome outcome) : BackgroundTask(BATCH), m_outcome(outcome) {}

  FilterResultPtr operator()() override {
    switch (m_outcome) {
      case Outcome::STD_EXCEPTION:
        throw std::runtime_error("test failure");
      case Outcome::OTHER_EXCEPTION:
        throw 42;
      case Outcome::RESULT:
        break;
    }
    return std::make_shared<DummyResult>();
  }

 private:
  const Outcome m_outcome;
};

struct Reported {
  BackgroundTaskPtr task;
  bool hasResult;
};

/** Submits the tasks and collects what the pool reports, waiting up to 10 seconds. */
std::vector<Reported> runTasks(const std::vector<BackgroundTaskPtr>& tasks) {
  int argc = 1;
  char appName[] = "core_tests";
  char* argv[] = {appName, nullptr};
  QCoreApplication app(argc, argv);

  std::vector<Reported> reported;
  WorkerThreadPool pool;
  QObject::connect(&pool, &WorkerThreadPool::taskResult,
                   [&reported](const BackgroundTaskPtr& task, const FilterResultPtr& result) {
                     reported.push_back({task, result != nullptr});
                   });

  for (const BackgroundTaskPtr& task : tasks) {
    pool.submitTask(task);
  }

  QElapsedTimer timer;
  timer.start();
  while (reported.size() < tasks.size() && timer.elapsed() < 10000) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
  }
  pool.shutdown();
  QCoreApplication::processEvents();
  return reported;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(CoreWorkerThreadPoolTestSuite)

/** A task that throws must not terminate the application, and must still be reported. */
BOOST_AUTO_TEST_CASE(test_failed_tasks_are_reported_with_null_result) {
  const std::vector<BackgroundTaskPtr> tasks{std::make_shared<Task>(Outcome::RESULT),
                                             std::make_shared<Task>(Outcome::STD_EXCEPTION),
                                             std::make_shared<Task>(Outcome::OTHER_EXCEPTION)};

  const std::vector<Reported> reported = runTasks(tasks);

  BOOST_REQUIRE_EQUAL(reported.size(), tasks.size());
  for (size_t i = 0; i < tasks.size(); ++i) {
    bool found = false;
    for (const Reported& r : reported) {
      if (r.task == tasks[i]) {
        found = true;
        BOOST_CHECK_EQUAL(r.hasResult, i == 0);
      }
    }
    BOOST_CHECK(found);
  }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
