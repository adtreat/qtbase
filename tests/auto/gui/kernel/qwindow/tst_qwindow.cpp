/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qwindow.h>

#include <QtTest/QtTest>

#include <QEvent>
#include <QStyleHints>

// For QSignalSpy slot connections.
Q_DECLARE_METATYPE(Qt::ScreenOrientation)

class tst_QWindow: public QObject
{
    Q_OBJECT

private slots:
    void mapGlobal();
    void positioning();
    void isActive();
    void testInputEvents();
    void touchToMouseTranslation();
    void mouseToTouchTranslation();
    void mouseToTouchLoop();
    void touchCancel();
    void touchCancelWithTouchToMouse();
    void orientation();
    void close();
    void activateAndClose();
    void mouseEventSequence();
    void windowModality();

    void initTestCase()
    {
        touchDevice = new QTouchDevice;
        touchDevice->setType(QTouchDevice::TouchScreen);
        QWindowSystemInterface::registerTouchDevice(touchDevice);
    }

private:
    QTouchDevice *touchDevice;
};


void tst_QWindow::mapGlobal()
{
    QWindow a;
    QWindow b(&a);
    QWindow c(&b);

    a.setGeometry(10, 10, 300, 300);
    b.setGeometry(20, 20, 200, 200);
    c.setGeometry(40, 40, 100, 100);

    QCOMPARE(a.mapToGlobal(QPoint(100, 100)), QPoint(110, 110));
    QCOMPARE(b.mapToGlobal(QPoint(100, 100)), QPoint(130, 130));
    QCOMPARE(c.mapToGlobal(QPoint(100, 100)), QPoint(170, 170));

    QCOMPARE(a.mapFromGlobal(QPoint(100, 100)), QPoint(90, 90));
    QCOMPARE(b.mapFromGlobal(QPoint(100, 100)), QPoint(70, 70));
    QCOMPARE(c.mapFromGlobal(QPoint(100, 100)), QPoint(30, 30));
}

class Window : public QWindow
{
public:
    Window()
    {
        reset();
        setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    }

    void reset()
    {
        m_received.clear();
    }

    bool event(QEvent *event)
    {
        m_received[event->type()]++;

        return QWindow::event(event);
    }

    int received(QEvent::Type type)
    {
        return m_received.value(type, 0);
    }

private:
    QHash<QEvent::Type, int> m_received;
};

void tst_QWindow::positioning()
{
    QRect geometry(80, 80, 40, 40);

    Window window;
    window.setGeometry(geometry);
    QCOMPARE(window.geometry(), geometry);
    window.show();

#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "This test fails on Mac OS X, see QTBUG-23059", Abort);
#endif
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_COMPARE(window.received(QEvent::Map), 1);

    QMargins originalMargins = window.frameMargins();

    QCOMPARE(window.pos(), window.framePos() + QPoint(originalMargins.left(), originalMargins.top()));
    QVERIFY(window.frameGeometry().contains(window.geometry()));

    QPoint originalPos = window.pos();
    QPoint originalFramePos = window.framePos();

    window.setWindowState(Qt::WindowFullScreen);
    QTRY_COMPARE(window.received(QEvent::Resize), 2);

    window.setWindowState(Qt::WindowNoState);
    QTRY_COMPARE(window.received(QEvent::Resize), 3);

    QTRY_COMPARE(originalPos, window.pos());
    QTRY_COMPARE(originalFramePos, window.framePos());
    QTRY_COMPARE(originalMargins, window.frameMargins());

    // if our positioning is actually fully respected by the window manager
    // test whether it correctly handles frame positioning as well
    if (originalPos == geometry.topLeft() && (originalMargins.top() != 0 || originalMargins.left() != 0)) {
        QPoint framePos(40, 40);

        window.reset();
        window.setFramePos(framePos);

        QTRY_VERIFY(window.received(QEvent::Move));
        QTRY_COMPARE(framePos, window.framePos());
        QTRY_COMPARE(originalMargins, window.frameMargins());
        QCOMPARE(window.pos(), window.framePos() + QPoint(originalMargins.left(), originalMargins.top()));

        // and back to regular positioning

        window.reset();
        window.setPos(originalPos);
        QTRY_VERIFY(window.received(QEvent::Move));
        QTRY_COMPARE(originalPos, window.pos());
    }
}

void tst_QWindow::isActive()
{
    Window window;
    window.setGeometry(80, 80, 40, 40);
    window.show();

#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "This test fails on Mac OS X, see QTBUG-23059", Abort);
#endif
    QTRY_COMPARE(window.received(QEvent::Map), 1);
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &window);
    QVERIFY(window.isActive());

    Window child;
    child.setParent(&window);
    child.setGeometry(10, 10, 20, 20);
    child.show();

    QTRY_COMPARE(child.received(QEvent::Map), 1);

    child.requestActivateWindow();

    QTRY_VERIFY(QGuiApplication::focusWindow() == &child);
    QVERIFY(child.isActive());

    // parent shouldn't receive new map or resize events from child being shown
    QTRY_COMPARE(window.received(QEvent::Map), 1);
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_COMPARE(window.received(QEvent::FocusIn), 1);
    QTRY_COMPARE(window.received(QEvent::FocusOut), 1);
    QTRY_COMPARE(child.received(QEvent::FocusIn), 1);

    // child has focus
    QVERIFY(window.isActive());

    Window dialog;
    dialog.setTransientParent(&window);
    dialog.setGeometry(110, 110, 30, 30);
    dialog.show();

    dialog.requestActivateWindow();

    QTRY_COMPARE(dialog.received(QEvent::Map), 1);
    QTRY_COMPARE(dialog.received(QEvent::Resize), 1);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &dialog);
    QVERIFY(dialog.isActive());

    // transient child has focus
    QVERIFY(window.isActive());

    // parent is active
    QVERIFY(child.isActive());

    window.requestActivateWindow();

    QTRY_VERIFY(QGuiApplication::focusWindow() == &window);
    QTRY_COMPARE(dialog.received(QEvent::FocusOut), 1);
    QTRY_COMPARE(window.received(QEvent::FocusIn), 2);

    QVERIFY(window.isActive());

    // transient parent has focus
    QVERIFY(dialog.isActive());

    // parent has focus
    QVERIFY(child.isActive());
}

class InputTestWindow : public QWindow
{
public:
    void keyPressEvent(QKeyEvent *event) {
        keyPressCode = event->key();
    }
    void keyReleaseEvent(QKeyEvent *event) {
        keyReleaseCode = event->key();
    }
    void mousePressEvent(QMouseEvent *event) {
        if (ignoreMouse) {
            event->ignore();
        } else {
            ++mousePressedCount;
            mouseSequenceSignature += 'p';
            mousePressButton = event->button();
            mousePressScreenPos = event->screenPos();
        }
    }
    void mouseReleaseEvent(QMouseEvent *event) {
        if (ignoreMouse) {
            event->ignore();
        } else {
            ++mouseReleasedCount;
            mouseSequenceSignature += 'r';
            mouseReleaseButton = event->button();
        }
    }
    void mouseMoveEvent(QMouseEvent *event) {
        if (ignoreMouse) {
            event->ignore();
        } else {
            mouseMoveButton = event->button();
            mouseMoveScreenPos = event->screenPos();
        }
    }
    void mouseDoubleClickEvent(QMouseEvent *event) {
        if (ignoreMouse) {
            event->ignore();
        } else {
            ++mouseDoubleClickedCount;
            mouseSequenceSignature += 'd';
        }
    }
    void touchEvent(QTouchEvent *event) {
        if (ignoreTouch) {
            event->ignore();
            return;
        }
        touchEventType = event->type();
        QList<QTouchEvent::TouchPoint> points = event->touchPoints();
        for (int i = 0; i < points.count(); ++i) {
            switch (points.at(i).state()) {
            case Qt::TouchPointPressed:
                ++touchPressedCount;
                break;
            case Qt::TouchPointReleased:
                ++touchReleasedCount;
                break;
            case Qt::TouchPointMoved:
                ++touchMovedCount;
                break;
            default:
                break;
            }
        }
    }
    void resetCounters() {
        mousePressedCount = mouseReleasedCount = mouseDoubleClickedCount = 0;
        mouseSequenceSignature = QString();
        touchPressedCount = touchReleasedCount = touchMovedCount = 0;
    }

    InputTestWindow() {
        keyPressCode = keyReleaseCode = 0;
        mousePressButton = mouseReleaseButton = mouseMoveButton = 0;
        ignoreMouse = ignoreTouch = false;
        resetCounters();
    }

    int keyPressCode, keyReleaseCode;
    int mousePressButton, mouseReleaseButton, mouseMoveButton;
    int mousePressedCount, mouseReleasedCount, mouseDoubleClickedCount;
    QString mouseSequenceSignature;
    QPointF mousePressScreenPos, mouseMoveScreenPos;
    int touchPressedCount, touchReleasedCount, touchMovedCount;
    QEvent::Type touchEventType;

    bool ignoreMouse, ignoreTouch;
};

void tst_QWindow::testInputEvents()
{
    InputTestWindow window;
    window.setGeometry(80, 80, 40, 40);
    window.show();
    QTest::qWaitForWindowShown(&window);

    QWindowSystemInterface::handleKeyEvent(&window, QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    QWindowSystemInterface::handleKeyEvent(&window, QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier);
    QCoreApplication::processEvents();
    QCOMPARE(window.keyPressCode, int(Qt::Key_A));
    QCOMPARE(window.keyReleaseCode, int(Qt::Key_A));

    QPointF local(12, 34);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressButton, int(Qt::LeftButton));
    QCOMPARE(window.mouseReleaseButton, int(Qt::LeftButton));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1, tp2;
    tp1.id = 1;
    tp1.state = Qt::TouchPointPressed;
    tp1.area = QRect(10, 10, 4, 4);
    tp2.id = 2;
    tp2.state = Qt::TouchPointPressed;
    tp2.area = QRect(20, 20, 4, 4);
    points << tp1 << tp2;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = Qt::TouchPointReleased;
    points[1].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchPressedCount, 2);
    QTRY_COMPARE(window.touchReleasedCount, 2);
}

void tst_QWindow::touchToMouseTranslation()
{
    InputTestWindow window;
    window.ignoreTouch = true;
    window.setGeometry(80, 80, 40, 40);
    window.show();
    QTest::qWaitForWindowShown(&window);

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1, tp2;
    const QRectF pressArea(101, 102, 4, 4);
    const QRectF moveArea(105, 108, 4, 4);
    tp1.id = 1;
    tp1.state = Qt::TouchPointPressed;
    tp1.area = pressArea;
    tp2.id = 2;
    tp2.state = Qt::TouchPointPressed;
    points << tp1 << tp2;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    // Now an update but with changed list order. The mouse event should still
    // be generated from the point with id 1.
    tp1.id = 2;
    tp1.state = Qt::TouchPointStationary;
    tp2.id = 1;
    tp2.state = Qt::TouchPointMoved;
    tp2.area = moveArea;
    points.clear();
    points << tp1 << tp2;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = Qt::TouchPointReleased;
    points[1].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();

    QTRY_COMPARE(window.mousePressButton, int(Qt::LeftButton));
    QTRY_COMPARE(window.mouseReleaseButton, int(Qt::LeftButton));
    QTRY_COMPARE(window.mousePressScreenPos, pressArea.center());
    QTRY_COMPARE(window.mouseMoveScreenPos, moveArea.center());

    window.mousePressButton = 0;
    window.mouseReleaseButton = 0;

    window.ignoreTouch = false;

    points[0].state = Qt::TouchPointPressed;
    points[1].state = Qt::TouchPointPressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = Qt::TouchPointReleased;
    points[1].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();

    // no new mouse events should be generated since the input window handles the touch events
    QTRY_COMPARE(window.mousePressButton, 0);
    QTRY_COMPARE(window.mouseReleaseButton, 0);

    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

    window.ignoreTouch = true;
    points[0].state = Qt::TouchPointPressed;
    points[1].state = Qt::TouchPointPressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = Qt::TouchPointReleased;
    points[1].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();

    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);

    // mouse event synthesizing disabled
    QTRY_COMPARE(window.mousePressButton, 0);
    QTRY_COMPARE(window.mouseReleaseButton, 0);
}

void tst_QWindow::mouseToTouchTranslation()
{
    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);

    InputTestWindow window;
    window.ignoreMouse = true;
    window.setGeometry(80, 80, 40, 40);
    window.show();
    QTest::qWaitForWindowShown(&window);

    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::NoButton);
    QCoreApplication::processEvents();

    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);

    QTRY_COMPARE(window.touchPressedCount, 1);
    QTRY_COMPARE(window.touchReleasedCount, 1);

    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);

    window.ignoreMouse = false;

    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::NoButton);
    QCoreApplication::processEvents();

    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);

    // no new touch events should be generated since the input window handles the mouse events
    QTRY_COMPARE(window.touchPressedCount, 1);
    QTRY_COMPARE(window.touchReleasedCount, 1);

    window.ignoreMouse = true;

    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::NoButton);
    QCoreApplication::processEvents();

    // touch event synthesis disabled
    QTRY_COMPARE(window.touchPressedCount, 1);
    QTRY_COMPARE(window.touchReleasedCount, 1);


}

void tst_QWindow::mouseToTouchLoop()
{
    // make sure there's no infinite loop when synthesizing both ways
    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);

    InputTestWindow window;
    window.ignoreMouse = true;
    window.ignoreTouch = true;
    window.setGeometry(80, 80, 40, 40);
    window.show();
    QTest::qWaitForWindowShown(&window);

    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::NoButton);
    QCoreApplication::processEvents();

    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);
}

void tst_QWindow::touchCancel()
{
    InputTestWindow window;
    window.setGeometry(80, 80, 40, 40);
    window.show();
    QTest::qWaitForWindowShown(&window);

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1;
    tp1.id = 1;

    // Start a touch.
    tp1.state = Qt::TouchPointPressed;
    tp1.area = QRect(10, 10, 4, 4);
    points << tp1;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchBegin);
    QTRY_COMPARE(window.touchPressedCount, 1);

    // Cancel the active touch sequence.
    QWindowSystemInterface::handleTouchCancelEvent(&window, touchDevice);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchCancel);

    // Send a move -> will not be delivered due to the cancellation.
    QTRY_COMPARE(window.touchMovedCount, 0);
    points[0].state = Qt::TouchPointMoved;
    tp1.area.adjust(2, 2, 2, 2);
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchMovedCount, 0);

    // Likewise. The only allowed transition is TouchCancel -> TouchBegin.
    QTRY_COMPARE(window.touchReleasedCount, 0);
    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchReleasedCount, 0);

    // Start a new sequence -> from this point on everything should go through normally.
    points[0].state = Qt::TouchPointPressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchBegin);
    QTRY_COMPARE(window.touchPressedCount, 2);

    points[0].state = Qt::TouchPointMoved;
    tp1.area.adjust(2, 2, 2, 2);
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchMovedCount, 1);

    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchReleasedCount, 1);
}

void tst_QWindow::touchCancelWithTouchToMouse()
{
    InputTestWindow window;
    window.ignoreTouch = true;
    window.setGeometry(80, 80, 40, 40);
    window.show();
    QTest::qWaitForWindowShown(&window);

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1;
    tp1.id = 1;

    tp1.state = Qt::TouchPointPressed;
    tp1.area = QRect(100, 100, 4, 4);
    points << tp1;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mousePressButton, int(Qt::LeftButton));
    QTRY_COMPARE(window.mousePressScreenPos, points[0].area.center());

    // Cancel the touch. Should result in a mouse release for windows that have
    // have an active touch-to-mouse sequence.
    QWindowSystemInterface::handleTouchCancelEvent(0, touchDevice);
    QCoreApplication::processEvents();

    QTRY_COMPARE(window.mouseReleaseButton, int(Qt::LeftButton));

    // Now change the window to accept touches.
    window.mousePressButton = window.mouseReleaseButton = 0;
    window.ignoreTouch = false;

    // Send a touch, there will be no mouse event generated.
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mousePressButton, 0);

    // Cancel the touch. It should not result in a mouse release with this window.
    QWindowSystemInterface::handleTouchCancelEvent(0, touchDevice);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mouseReleaseButton, 0);
}

void tst_QWindow::orientation()
{
    qRegisterMetaType<Qt::ScreenOrientation>("Qt::ScreenOrientation");

    QWindow window;
    window.setGeometry(80, 80, 40, 40);
    window.create();

    window.reportContentOrientationChange(Qt::PortraitOrientation);
    QCOMPARE(window.contentOrientation(), Qt::PortraitOrientation);

    window.reportContentOrientationChange(Qt::PrimaryOrientation);
    QCOMPARE(window.contentOrientation(), Qt::PrimaryOrientation);

    QVERIFY(!window.requestWindowOrientation(Qt::LandscapeOrientation) || window.windowOrientation() == Qt::LandscapeOrientation);
    QVERIFY(!window.requestWindowOrientation(Qt::PortraitOrientation) || window.windowOrientation() == Qt::PortraitOrientation);
    QVERIFY(!window.requestWindowOrientation(Qt::PrimaryOrientation) || window.windowOrientation() == Qt::PrimaryOrientation);

    QSignalSpy spy(&window, SIGNAL(contentOrientationChanged(Qt::ScreenOrientation)));
    window.reportContentOrientationChange(Qt::LandscapeOrientation);
    QCOMPARE(spy.count(), 1);
}

void tst_QWindow::close()
{
    QWindow a;
    QWindow b;
    QWindow c(&a);

    a.show();
    b.show();

    // we can not close a non top level window
    QVERIFY(!c.close());
    QVERIFY(a.close());
    QVERIFY(b.close());
}

void tst_QWindow::activateAndClose()
{
    for (int i = 0; i < 10; ++i)  {
       QWindow window;
       window.show();
       QTest::qWaitForWindowShown(&window);
       window.requestActivateWindow();
       QTRY_COMPARE(qGuiApp->focusWindow(), &window);
    }
}

void tst_QWindow::mouseEventSequence()
{
    int doubleClickInterval = qGuiApp->styleHints()->mouseDoubleClickInterval();

    InputTestWindow window;
    window.setGeometry(80, 80, 40, 40);
    window.show();
    QTest::qWaitForWindowShown(&window);

    ulong timestamp = 0;
    QPointF local(12, 34);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 1);
    QCOMPARE(window.mouseReleasedCount, 1);
    QCOMPARE(window.mouseDoubleClickedCount, 0);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("pr"));

    window.resetCounters();
    timestamp += doubleClickInterval;

    // A double click must result in press, release, press, doubleclick, release.
    // Check that no unexpected event suppression occurs and that the order is correct.
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 2);
    QCOMPARE(window.mouseReleasedCount, 2);
    QCOMPARE(window.mouseDoubleClickedCount, 1);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prpdr"));

    timestamp += doubleClickInterval;
    window.resetCounters();

    // Triple click = double + single click
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 3);
    QCOMPARE(window.mouseReleasedCount, 3);
    QCOMPARE(window.mouseDoubleClickedCount, 1);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prpdrpr"));

    timestamp += doubleClickInterval;
    window.resetCounters();

    // Two double clicks.
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 4);
    QCOMPARE(window.mouseReleasedCount, 4);
    QCOMPARE(window.mouseDoubleClickedCount, 2);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prpdrprpdr"));

    timestamp += doubleClickInterval;
    window.resetCounters();

    // Four clicks, none of which qualifies as a double click.
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    timestamp += doubleClickInterval;
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    timestamp += doubleClickInterval;
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    timestamp += doubleClickInterval;
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    timestamp += doubleClickInterval;
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 4);
    QCOMPARE(window.mouseReleasedCount, 4);
    QCOMPARE(window.mouseDoubleClickedCount, 0);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prprprpr"));
}

void tst_QWindow::windowModality()
{
    qRegisterMetaType<Qt::WindowModality>("Qt::WindowModality");

    QWindow window;
    QSignalSpy spy(&window, SIGNAL(windowModalityChanged(Qt::WindowModality)));

    QCOMPARE(window.windowModality(), Qt::NonModal);
    window.setWindowModality(Qt::NonModal);
    QCOMPARE(window.windowModality(), Qt::NonModal);
    QCOMPARE(spy.count(), 0);

    window.setWindowModality(Qt::WindowModal);
    QCOMPARE(window.windowModality(), Qt::WindowModal);
    QCOMPARE(spy.count(), 1);
    window.setWindowModality(Qt::WindowModal);
    QCOMPARE(window.windowModality(), Qt::WindowModal);
    QCOMPARE(spy.count(), 1);

    window.setWindowModality(Qt::ApplicationModal);
    QCOMPARE(window.windowModality(), Qt::ApplicationModal);
    QCOMPARE(spy.count(), 2);
    window.setWindowModality(Qt::ApplicationModal);
    QCOMPARE(window.windowModality(), Qt::ApplicationModal);
    QCOMPARE(spy.count(), 2);

    window.setWindowModality(Qt::NonModal);
    QCOMPARE(window.windowModality(), Qt::NonModal);
    QCOMPARE(spy.count(), 3);
}

#include <tst_qwindow.moc>
QTEST_MAIN(tst_QWindow)
