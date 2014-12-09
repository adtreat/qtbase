/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QLIBINPUTTOUCH_P_H
#define QLIBINPUTTOUCH_P_H

#include <QtCore/QHash>
#include <QtCore/QList>
#include <qpa/qwindowsysteminterface.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

struct libinput_event_touch;
struct libinput_device;

QT_BEGIN_NAMESPACE

class QLibInputTouch
{
public:
    void registerDevice(libinput_device *dev);
    void unregisterDevice(libinput_device *dev);
    void processTouchDown(libinput_event_touch *e);
    void processTouchMotion(libinput_event_touch *e);
    void processTouchUp(libinput_event_touch *e);
    void processTouchCancel(libinput_event_touch *e);
    void processTouchFrame(libinput_event_touch *e);

private:
    struct DeviceState {
        DeviceState() : m_touchDevice(0) { }
        QWindowSystemInterface::TouchPoint *point(int32_t slot);
        QList<QWindowSystemInterface::TouchPoint> m_points;
        QTouchDevice *m_touchDevice;
    };

    DeviceState *deviceState(libinput_event_touch *e);

    QHash<libinput_device *, DeviceState> m_devState;
};

QT_END_NAMESPACE

#endif
