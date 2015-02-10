/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mirshell.h"
#include "logging.h"
#include "tracepoints.h" // generated from tracepoints.tp

#include <mir/geometry/rectangle.h>
#include <mir/scene/session.h>
#include <mir/scene/surface_creation_parameters.h>
#include <mir/shell/display_layout.h>

namespace ms = mir::scene;
using mir::shell::AbstractShell;

MirShell::MirShell(
    const std::shared_ptr<mir::shell::InputTargeter> &inputTargeter,
    const std::shared_ptr<mir::scene::SurfaceCoordinator> &surfaceCoordinator,
    const std::shared_ptr<mir::scene::SessionCoordinator> &sessionCoordinator,
    const std::shared_ptr<mir::scene::PromptSessionManager> &promptSessionManager,
    const std::shared_ptr<mir::shell::DisplayLayout> &displayLayout) :
    AbstractShell(inputTargeter, surfaceCoordinator, sessionCoordinator, promptSessionManager),
    m_displayLayout{displayLayout}
{
    qCDebug(QTMIR_MIR_MESSAGES) << "MirShell::MirShell";
}

mir::frontend::SurfaceId MirShell::create_surface(const std::shared_ptr<ms::Session> &session, const ms::SurfaceCreationParameters &requestParameters)
{
    tracepoint(qtmirserver, surfacePlacementStart);

     // TODO: Callback unity8 so that it can make a decision on that.
     //       unity8 must bear in mind that the called function will be on a Mir thread though.
     //       The QPA shouldn't be deciding for itself on such things.

     ms::SurfaceCreationParameters placedParameters = requestParameters;

     // Just make it fullscreen for now
     mir::geometry::Rectangle rect{requestParameters.top_left, requestParameters.size};
     m_displayLayout->size_to_output(rect);
     placedParameters.size = rect.size;

     qCDebug(QTMIR_MIR_MESSAGES) << "MirShell::create_surface(): size requested ("
         << requestParameters.size.width.as_int() << "," << requestParameters.size.height.as_int() << ") and placed ("
         << placedParameters.size.width.as_int() << "," << placedParameters.size.height.as_int() << ")";

     tracepoint(qtmirserver, surfacePlacementEnd);

     return AbstractShell::create_surface(session, placedParameters);
}

int MirShell::set_surface_attribute(
    const std::shared_ptr<mir::scene::Session> &session,
    const std::shared_ptr<mir::scene::Surface> &surface,
    MirSurfaceAttrib attrib,
    int value)
{
    auto const result = AbstractShell::set_surface_attribute(session, surface, attrib, value);
    Q_EMIT surfaceAttributeChanged(surface.get(), attrib, result);

    return result;
}
