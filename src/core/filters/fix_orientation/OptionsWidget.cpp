// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptionsWidget.h"

#include <core/IconProvider.h>

#include <QSize>

#include <algorithm>
#include <cassert>
#include <utility>

#include "ApplyDialog.h"
#include "Filter.h"
#include "ProjectPages.h"
#include "Settings.h"

namespace fix_orientation {
namespace {

const int kMinInnerSide = 32;

}  // namespace

OptionsWidget::OptionsWidget(std::shared_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor)
    : m_settings(std::move(settings)),
      m_pageSelectionAccessor(pageSelectionAccessor),
      m_connectionManager(std::bind(&OptionsWidget::setupUiConnections, this)) {
  setupUi(this);
  setupIcons();

  setupUiConnections();
}

OptionsWidget::~OptionsWidget() = default;

void OptionsWidget::preUpdateUI(const PageInfo& pageInfo, const OrthogonalRotation rotation) {
  auto block = m_connectionManager.getScopedBlock();

  m_pageId = pageInfo.id();
  m_imagePixelSize = pageInfo.metadata().size();
  m_rotation = rotation;
  setRotationPixmap();
  updateTrimMaximums();
  pullTrimToControls();
}

void OptionsWidget::postUpdateUI(const OrthogonalRotation rotation) {
  auto block = m_connectionManager.getScopedBlock();

  setRotation(rotation);
}

void OptionsWidget::rotateLeft() {
  OrthogonalRotation rotation(m_rotation);
  rotation.prevClockwiseDirection();
  setRotation(rotation);
}

void OptionsWidget::rotateRight() {
  OrthogonalRotation rotation(m_rotation);
  rotation.nextClockwiseDirection();
  setRotation(rotation);
}

void OptionsWidget::resetRotation() {
  setRotation(OrthogonalRotation());
}

void OptionsWidget::showApplyToDialog() {
  auto* dialog = new ApplyDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(dialog, SIGNAL(appliedTo(const std::set<PageId>&)), this, SLOT(appliedTo(const std::set<PageId>&)));
  connect(dialog, SIGNAL(appliedToAllPages(const std::set<PageId>&)), this,
          SLOT(appliedToAllPages(const std::set<PageId>&)));
  dialog->show();
}

void OptionsWidget::appliedTo(const std::set<PageId>& pages) {
  if (pages.empty()) {
    return;
  }

  m_settings->applyRotation(pages, m_rotation);

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& pageId : pages) {
      emit invalidateThumbnail(pageId);
    }
  }
}

void OptionsWidget::appliedToAllPages(const std::set<PageId>& pages) {
  m_settings->applyRotation(pages, m_rotation);
  emit invalidateAllThumbnails();
}

void OptionsWidget::setRotation(const OrthogonalRotation& rotation) {
  if (rotation == m_rotation) {
    return;
  }

  m_rotation = rotation;
  setRotationPixmap();

  m_settings->applyRotation(m_pageId.imageId(), rotation);

  emit rotated(rotation);
  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::trimEnableToggled(const bool checked) {
  trimLeftSpin->setEnabled(checked);
  trimRightSpin->setEnabled(checked);
  trimTopSpin->setEnabled(checked);
  trimBottomSpin->setEnabled(checked);
  resetTrimBtn->setEnabled(checked);
  if (!checked) {
    m_settings->clearTrim(m_pageId.imageId());
    emit invalidateThumbnail(m_pageId);
    return;
  }
  pushTrimFromControls();
}

void OptionsWidget::trimMarginsChanged(const int) {
  if (!trimEnabledCheck->isChecked()) {
    return;
  }
  pushTrimFromControls();
}

void OptionsWidget::resetTrim() {
  auto block = m_connectionManager.getScopedBlock();
  trimLeftSpin->setValue(0);
  trimRightSpin->setValue(0);
  trimTopSpin->setValue(0);
  trimBottomSpin->setValue(0);
  if (trimEnabledCheck->isChecked()) {
    pushTrimFromControls();
  } else {
    m_settings->clearTrim(m_pageId.imageId());
    emit invalidateThumbnail(m_pageId);
  }
}

void OptionsWidget::updateTrimMaximums() {
  const int w = m_imagePixelSize.width();
  const int h = m_imagePixelSize.height();
  const int maxSide = std::max(0, std::max(w, h) - kMinInnerSide);
  trimLeftSpin->setMaximum(maxSide);
  trimRightSpin->setMaximum(maxSide);
  trimTopSpin->setMaximum(maxSide);
  trimBottomSpin->setMaximum(maxSide);
}

void OptionsWidget::pullTrimToControls() {
  const ImageTrim trim(m_settings->getTrim(m_pageId.imageId()));
  trimEnabledCheck->setChecked(trim.enabled);
  trimLeftSpin->setValue(trim.left);
  trimRightSpin->setValue(trim.right);
  trimTopSpin->setValue(trim.top);
  trimBottomSpin->setValue(trim.bottom);
  trimLeftSpin->setEnabled(trim.enabled);
  trimRightSpin->setEnabled(trim.enabled);
  trimTopSpin->setEnabled(trim.enabled);
  trimBottomSpin->setEnabled(trim.enabled);
  resetTrimBtn->setEnabled(trim.enabled);
}

void OptionsWidget::pushTrimFromControls() {
  ImageTrim trim;
  trim.enabled = true;
  trim.left = trimLeftSpin->value();
  trim.right = trimRightSpin->value();
  trim.top = trimTopSpin->value();
  trim.bottom = trimBottomSpin->value();
  m_settings->setTrim(m_pageId.imageId(), trim);
  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::setRotationPixmap() {
  QIcon icon;
  switch (m_rotation.toDegrees()) {
    case 0:
      icon = IconProvider::getInstance().getIcon("big-up-arrow");
      break;
    case 90:
      icon = IconProvider::getInstance().getIcon("big-right-arrow");
      break;
    case 180:
      icon = IconProvider::getInstance().getIcon("big-down-arrow");
      break;
    case 270:
      icon = IconProvider::getInstance().getIcon("big-left-arrow");
      break;
    default:
      assert(!"Unreachable");
  }
  rotationIndicator->setPixmap(icon.pixmap(32, 32));
}

#define CONNECT(...) m_connectionManager.addConnection(connect(__VA_ARGS__))

void OptionsWidget::setupUiConnections() {
  CONNECT(rotateLeftBtn, SIGNAL(clicked()), this, SLOT(rotateLeft()));
  CONNECT(rotateRightBtn, SIGNAL(clicked()), this, SLOT(rotateRight()));
  CONNECT(resetBtn, SIGNAL(clicked()), this, SLOT(resetRotation()));
  CONNECT(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
  CONNECT(trimEnabledCheck, SIGNAL(toggled(bool)), this, SLOT(trimEnableToggled(bool)));
  CONNECT(trimLeftSpin, SIGNAL(valueChanged(int)), this, SLOT(trimMarginsChanged(int)));
  CONNECT(trimRightSpin, SIGNAL(valueChanged(int)), this, SLOT(trimMarginsChanged(int)));
  CONNECT(trimTopSpin, SIGNAL(valueChanged(int)), this, SLOT(trimMarginsChanged(int)));
  CONNECT(trimBottomSpin, SIGNAL(valueChanged(int)), this, SLOT(trimMarginsChanged(int)));
  CONNECT(resetTrimBtn, SIGNAL(clicked()), this, SLOT(resetTrim()));
}

#undef CONNECT

void OptionsWidget::setupIcons() {
  auto& iconProvider = IconProvider::getInstance();
  rotateLeftBtn->setIcon(iconProvider.getIcon("object-rotate-left"));
  rotateRightBtn->setIcon(iconProvider.getIcon("object-rotate-right"));
}
}  // namespace fix_orientation
