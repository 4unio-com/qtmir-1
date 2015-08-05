/*
 * Copyright (C) 2015 Canonical, Ltd.
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
 */

#ifndef SCREENCONTROLLER_H
#define SCREENCONTROLLER_H

#include <QObject>
#include <QPoint>

// Mir
#include <mir/graphics/display_configuration.h>

// std
#include <memory>

// local
#include "mirserver.h"

class Screen;
class QWindow;


class ScreenController : public QObject
{
    Q_OBJECT
public:
    explicit ScreenController(QObject *parent = 0);

    Screen* getUnusedScreen();
    QList<Screen*> screens() const { return m_screenList; }

    QWindow* getWindowForPoint(const QPoint &point);

Q_SIGNALS:
    void screenAdded(Screen *screen);

public Q_SLOTS:
    void update();

protected: // Protected for Testing Purposes
    Screen* findScreenWithId(const QList<Screen*> &list, const mir::graphics::DisplayConfigurationOutputId id);
    void init(const std::shared_ptr<mir::graphics::Display> &display,
              const std::shared_ptr<mir::compositor::Compositor> &compositor,
              const std::shared_ptr<mir::MainLoop> &mainLoop);
    void terminate();
    virtual Screen *screenFactory(const mir::graphics::DisplayConfigurationOutput &output) const;

protected Q_SLOTS:
    void onCompositorStarting();
    void onCompositorStopping();

private:
    std::weak_ptr<mir::graphics::Display> m_display;
    std::shared_ptr<mir::compositor::Compositor> m_compositor;
    QList<Screen*> m_screenList;

    friend class MirServer;
};

#endif // SCREENCONTROLLER_H