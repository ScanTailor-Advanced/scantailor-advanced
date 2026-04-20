// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Settings.h"

#include "../../Utils.h"
#include "AbstractRelinker.h"
#include "RelinkablePath.h"

using namespace core;

namespace fix_orientation {
Settings::Settings() = default;

Settings::~Settings() = default;

void Settings::clear() {
  QMutexLocker locker(&m_mutex);
  m_perImageRotation.clear();
  m_perImageTrim.clear();
}

void Settings::performRelinking(const AbstractRelinker& relinker) {
  QMutexLocker locker(&m_mutex);
  PerImageRotation newRotations;

  for (const PerImageRotation::value_type& kv : m_perImageRotation) {
    const RelinkablePath oldPath(kv.first.filePath(), RelinkablePath::File);
    ImageId newImageId(kv.first);
    newImageId.setFilePath(relinker.substitutionPathFor(oldPath));
    newRotations.insert(PerImageRotation::value_type(newImageId, kv.second));
  }

  m_perImageRotation.swap(newRotations);

  PerImageTrim newTrims;
  for (const PerImageTrim::value_type& kv : m_perImageTrim) {
    const RelinkablePath oldPath(kv.first.filePath(), RelinkablePath::File);
    ImageId newImageId(kv.first);
    newImageId.setFilePath(relinker.substitutionPathFor(oldPath));
    newTrims.insert(PerImageTrim::value_type(newImageId, kv.second));
  }
  m_perImageTrim.swap(newTrims);
}

void Settings::applyRotation(const ImageId& imageId, const OrthogonalRotation rotation) {
  QMutexLocker locker(&m_mutex);
  setImageRotationLocked(imageId, rotation);
}

void Settings::applyRotation(const std::set<PageId>& pages, const OrthogonalRotation rotation) {
  QMutexLocker locker(&m_mutex);

  for (const PageId& page : pages) {
    setImageRotationLocked(page.imageId(), rotation);
  }
}

OrthogonalRotation Settings::getRotationFor(const ImageId& imageId) const {
  QMutexLocker locker(&m_mutex);

  auto it(m_perImageRotation.find(imageId));
  if (it != m_perImageRotation.end()) {
    return it->second;
  } else {
    return OrthogonalRotation();
  }
}

void Settings::setImageRotationLocked(const ImageId& imageId, const OrthogonalRotation& rotation) {
  Utils::mapSetValue(m_perImageRotation, imageId, rotation);
}

bool Settings::isRotationNull(const ImageId& imageId) const {
  QMutexLocker locker(&m_mutex);
  return (m_perImageRotation.find(imageId) == m_perImageRotation.end());
}

void Settings::setTrim(const ImageId& imageId, const ImageTrim& trim) {
  QMutexLocker locker(&m_mutex);
  if (!trim.enabled) {
    m_perImageTrim.erase(imageId);
    return;
  }
  Utils::mapSetValue(m_perImageTrim, imageId, trim);
}

ImageTrim Settings::getTrim(const ImageId& imageId) const {
  QMutexLocker locker(&m_mutex);
  const auto it(m_perImageTrim.find(imageId));
  if (it != m_perImageTrim.end()) {
    return it->second;
  }
  return ImageTrim();
}

void Settings::clearTrim(const ImageId& imageId) {
  QMutexLocker locker(&m_mutex);
  m_perImageTrim.erase(imageId);
}
}  // namespace fix_orientation