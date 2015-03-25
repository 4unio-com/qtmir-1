/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Gerry Boland <gerry.boland@canonical.com>
 */

#ifndef DISPLAYWINDOW_H
#define DISPLAYWINDOW_H

#include <qpa/qplatformwindow.h>

#include <mir/graphics/display.h>
#include <mir/graphics/display_buffer.h>

#include <QObject>

// DisplayWindow wraps the whatever implementation Mir creates of a DisplayBuffer,
// which is the buffer output for an individual display.

class DisplayWindow : public QObject, public QPlatformWindow
{
    Q_OBJECT
public:
    explicit DisplayWindow(QWindow *window, mir::graphics::DisplaySyncGroup*, mir::graphics::DisplayBuffer*);

    QRect geometry() const override;
    void setGeometry(const QRect &rect) override;

    WId winId() const override { return m_winId; }

    bool isExposed() const override;

    bool event(QEvent *event) override;

    void swapBuffers();
    void makeCurrent();
    void doneCurrent();

private:
    bool m_isExposed;
    WId m_winId;
    mir::graphics::DisplaySyncGroup *m_displayGroup;
    mir::graphics::DisplayBuffer *m_displayBuffer;
};

#endif // DISPLAYWINDOW_H
