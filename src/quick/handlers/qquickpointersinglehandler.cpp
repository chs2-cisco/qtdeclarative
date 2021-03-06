/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickpointersinglehandler_p.h"

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(DBG_TOUCH_TARGET)

/*!
    An intermediate class (not registered as a QML type)
    for the most common handlers: those which expect only a single point.
    wantsPointerEvent() will choose the first point which is inside the
    \l target item, and return true as long as the event contains that point.
    Override handleEventPoint() to implement a single-point handler.
*/

QQuickPointerSingleHandler::QQuickPointerSingleHandler(QObject *parent)
  : QQuickPointerDeviceHandler(parent)
  , m_acceptedButtons(Qt::LeftButton)
  , m_ignoreAdditionalPoints(false)
{
}

bool QQuickPointerSingleHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;
    if (event->device()->pointerType() != QQuickPointerDevice::Finger &&
            (event->buttons() & m_acceptedButtons) == 0 && (event->button() & m_acceptedButtons) == 0)
        return false;

    if (m_pointInfo.m_id) {
        // We already know which one we want, so check whether it's there.
        // It's expected to be an update or a release.
        // If we no longer want it, cancel the grab.
        int candidatePointCount = 0;
        QQuickEventPoint *point = nullptr;
        int c = event->pointCount();
        for (int i = 0; i < c; ++i) {
            QQuickEventPoint *p = event->point(i);
            if (wantsEventPoint(p)) {
                ++candidatePointCount;
                if (p->pointId() == m_pointInfo.m_id)
                    point = p;
            }
        }
        if (point) {
            if (candidatePointCount == 1 || (candidatePointCount > 1 && m_ignoreAdditionalPoints)) {
                point->setAccepted();
                return true;
            } else {
                point->cancelAllGrabs(this);
            }
        } else {
            qCWarning(DBG_TOUCH_TARGET) << this << "pointId" << hex << m_pointInfo.m_id
                << "is missing from current event, but was neither canceled nor released";
            return false;
        }
    } else {
        // We have not yet chosen a point; choose the first one for which wantsEventPoint() returns true.
        int candidatePointCount = 0;
        int c = event->pointCount();
        QQuickEventPoint *chosen = nullptr;
        for (int i = 0; i < c; ++i) {
            QQuickEventPoint *p = event->point(i);
            if (!p->exclusiveGrabber() && wantsEventPoint(p)) {
                if (!chosen)
                    chosen = p;
                ++candidatePointCount;
            }
        }
        if (chosen && candidatePointCount == 1) {
            m_pointInfo.m_id = chosen->pointId();
            chosen->setAccepted();
        }
    }
    return m_pointInfo.m_id;
}

void QQuickPointerSingleHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    QQuickPointerDeviceHandler::handlePointerEventImpl(event);
    QQuickEventPoint *currentPoint = event->pointById(m_pointInfo.m_id);
    Q_ASSERT(currentPoint);
    if (!m_pointInfo.m_id || !currentPoint->isAccepted()) {
        reset();
    } else {
        if (event->asPointerTouchEvent()) {
            QQuickEventTouchPoint *tp = static_cast<QQuickEventTouchPoint *>(currentPoint);
            m_pointInfo.m_uniqueId = tp->uniqueId();
            m_pointInfo.m_rotation = tp->rotation();
            m_pointInfo.m_pressure = tp->pressure();
            m_pointInfo.m_ellipseDiameters = tp->ellipseDiameters();
        } else if (event->asPointerTabletEvent()) {
            // TODO
        } else {
            m_pointInfo.m_uniqueId = event->device()->uniqueId();
            m_pointInfo.m_rotation = 0;
            m_pointInfo.m_pressure = event->buttons() ? 1 : 0;
            m_pointInfo.m_ellipseDiameters = QSizeF();
        }
        m_pointInfo.m_position = currentPoint->pos();
        m_pointInfo.m_scenePosition = currentPoint->scenePos();
        if (currentPoint->state() == QQuickEventPoint::Updated)
            m_pointInfo.m_velocity = currentPoint->velocity();
        handleEventPoint(currentPoint);
        switch (currentPoint->state()) {
        case QQuickEventPoint::Pressed:
            m_pointInfo.m_pressPosition = currentPoint->pos();
            m_pointInfo.m_scenePressPosition = currentPoint->scenePos();
            m_pointInfo.m_pressedButtons = event->buttons();
            break;
        case QQuickEventPoint::Released:
            setExclusiveGrab(currentPoint, false);
            reset();
            break;
        default:
            m_pointInfo.m_pressedButtons = event->buttons();
            break;
        }
        emit pointChanged();
    }
}

bool QQuickPointerSingleHandler::wantsEventPoint(QQuickEventPoint *point)
{
    return parentContains(point);
}

void QQuickPointerSingleHandler::onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabState stateChange, QQuickEventPoint *point)
{
    if (grabber != this)
        return;
    switch (stateChange) {
    case QQuickEventPoint::GrabExclusive:
        m_pointInfo.m_sceneGrabPosition = point->sceneGrabPos();
        setActive(true);
        QQuickPointerHandler::onGrabChanged(grabber, stateChange, point);
        break;
    case QQuickEventPoint::GrabPassive:
        m_pointInfo.m_sceneGrabPosition = point->sceneGrabPos();
        QQuickPointerHandler::onGrabChanged(grabber, stateChange, point);
        break;
    case QQuickEventPoint::OverrideGrabPassive:
        return; // don't emit
    case QQuickEventPoint::UngrabPassive:
    case QQuickEventPoint::UngrabExclusive:
    case QQuickEventPoint::CancelGrabPassive:
    case QQuickEventPoint::CancelGrabExclusive:
        // the grab is lost or relinquished, so the point is no longer relevant
        QQuickPointerHandler::onGrabChanged(grabber, stateChange, point);
        reset();
        break;
    }
    emit singlePointGrabChanged();
}

void QQuickPointerSingleHandler::setIgnoreAdditionalPoints(bool v)
{
    m_ignoreAdditionalPoints = v;
}

void QQuickPointerSingleHandler::moveTarget(QPointF pos, QQuickEventPoint *point)
{
    target()->setPosition(pos);
    m_pointInfo.m_scenePosition = point->scenePos();
    m_pointInfo.m_position = target()->mapFromScene(m_pointInfo.m_scenePosition);
}

void QQuickPointerSingleHandler::setAcceptedButtons(Qt::MouseButtons buttons)
{
    if (m_acceptedButtons == buttons)
        return;

    m_acceptedButtons = buttons;
    emit acceptedButtonsChanged();
}

void QQuickPointerSingleHandler::reset()
{
    setActive(false);
    m_pointInfo.reset();
}

QQuickHandlerPoint::QQuickHandlerPoint()
    : m_id(0)
    , m_rotation(0)
    , m_pressure(0)
{}

void QQuickHandlerPoint::reset()
{
    m_id = 0;
    m_uniqueId = QPointingDeviceUniqueId();
    m_position = QPointF();
    m_scenePosition = QPointF();
    m_pressPosition = QPointF();
    m_scenePressPosition = QPointF();
    m_sceneGrabPosition = QPointF();
    m_velocity = QVector2D();
    m_rotation = 0;
    m_pressure = 0;
    m_ellipseDiameters = QSizeF();
    m_pressedButtons = Qt::NoButton;
}

QT_END_NAMESPACE
