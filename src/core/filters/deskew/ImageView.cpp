// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageView.h"

#include <Constants.h>
#include <core/IconProvider.h>

#include <QAction>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QWheelEvent>
#include <boost/bind/bind.hpp>

#include "ImagePresentation.h"

namespace deskew {
const double ImageView::m_maxRotationDeg = 45.0;
const double ImageView::m_maxRotationSin = std::sin(m_maxRotationDeg * constants::DEG2RAD);
const double ImageView::m_maxObliqueDeg = 5.0;
const int ImageView::m_cellSize = 20;

ImageView::ImageView(const QImage& image, const QImage& downscaledImage, const ImageTransformation& xform)
    : ImageViewBase(image, downscaledImage, ImagePresentation(xform.transform(), xform.resultingPreCropArea())),
      m_handlePixmap(IconProvider::getInstance().getIcon("aqua-sphere").pixmap(16, 16)),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_xform(xform) {
  setMouseTracking(true);

  interactionState().setDefaultStatusTip(tr("Use Ctrl+Wheel to rotate or Ctrl+Shift+Wheel for finer rotation."));

  const QString rotationTip(tr("Drag this handle to rotate the image."));
  const QString obliqueTip(tr("Drag this handle to adjust oblique (shear)."));
  const double hitRadius = std::max<double>(0.5 * m_handlePixmap.width(), 15.0);
  for (int i = 0; i < 4; ++i) {
    m_handles[i].setHitRadius(hitRadius);
    m_handles[i].setPositionCallback(boost::bind(&ImageView::handlePosition, this, i));
    m_handles[i].setMoveRequestCallback(boost::bind(&ImageView::handleMoveRequest, this, i, boost::placeholders::_1));
    m_handles[i].setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));

    m_handleInteractors[i].setProximityStatusTip(i < 2 ? rotationTip : obliqueTip);
    m_handleInteractors[i].setObject(&m_handles[i]);

    makeLastFollower(m_handleInteractors[i]);
  }

  m_zoomHandler.setFocus(ZoomHandler::CENTER);

  rootInteractionHandler().makeLastFollower(*this);
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);

  auto* rotateLeft = new QAction(nullptr);
  rotateLeft->setShortcut(QKeySequence(","));
  connect(rotateLeft, SIGNAL(triggered(bool)), SLOT(doRotateLeft()));
  addAction(rotateLeft);

  auto* rotateRight = new QAction(nullptr);
  rotateRight->setShortcut(QKeySequence("."));
  connect(rotateRight, SIGNAL(triggered(bool)), SLOT(doRotateRight()));
  addAction(rotateRight);
}

ImageView::~ImageView() = default;

void ImageView::doRotate(double deg) {
  manualDeskewAngleSetExternally(m_xform.postRotation() + deg);
  emit manualDeskewAngleSet(m_xform.postRotation());
}

void ImageView::doRotateLeft() {
  doRotate(-0.10);
}

void ImageView::doRotateRight() {
  doRotate(0.10);
}

void ImageView::manualDeskewAngleSetExternally(const double degrees) {
  if (m_xform.postRotation() == degrees) {
    return;
  }

  m_xform.setPostRotation(degrees);
  updateTransform(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
}

void ImageView::manualObliqueAngleSetExternally(const double degrees) {
  if (m_xform.postOblique() == degrees) {
    return;
  }

  m_xform.setPostOblique(degrees);
  updateTransform(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
}

void ImageView::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHints(QPainter::Antialiasing, false);

  const QRectF contentArea(getContentAreaRect());
  const QPointF center(getImageRotationOrigin());

  // Draw the semi-transparent grid.
  QPen pen(QColor(0, 0, 0xd1, 90));
  pen.setCosmetic(true);
  pen.setWidth(1);
  painter.setPen(pen);
  QVector<QLineF> lines;
  for (double y = center.y(); (y -= m_cellSize) > contentArea.top();) {
    lines.push_back(QLineF(contentArea.left() + 0.5, y, contentArea.right() - 0.5, y));
  }
  for (double y = center.y(); (y += m_cellSize) < contentArea.bottom();) {
    lines.push_back(QLineF(contentArea.left() + 0.5, y, contentArea.right() - 0.5, y));
  }
  for (double x = center.x(); (x -= m_cellSize) > contentArea.left();) {
    lines.push_back(QLineF(x, contentArea.top() + 0.5, x, contentArea.bottom() - 0.5));
  }
  for (double x = center.x(); (x += m_cellSize) < contentArea.right();) {
    lines.push_back(QLineF(x, contentArea.top() + 0.5, x, contentArea.bottom() - 0.5));
  }
  painter.drawLines(lines);

  // Draw the horizontal and vertical line crossing at the center.
  pen.setColor(QColor(0, 0, 0xd1));
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);
  painter.drawLine(QPointF(contentArea.left() + 0.5, center.y()), QPointF(contentArea.right() - 0.5, center.y()));
  painter.drawLine(QPointF(center.x(), contentArea.top() + 0.5), QPointF(center.x(), contentArea.bottom() - 0.5));
  // Draw the rotation arcs.
  // Those will look like this (  )
  const QRectF arcSquare(getRotationArcSquare());

  painter.setRenderHints(QPainter::Antialiasing, true);
  pen.setWidthF(1.5);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);
  painter.drawArc(arcSquare, qRound(16 * -m_maxRotationDeg), qRound(16 * 2 * m_maxRotationDeg));
  painter.drawArc(arcSquare, qRound(16 * (180 - m_maxRotationDeg)), qRound(16 * 2 * m_maxRotationDeg));

  const std::pair<QPointF, QPointF> handles(getRotationHandles(arcSquare));

  QRectF rect(m_handlePixmap.rect());
  rect.moveCenter(handles.first);
  painter.drawPixmap(rect.topLeft(), m_handlePixmap);
  rect.moveCenter(handles.second);
  painter.drawPixmap(rect.topLeft(), m_handlePixmap);

  // Vertical arc and handles for oblique (shear).
  const QRectF obliqueArcSquare(getObliqueArcSquare());
  painter.drawArc(obliqueArcSquare, qRound(16 * (90 - m_maxObliqueDeg)), qRound(16 * 2 * m_maxObliqueDeg));
  painter.drawArc(obliqueArcSquare, qRound(16 * (270 - m_maxObliqueDeg)), qRound(16 * 2 * m_maxObliqueDeg));

  const std::pair<QPointF, QPointF> obliqueHandles(getObliqueHandles(obliqueArcSquare));
  rect.moveCenter(obliqueHandles.first);
  painter.drawPixmap(rect.topLeft(), m_handlePixmap);
  rect.moveCenter(obliqueHandles.second);
  painter.drawPixmap(rect.topLeft(), m_handlePixmap);
}  // ImageView::onPaint

void ImageView::onWheelEvent(QWheelEvent* event, InteractionState& interaction) {
  if (interaction.captured()) {
    return;
  }

  double degreeFraction = 0;

  if (event->modifiers() == Qt::ControlModifier) {
    degreeFraction = 0.1;
  } else if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
    degreeFraction = 0.05;
  } else {
    return;
  }

  event->accept();
  const double delta = degreeFraction * event->angleDelta().y() / 120;
  double angleDeg = m_xform.postRotation() - delta;
  angleDeg = qBound(-m_maxRotationDeg, angleDeg, m_maxRotationDeg);
  if (angleDeg == m_xform.postRotation()) {
    return;
  }

  m_xform.setPostRotation(angleDeg);
  updateTransformPreservingScale(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
  emit manualDeskewAngleSet(m_xform.postRotation());
}  // ImageView::onWheelEvent

QPointF ImageView::handlePosition(int idx) const {
  if (idx < 2) {
    const std::pair<QPointF, QPointF> handles(getRotationHandles(getRotationArcSquare()));
    return idx == 0 ? handles.first : handles.second;
  } else {
    const std::pair<QPointF, QPointF> handles(getObliqueHandles(getObliqueArcSquare()));
    return idx == 2 ? handles.first : handles.second;
  }
}

void ImageView::handleMoveRequest(int idx, const QPointF& pos) {
  if (idx < 2) {
    const QRectF arcSquare(getRotationArcSquare());
    const double arcRadius = 0.5 * arcSquare.width();
    const double absY = pos.y();
    double relY = absY - arcSquare.center().y();
    relY = qBound(-arcRadius, relY, arcRadius);

    double angleRad = std::asin(relY / arcRadius);
    if (idx == 0) {
      angleRad = -angleRad;
    }
    double angleDeg = angleRad * constants::RAD2DEG;
    angleDeg = qBound(-m_maxRotationDeg, angleDeg, m_maxRotationDeg);
    if (angleDeg == m_xform.postRotation()) {
      return;
    }

    m_xform.setPostRotation(angleDeg);
    updateTransformPreservingScale(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
  } else {
    const QRectF arcSquare(getObliqueArcSquare());
    const double arcRadius = 0.5 * arcSquare.height();
    const double absX = pos.x();
    double relX = absX - arcSquare.center().x();
    relX = qBound(-arcRadius, relX, arcRadius);

    double angleRad = std::asin(relX / arcRadius);
    if (idx == 3) {
      angleRad = -angleRad;  // bottom handle: right drag => negative oblique
    }
    double angleDeg = angleRad * constants::RAD2DEG;
    angleDeg = qBound(-m_maxObliqueDeg, angleDeg, m_maxObliqueDeg);
    if (angleDeg == m_xform.postOblique()) {
      return;
    }

    m_xform.setPostOblique(angleDeg);
    updateTransformPreservingScale(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
  }
}

void ImageView::dragFinished() {
  emit manualDeskewAngleSet(m_xform.postRotation());
  emit manualObliqueAngleSet(m_xform.postOblique());
}

/**
 * Get the point at the center of the widget content area, in widget coordinates.
 */
QPointF ImageView::getImageRotationOrigin() const {
  const QRectF occupied(getOccupiedWidgetRect());
  if (!occupied.isEmpty()) {
    return occupied.center();
  }

  const QRectF contentArea(getContentAreaRect());
  return contentArea.center();
}

QRectF ImageView::getContentAreaRect() const {
  const double hMargin
      = 0.5 * m_handlePixmap.width()
        + (verticalScrollBar()->isVisible()
               ? verticalScrollBar()->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, verticalScrollBar())
               : 0.0);
  const double vMargin
      = 0.5 * m_handlePixmap.height()
        + (horizontalScrollBar()->isVisible()
               ? horizontalScrollBar()->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, horizontalScrollBar())
               : 0.0);

  QRectF content(maxViewportRect());
  content.adjust(hMargin, vMargin, -hMargin, -vMargin);
  if (content.isEmpty()) {
    return maxViewportRect();
  }
  return content;
}

/**
 * Get the square in widget coordinates where two rotation arcs will be drawn.
 */
QRectF ImageView::getRotationArcSquare() const {
  const QRectF reducedScreenRect(getContentAreaRect());

  QSizeF arcSize(1.0, m_maxRotationSin);
  arcSize.scale(reducedScreenRect.size(), Qt::KeepAspectRatio);
  arcSize.setHeight(arcSize.width());

  QRectF arcSquare(QPointF(0, 0), arcSize);
  arcSquare.moveCenter(getImageRotationOrigin());
  return arcSquare;
}

std::pair<QPointF, QPointF> ImageView::getRotationHandles(const QRectF& arcSquare) const {
  const double rotSin = m_xform.postRotationSin();
  const double rotCos = m_xform.postRotationCos();
  const double arcRadius = 0.5 * arcSquare.width();
  const QPointF arcCenter(arcSquare.center());
  QPointF leftHandle(-rotCos * arcRadius, -rotSin * arcRadius);
  leftHandle += arcCenter;
  QPointF rightHandle(rotCos * arcRadius, rotSin * arcRadius);
  rightHandle += arcCenter;
  return std::make_pair(leftHandle, rightHandle);
}

QRectF ImageView::getObliqueArcSquare() const {
  const QRectF reducedScreenRect(getContentAreaRect());
  const QRectF occupied(getOccupiedWidgetRect());

  const double obliqueSin = std::sin(m_maxObliqueDeg * constants::DEG2RAD);
  QSizeF arcSize(obliqueSin, 1.0);
  arcSize.scale(reducedScreenRect.size(), Qt::KeepAspectRatio);
  arcSize.setWidth(arcSize.height());

  QRectF arcSquare(QPointF(0, 0), arcSize);
  const double rightEdge = occupied.isEmpty() ? reducedScreenRect.right() : occupied.right();
  arcSquare.moveRight(rightEdge);
  arcSquare.moveCenter(QPointF(arcSquare.center().x(), getImageRotationOrigin().y()));
  return arcSquare;
}

std::pair<QPointF, QPointF> ImageView::getObliqueHandles(const QRectF& arcSquare) const {
  const double obliqueRad = m_xform.postOblique() * constants::DEG2RAD;
  const double oblSin = std::sin(obliqueRad);
  const double oblCos = std::cos(obliqueRad);
  const double arcRadius = 0.5 * arcSquare.height();
  const QPointF arcCenter(arcSquare.center());
  QPointF topHandle(arcCenter.x() + oblSin * arcRadius, arcCenter.y() - oblCos * arcRadius);
  QPointF bottomHandle(arcCenter.x() - oblSin * arcRadius, arcCenter.y() + oblCos * arcRadius);
  return std::make_pair(topHandle, bottomHandle);
}
}  // namespace deskew
