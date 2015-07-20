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

#include "screencontroller.h"

struct TestableScreenController : public ScreenController
{
    Q_OBJECT

public:
    void do_init(const std::shared_ptr<mir::graphics::Display> &display,
              const std::shared_ptr<mir::compositor::Compositor> &compositor,
              const std::shared_ptr<mir::MainLoop> &mainLoop)
    {
        init(display, compositor, mainLoop);
    }

    void do_terminate() { terminate(); }
};
