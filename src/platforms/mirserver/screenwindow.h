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

#ifndef SCREENWINDOW_H
#define SCREENWINDOW_H

#include <qpa/qplatformwindow.h>

#include <QObject>

// ScreenWindow implements the basics of a QPlatformWindow.
// QtMir enforces one Window per Screen, so Window and Screen are tightly coupled.
// All Mir specifics live in the associated Screen object.

class ScreenWindow : public QObject, public QPlatformWindow
{
    Q_OBJECT
public:
    explicit ScreenWindow(QWindow *window);
    virtual ~ScreenWindow();

    WId winId() const override { return m_winId; }

    //bool isExposed() const override;

    bool event(QEvent *event) override;

    void swapBuffers();
    void makeCurrent();
    void doneCurrent();

private:
    bool m_isExposed;
    WId m_winId;
};

#endif // SCREENWINDOW_H