// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_DEVIATIONPROVIDER_H_
#define SCANTAILOR_CORE_DEVIATIONPROVIDER_H_

#include <foundation/NonCopyable.h>

#include <cmath>
#include <functional>
#include <mutex>
#include <unordered_map>

/**
 * \brief Tracks one value per key and tells which keys deviate from the rest.
 *
 * Thread-safe: worker threads update values while the GUI thread queries them.
 * The compute callback is always invoked without the internal lock held, so it may
 * take the owner's own locks.
 */
template <typename K, typename Hash = std::hash<K>>
class DeviationProvider {
  DECLARE_NON_COPYABLE(DeviationProvider)
 public:
  DeviationProvider() = default;

  explicit DeviationProvider(const std::function<double(const K&)>& computeValueByKey);

  bool isDeviant(const K& key, double coefficient = 1.0, double threshold = 0.0, bool defaultVal = false) const;

  double getDeviationValue(const K& key) const;

  void addOrUpdate(const K& key);

  void addOrUpdate(const K& key, double value);

  void remove(const K& key);

  void clear();

  void setComputeValueByKey(const std::function<double(const K&)>& computeValueByKey);

 private:
  /** Recomputes the cached statistics if stale. Requires m_mutex. */
  void update() const;

  /** Stores a value, marking the statistics stale only if it changed. Requires m_mutex. */
  void store(const K& key, double value);

  mutable std::mutex m_mutex;
  std::function<double(const K&)> m_computeValueByKey;
  std::unordered_map<K, double, Hash> m_keyValueMap;

  // Cached values.
  mutable bool m_needUpdate = false;
  mutable double m_meanValue = 0.0;
  mutable double m_standardDeviation = 0.0;
};


template <typename K, typename Hash>
DeviationProvider<K, Hash>::DeviationProvider(const std::function<double(const K&)>& computeValueByKey)
    : m_computeValueByKey(computeValueByKey) {}

template <typename K, typename Hash>
bool DeviationProvider<K, Hash>::isDeviant(const K& key, double coefficient, double threshold, bool defaultVal) const {
  const std::lock_guard<std::mutex> lock(m_mutex);

  const auto it = m_keyValueMap.find(key);
  if (it == m_keyValueMap.end()) {
    return false;
  }
  if (m_keyValueMap.size() < 3) {
    return false;
  }

  const double value = it->second;
  if (std::isnan(value)) {
    return defaultVal;
  }

  update();
  return (std::abs(value - m_meanValue)
          > std::max((coefficient * m_standardDeviation), (threshold / 100) * m_meanValue));
}

template <typename K, typename Hash>
double DeviationProvider<K, Hash>::getDeviationValue(const K& key) const {
  const std::lock_guard<std::mutex> lock(m_mutex);

  const auto it = m_keyValueMap.find(key);
  if (it == m_keyValueMap.end()) {
    return -1.0;
  }
  if (m_keyValueMap.size() < 2) {
    return .0;
  }

  const double value = it->second;
  if (std::isnan(value)) {
    return -1.0;
  }

  update();
  return std::abs(value - m_meanValue);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::addOrUpdate(const K& key) {
  std::function<double(const K&)> computeValueByKey;
  {
    const std::lock_guard<std::mutex> lock(m_mutex);
    computeValueByKey = m_computeValueByKey;
  }
  const double value = computeValueByKey(key);

  const std::lock_guard<std::mutex> lock(m_mutex);
  store(key, value);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::addOrUpdate(const K& key, const double value) {
  const std::lock_guard<std::mutex> lock(m_mutex);
  store(key, value);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::store(const K& key, const double value) {
  const auto it = m_keyValueMap.find(key);
  if (it == m_keyValueMap.end()) {
    m_keyValueMap.emplace(key, value);
  } else if ((it->second == value) || (std::isnan(it->second) && std::isnan(value))) {
    // Nothing changed: keep the cached statistics. Thumbnail generation re-submits
    // the same value for every page, so this avoids a full recomputation per page.
    return;
  } else {
    it->second = value;
  }
  m_needUpdate = true;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::remove(const K& key) {
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (m_keyValueMap.erase(key) != 0) {
    m_needUpdate = true;
  }
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::update() const {
  if (!m_needUpdate) {
    return;
  }
  if (m_keyValueMap.size() < 2) {
    return;
  }

  int count = 0;
  {
    double sum = .0;
    for (const auto& [key, value] : m_keyValueMap) {
      if (!std::isnan(value)) {
        sum += value;
        count++;
      }
    }
    m_meanValue = sum / count;
  }

  {
    double differencesSum = .0;
    for (const auto& [key, value] : m_keyValueMap) {
      if (!std::isnan(value)) {
        differencesSum += std::pow(value - m_meanValue, 2);
      }
    }
    m_standardDeviation = std::sqrt(differencesSum / (count - 1));
  }

  m_needUpdate = false;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::setComputeValueByKey(const std::function<double(const K&)>& computeValueByKey) {
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_computeValueByKey = computeValueByKey;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::clear() {
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_keyValueMap.clear();

  m_needUpdate = false;
  m_meanValue = 0.0;
  m_standardDeviation = 0.0;
}


#endif  // SCANTAILOR_CORE_DEVIATIONPROVIDER_H_
