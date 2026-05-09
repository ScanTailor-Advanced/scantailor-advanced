// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FIX_ORIENTATION_OPTIONSWIDGET_H_
#define SCANTAILOR_FIX_ORIENTATION_OPTIONSWIDGET_H_

#include <core/ConnectionManager.h>

#include <memory>

#include "FilterOptionsWidget.h"
#include "ImageTrim.h"
#include "OrthogonalRotation.h"
#include "PageId.h"
#include "PageInfo.h"
#include "PageSelectionAccessor.h"
#include "ui_OptionsWidget.h"

namespace fix_orientation {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::OptionsWidget {
  Q_OBJECT
 public:
  OptionsWidget(std::shared_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageInfo& pageInfo, OrthogonalRotation rotation);

  void postUpdateUI(OrthogonalRotation rotation);

 signals:

  void rotated(OrthogonalRotation rotation);

 private slots:

  void rotateLeft();

  void rotateRight();

  void resetRotation();

  void showApplyToDialog();

  void appliedTo(const std::set<PageId>& pages);

  void appliedToAllPages(const std::set<PageId>& pages);

  void trimEnableToggled(bool checked);

  void trimMarginsChanged(int value);

  void resetTrim();

 private:
  void setRotation(const OrthogonalRotation& rotation);

  void setRotationPixmap();

  void setupUiConnections();

  void setupIcons();

  void updateTrimMaximums();

  void pullTrimToControls();

  void pushTrimFromControls();

  ImageTrim currentTrimFromControls() const;

  std::shared_ptr<Settings> m_settings;
  PageSelectionAccessor m_pageSelectionAccessor;
  PageId m_pageId;
  QSize m_imagePixelSize;
  OrthogonalRotation m_rotation;

  ConnectionManager m_connectionManager;
};
}  // namespace fix_orientation
#endif  // ifndef SCANTAILOR_FIX_ORIENTATION_OPTIONSWIDGET_H_
