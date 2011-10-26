/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKSPRITEIMAGE_P_H
#define QQUICKSPRITEIMAGE_P_H

#include <QQuickItem>
#include <QTime>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGContext;
class QQuickSprite;
class QQuickSpriteEngine;
class QSGGeometryNode;
class QQuickSpriteMaterial;
class QQuickSpriteImage : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool interpolate READ interpolate WRITE setInterpolate NOTIFY interpolateChanged)
    Q_PROPERTY(QString goalState READ goalState WRITE setGoalState NOTIFY goalStateChanged)
    //###try to share similar spriteEngines for less overhead?
    Q_PROPERTY(QDeclarativeListProperty<QQuickSprite> sprites READ sprites)
    Q_CLASSINFO("DefaultProperty", "sprites")

public:
    explicit QQuickSpriteImage(QQuickItem *parent = 0);

    QDeclarativeListProperty<QQuickSprite> sprites();

    bool running() const
    {
        return m_running;
    }

    bool interpolate() const
    {
        return m_interpolate;
    }

    QString goalState() const
    {
        return m_goalState;
    }

signals:

    void runningChanged(bool arg);
    void interpolateChanged(bool arg);
    void goalStateChanged(QString arg);

public slots:

    void jumpTo(const QString &sprite);
    void setGoalState(const QString &sprite);

    void setRunning(bool arg)
    {
        if (m_running != arg) {
            m_running = arg;
            emit runningChanged(arg);
        }
    }

    void setInterpolate(bool arg)
    {
        if (m_interpolate != arg) {
            m_interpolate = arg;
            emit interpolateChanged(arg);
        }
    }

private slots:
    void createEngine();
protected:
    void reset();
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
private:
    void prepareNextFrame();
    QSGGeometryNode* buildNode();
    QSGGeometryNode *m_node;
    QQuickSpriteMaterial *m_material;
    QList<QQuickSprite*> m_sprites;
    QQuickSpriteEngine* m_spriteEngine;
    QTime m_timestamp;
    int m_maxFrames;
    bool m_pleaseReset;
    bool m_running;
    bool m_interpolate;
    QString m_goalState;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQUICKSPRITEIMAGE_P_H
