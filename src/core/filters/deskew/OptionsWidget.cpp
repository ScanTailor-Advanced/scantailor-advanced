// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptionsWidget.h"

#include <utility>

#include "ApplyDialog.h"
#include "Params.h"
#include "Settings.h"

namespace deskew {
namespace {

Params mergeParamsForApply(const std::unique_ptr<Params>& existing,
                           const OptionsWidget::UiData& cur,
                           const bool applyDeskew,
                           const bool applyOblique) {
  const Dependencies deps(cur.dependencies());
  if (!existing) {
    return Params(applyDeskew ? cur.effectiveDeskewAngle() : 0.0, applyOblique ? cur.effectiveObliqueAngle() : 0.0,
                  deps, applyDeskew ? cur.mode() : MODE_AUTO, applyOblique ? cur.obliqueMode() : MODE_AUTO);
  }
  return Params(applyDeskew ? cur.effectiveDeskewAngle() : existing->deskewAngle(),
                applyOblique ? cur.effectiveObliqueAngle() : existing->obliqueAngle(), deps,
                applyDeskew ? cur.mode() : existing->mode(),
                applyOblique ? cur.obliqueMode() : existing->obliqueMode());
}

}  // namespace

const double OptionsWidget::MAX_ANGLE = 45.0;

OptionsWidget::OptionsWidget(std::shared_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor)
    : m_settings(std::move(settings)),
      m_pageSelectionAccessor(pageSelectionAccessor),
      m_connectionManager(std::bind(&OptionsWidget::setupUiConnections, this)) {
  setupUi(this);
  angleSpinBox->setSuffix(QChar(0x00B0));  // the degree symbol
  angleSpinBox->setRange(-MAX_ANGLE, MAX_ANGLE);
  angleSpinBox->adjustSize();
  setSpinBoxUnknownState();
  topEdgeCheckBox->setChecked(!m_settings->algoContentBased());

  setupUiConnections();
}

OptionsWidget::~OptionsWidget() = default;

void OptionsWidget::showDeskewDialog() {
  auto* dialog = new ApplyDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(tr("Apply Deskew"));
  connect(dialog, &ApplyDialog::appliedTo, this, &OptionsWidget::appliedTo);
  connect(dialog, &ApplyDialog::appliedToAllPages, this, &OptionsWidget::appliedToAllPages);
  dialog->show();
}

void OptionsWidget::appliedTo(const std::set<PageId>& pages, const bool applyDeskew, const bool applyOblique) {
  if (pages.empty() || (!applyDeskew && !applyOblique)) {
    return;
  }

  for (const PageId& pageId : pages) {
    std::unique_ptr<Params> existing(m_settings->getPageParams(pageId));
    const Params merged(mergeParamsForApply(existing, m_uiData, applyDeskew, applyOblique));
    m_settings->setPageParams(pageId, merged);
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& pageId : pages) {
      emit invalidateThumbnail(pageId);
    }
  }
}

void OptionsWidget::appliedToAllPages(const std::set<PageId>& pages,
                                      const bool applyDeskew,
                                      const bool applyOblique) {
  if (pages.empty() || (!applyDeskew && !applyOblique)) {
    return;
  }

  for (const PageId& pageId : pages) {
    std::unique_ptr<Params> existing(m_settings->getPageParams(pageId));
    const Params merged(mergeParamsForApply(existing, m_uiData, applyDeskew, applyOblique));
    m_settings->setPageParams(pageId, merged);
  }
  emit invalidateAllThumbnails();
}

void OptionsWidget::manualDeskewAngleSetExternally(const double degrees) {
  m_uiData.setEffectiveDeskewAngle(degrees);
  m_uiData.setMode(MODE_MANUAL);
  updateModeIndication(MODE_MANUAL);
  setSpinBoxKnownState(degreesToSpinBox(degrees));
  commitCurrentParams();

  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::manualObliqueAngleSetExternally(const double degrees) {
  auto block = m_connectionManager.getScopedBlock();

  m_uiData.setEffectiveObliqueAngle(degrees);
  m_uiData.setObliqueMode(MODE_MANUAL);
  updateObliqueModeIndication(MODE_MANUAL);
  obliqueSpinBox->setValue(degrees);
  commitCurrentParams();

  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::preUpdateUI(const PageId& pageId) {
  auto block = m_connectionManager.getScopedBlock();

  m_pageId = pageId;
  setSpinBoxUnknownState();
  autoBtn->setChecked(true);
  autoBtn->setEnabled(false);
  manualBtn->setEnabled(false);
  obliqueAutoBtn->setEnabled(false);
  obliqueManualBtn->setEnabled(false);
}

void OptionsWidget::postUpdateUI(const UiData& uiData) {
  auto block = m_connectionManager.getScopedBlock();

  m_uiData = uiData;
  autoBtn->setEnabled(true);
  manualBtn->setEnabled(true);
  obliqueAutoBtn->setEnabled(true);
  obliqueManualBtn->setEnabled(true);
  updateModeIndication(uiData.mode());
  updateObliqueModeIndication(uiData.obliqueMode());
  setSpinBoxKnownState(degreesToSpinBox(uiData.effectiveDeskewAngle()));
  obliqueSpinBox->setValue(m_uiData.effectiveObliqueAngle());
}

void OptionsWidget::spinBoxValueChanged(const double value) {
  auto block = m_connectionManager.getScopedBlock();

  const double degrees = spinBoxToDegrees(value);
  m_uiData.setEffectiveDeskewAngle(degrees);
  m_uiData.setMode(MODE_MANUAL);
  updateModeIndication(MODE_MANUAL);
  commitCurrentParams();

  emit manualDeskewAngleSet(degrees);
  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::modeChanged(const bool autoMode) {
  if (autoMode) {
    m_uiData.setMode(MODE_AUTO);
    if (m_uiData.obliqueMode() == MODE_AUTO) {
      m_uiData.setEffectiveObliqueAngle(0.0);
    }
    m_settings->clearPageParams(m_pageId);
    emit reloadRequested();
  } else {
    m_uiData.setMode(MODE_MANUAL);
    commitCurrentParams();
  }
}

void OptionsWidget::obliqueModeChanged(const bool autoMode) {
  if (autoMode) {
    m_uiData.setObliqueMode(MODE_AUTO);
    m_uiData.setEffectiveObliqueAngle(0.0);
    obliqueSpinBox->setValue(0.0);
    commitCurrentParams();
    emit reloadRequested();
  } else {
    m_uiData.setObliqueMode(MODE_MANUAL);
    commitCurrentParams();
  }
}

void OptionsWidget::updateModeIndication(const AutoManualMode mode) {
  auto block = m_connectionManager.getScopedBlock();

  if (mode == MODE_AUTO) {
    autoBtn->setChecked(true);
  } else {
    manualBtn->setChecked(true);
  }
}

void OptionsWidget::updateObliqueModeIndication(const AutoManualMode mode) {
  auto block = m_connectionManager.getScopedBlock();

  if (mode == MODE_AUTO) {
    obliqueAutoBtn->setChecked(true);
  } else {
    obliqueManualBtn->setChecked(true);
  }
}

void OptionsWidget::setSpinBoxUnknownState() {
  auto block = m_connectionManager.getScopedBlock();

  angleSpinBox->setSpecialValueText("?");
  angleSpinBox->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  angleSpinBox->setValue(angleSpinBox->minimum());
  angleSpinBox->setEnabled(false);
  obliqueSpinBox->setValue(0.0);
  obliqueSpinBox->setEnabled(false);
  obliqueAutoBtn->setEnabled(false);
  obliqueManualBtn->setEnabled(false);
}

void OptionsWidget::setSpinBoxKnownState(const double angle) {
  auto block = m_connectionManager.getScopedBlock();

  angleSpinBox->setSpecialValueText("");
  angleSpinBox->setValue(angle);

  // Right alignment doesn't work correctly, so we use the left one.
  angleSpinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  angleSpinBox->setEnabled(true);
  obliqueSpinBox->setEnabled(true);
  obliqueAutoBtn->setEnabled(true);
  obliqueManualBtn->setEnabled(true);
}

void OptionsWidget::commitCurrentParams() {
  Params params(m_uiData.effectiveDeskewAngle(), m_uiData.effectiveObliqueAngle(), m_uiData.dependencies(),
                m_uiData.mode(), m_uiData.obliqueMode());
  m_settings->setPageParams(m_pageId, params);
}

double OptionsWidget::spinBoxToDegrees(const double sbValue) {
  // The spin box shows the angle in a usual geometric way,
  // with positive angles going counter-clockwise.
  // Internally, we operate with angles going clockwise,
  // because the Y axis points downwards in computer graphics.
  return -sbValue;
}

double OptionsWidget::degreesToSpinBox(const double degrees) {
  // See above.
  return -degrees;
}

#define CONNECT(...) m_connectionManager.addConnection(connect(__VA_ARGS__))

void OptionsWidget::topEdgeToggled(bool checked) {
  m_settings->setAlgoContentBased(!checked);
  if (autoBtn->isChecked()) {
    emit reloadRequested();
  }
}

void OptionsWidget::obliqueSpinBoxValueChanged(double value) {
  auto block = m_connectionManager.getScopedBlock();

  m_uiData.setEffectiveObliqueAngle(value);
  if (value != 0.0) {
    m_uiData.setObliqueMode(MODE_MANUAL);
    updateObliqueModeIndication(MODE_MANUAL);
  }
  commitCurrentParams();
  emit manualObliqueAngleSet(value);
  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::setupUiConnections() {
  CONNECT(angleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxValueChanged(double)));
  CONNECT(obliqueSpinBox, SIGNAL(valueChanged(double)), this, SLOT(obliqueSpinBoxValueChanged(double)));
  CONNECT(autoBtn, SIGNAL(toggled(bool)), this, SLOT(modeChanged(bool)));
  CONNECT(obliqueAutoBtn, SIGNAL(toggled(bool)), this, SLOT(obliqueModeChanged(bool)));
  CONNECT(topEdgeCheckBox, SIGNAL(toggled(bool)), this, SLOT(topEdgeToggled(bool)));
  CONNECT(applyDeskewBtn, SIGNAL(clicked()), this, SLOT(showDeskewDialog()));
}

#undef CONNECT

/*========================== OptionsWidget::UiData =========================*/

OptionsWidget::UiData::UiData()
    : m_effDeskewAngle(0.0), m_effObliqueAngle(0.0), m_mode(MODE_AUTO), m_obliqueMode(MODE_AUTO) {}

OptionsWidget::UiData::~UiData() = default;
}  // namespace deskew