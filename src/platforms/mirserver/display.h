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

#ifndef DISPLAY_H
#define DISPLAY_H

#include <qpa/qplatformscreen.h>
#include <memory>

namespace mir { namespace graphics { class DisplayConfiguration; }}

class Display
{
public:
    Display(const std::shared_ptr<mir::graphics::DisplayConfiguration> &displayConfig);
    virtual ~Display();

    QList<QPlatformScreen *> screens() const { return m_screens; }

private:
    QList<QPlatformScreen *> m_screens;
};

#endif // DISPLAY_H
