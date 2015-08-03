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

#include "mirwindowmanager.h"
#include "basic_window_manager.h"
#include "canonical_window_manager_info.h"
#include "logging.h"
#include "tracepoints.h" // generated from tracepoints.tp

#include <mir/geometry/rectangle.h>
#include <mir/scene/session.h>
#include <mir/scene/surface_creation_parameters.h>
#include <mir/scene/surface.h>
#include <mir/shell/display_layout.h>
#include <mir/shell/surface_ready_observer.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
using namespace mir::geometry;

namespace 
{
class CanonicalWindowManagerPolicy
{
public:
    using Tools = mir::shell::QtmirBasicWindowManagerTools<mir::shell::QtmirSessionInfo, mir::shell::QtmirSurfaceInfo>;
    using CanonicalSessionInfoMap = typename mir::shell::QtmirSessionTo<mir::shell::QtmirSessionInfo>::type;
    using CanonicalSurfaceInfoMap = typename mir::shell::QtmirSurfaceTo<mir::shell::QtmirSurfaceInfo>::type;

    explicit CanonicalWindowManagerPolicy(
        Tools* const tools,
        std::shared_ptr<mir::shell::DisplayLayout> const& display_layout);

//    void click(mir::geometry::Point cursor);
//
    void handle_session_info_updated(
        CanonicalSessionInfoMap& session_info,
        mir::geometry::Rectangles const& displays);

    void handle_displays_updated(
        CanonicalSessionInfoMap& session_info,
        mir::geometry::Rectangles const& displays);

//    void resize(mir::geometry::Point cursor);
//
    auto handle_place_new_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& request_parameters)
        -> mir::scene::SurfaceCreationParameters;

    void handle_new_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface);

    void handle_modify_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        mir::shell::SurfaceSpecification const& modifications);

    void handle_delete_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::weak_ptr<mir::scene::Surface> const& surface);

    int handle_set_state(
        std::shared_ptr<mir::scene::Surface> const& surface, MirSurfaceState value);

//    void drag(mir::geometry::Point cursor);
//
    bool handle_keyboard_event(MirKeyboardEvent const* event);

    bool handle_touch_event(MirTouchEvent const* event);

    bool handle_pointer_event(MirPointerEvent const* event);

    std::vector<std::shared_ptr<mir::scene::Surface>> generate_decorations_for(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface);

private:
    Tools* const tools;
    std::shared_ptr<mir::shell::DisplayLayout> const display_layout;
};

class MirWindowManagerImpl : public MirWindowManager
{
public:

    MirWindowManagerImpl(const std::shared_ptr<mir::shell::DisplayLayout> &displayLayout);

    void add_session(std::shared_ptr<mir::scene::Session> const& session) override;

    void remove_session(std::shared_ptr<mir::scene::Session> const& session) override;

    mir::frontend::SurfaceId add_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params,
        std::function<mir::frontend::SurfaceId(std::shared_ptr<mir::scene::Session> const& session, mir::scene::SurfaceCreationParameters const& params)> const& build) override;

    void remove_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::weak_ptr<mir::scene::Surface> const& surface) override;

    void add_display(mir::geometry::Rectangle const& area) override;

    void remove_display(mir::geometry::Rectangle const& area) override;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    bool handle_touch_event(MirTouchEvent const* event) override;

    bool handle_pointer_event(MirPointerEvent const* event) override;

    int set_surface_attribute(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        MirSurfaceAttrib attrib,
        int value) override;

    void modify_surface(const std::shared_ptr<mir::scene::Session>&, const std::shared_ptr<mir::scene::Surface>&, const mir::shell::SurfaceSpecification&);

private:
    std::shared_ptr<mir::shell::DisplayLayout> const m_displayLayout;
};

}

CanonicalWindowManagerPolicy::CanonicalWindowManagerPolicy(
    Tools* const tools,
    std::shared_ptr<msh::DisplayLayout> const& display_layout) :
    tools{tools},
    display_layout{display_layout}
{
}

//void CanonicalWindowManagerPolicy::click(Point cursor)
//{
//    // TODO
//}
//
void CanonicalWindowManagerPolicy::handle_session_info_updated(CanonicalSessionInfoMap& /*session_info*/, Rectangles const& /*displays*/)
{
    // TODO
}

void CanonicalWindowManagerPolicy::handle_displays_updated(CanonicalSessionInfoMap& /*session_info*/, Rectangles const& /*displays*/)
{
    // TODO
}

//void CanonicalWindowManagerPolicy::resize(Point cursor)
//{
//    // TODO
//}

auto CanonicalWindowManagerPolicy::handle_place_new_surface(
    std::shared_ptr<ms::Session> const& /*session*/,
    ms::SurfaceCreationParameters const& request_parameters)
-> ms::SurfaceCreationParameters
{
    auto parameters = request_parameters;

    tracepoint(qtmirserver, surfacePlacementStart);

    // TODO: Callback unity8 so that it can make a decision on that.
    //       unity8 must bear in mind that the called function will be on a Mir thread though.
    //       The QPA shouldn't be deciding for itself on such things.

    // Just make it fullscreen for now
    mir::geometry::Rectangle rect{request_parameters.top_left, request_parameters.size};
    display_layout->size_to_output(rect);
    parameters.size = rect.size;

    qCDebug(QTMIR_MIR_MESSAGES) << "MirWindowManagerImpl::add_surface(): size requested ("
                                << request_parameters.size.width.as_int() << "," << request_parameters.size.height.as_int() << ") and placed ("
                                << parameters.size.width.as_int() << "," << parameters.size.height.as_int() << ")";

    tracepoint(qtmirserver, surfacePlacementEnd);

//    if (!parameters.type.is_set())
//        throw std::runtime_error("Surface type must be provided");
//
//    if (!parameters.state.is_set())
//        parameters.state = mir_surface_state_restored;
//
//    auto const active_display = tools->active_display();
//
//    auto const width = parameters.size.width.as_int();
//    auto const height = parameters.size.height.as_int();
//
//    bool positioned = false;
//
//    auto const parent = parameters.parent.lock();
//
//    if (msh::QtmirSurfaceInfo::must_not_have_parent(parameters.type.value()) && parent)
//        throw std::runtime_error("Surface type cannot have parent");
//
//    if (msh::QtmirSurfaceInfo::must_have_parent(parameters.type.value()) && !parent)
//        throw std::runtime_error("Surface type must have parent");
//
//    if (parameters.output_id != mir::graphics::DisplayConfigurationOutputId{0})
//    {
//        Rectangle rect{parameters.top_left, parameters.size};
//        display_layout->place_in_output(parameters.output_id, rect);
//        parameters.top_left = rect.top_left;
//        parameters.size = rect.size;
//        parameters.state = mir_surface_state_fullscreen;
//        positioned = true;
//    }
//    else if (!parent) // No parent => client can't suggest positioning
//    {
//        if (auto const default_surface = session->default_surface())
//        {
//            static Displacement const offset{title_bar_height, title_bar_height};
//
//            parameters.top_left = default_surface->top_left() + offset;
//
//            geometry::Rectangle display_for_app{default_surface->top_left(), default_surface->size()};
//            display_layout->size_to_output(display_for_app);
//
////            // TODO This is what is currently in the spec, but I think it's wrong
////            if (!display_for_app.contains(parameters.top_left + as_displacement(parameters.size)))
////            {
////                parameters.size = as_size(display_for_app.bottom_right() - parameters.top_left);
////            }
////
////            positioned = display_for_app.contains(Rectangle{parameters.top_left, parameters.size});
//
//
//            positioned = display_for_app.overlaps(Rectangle{parameters.top_left, parameters.size});
//        }
//    }
//
//    if (parent && parameters.aux_rect.is_set() && parameters.edge_attachment.is_set())
//    {
//        auto const edge_attachment = parameters.edge_attachment.value();
//        auto const aux_rect = parameters.aux_rect.value();
//        auto const parent_top_left = parent->top_left();
//        auto const top_left = aux_rect.top_left     -Point{} + parent_top_left;
//        auto const top_right= aux_rect.top_right()  -Point{} + parent_top_left;
//        auto const bot_left = aux_rect.bottom_left()-Point{} + parent_top_left;
//
//        if (edge_attachment & mir_edge_attachment_vertical)
//        {
//            if (active_display.contains(top_right + Displacement{width, height}))
//            {
//                parameters.top_left = top_right;
//                positioned = true;
//            }
//            else if (active_display.contains(top_left + Displacement{-width, height}))
//            {
//                parameters.top_left = top_left + Displacement{-width, 0};
//                positioned = true;
//            }
//        }
//
//        if (edge_attachment & mir_edge_attachment_horizontal)
//        {
//            if (active_display.contains(bot_left + Displacement{width, height}))
//            {
//                parameters.top_left = bot_left;
//                positioned = true;
//            }
//            else if (active_display.contains(top_left + Displacement{width, -height}))
//            {
//                parameters.top_left = top_left + Displacement{0, -height};
//                positioned = true;
//            }
//        }
//    }
//    else if (parent)
//    {
//        //  o Otherwise, if the dialog is not the same as any previous dialog for the
//        //    same parent window, and/or it does not have user-customized position:
//        //      o It should be optically centered relative to its parent, unless this
//        //        would overlap or cover the title bar of the parent.
//        //      o Otherwise, it should be cascaded vertically (but not horizontally)
//        //        relative to its parent, unless, this would cause at least part of
//        //        it to extend into shell space.
//        auto const parent_top_left = parent->top_left();
//        auto const centred = parent_top_left
//             + 0.5*(as_displacement(parent->size()) - as_displacement(parameters.size))
//             - DeltaY{(parent->size().height.as_int()-height)/6};
//
//        parameters.top_left = centred;
//        positioned = true;
//    }
//
//    if (!positioned)
//    {
//        auto const centred = active_display.top_left
//            + 0.5*(as_displacement(active_display.size) - as_displacement(parameters.size))
//            - DeltaY{(active_display.size.height.as_int()-height)/6};
//
//        switch (parameters.state.value())
//        {
//        case mir_surface_state_fullscreen:
//        case mir_surface_state_maximized:
//            parameters.top_left = active_display.top_left;
//            parameters.size = active_display.size;
//            break;
//
//        case mir_surface_state_vertmaximized:
//            parameters.top_left = centred;
//            parameters.top_left.y = active_display.top_left.y;
//            parameters.size.height = active_display.size.height;
//            break;
//
//        case mir_surface_state_horizmaximized:
//            parameters.top_left = centred;
//            parameters.top_left.x = active_display.top_left.x;
//            parameters.size.width = active_display.size.width;
//            break;
//
//        default:
//            parameters.top_left = centred;
//        }
//
//        if (parameters.top_left.y < display_area.top_left.y)
//            parameters.top_left.y = display_area.top_left.y;
//    }

    return parameters;
}

void CanonicalWindowManagerPolicy::handle_new_surface(std::shared_ptr<ms::Session> const& session, std::shared_ptr<ms::Surface> const& surface)
{
    auto& surface_info = tools->info_for(surface);
    if (auto const parent = surface->parent())
    {
        tools->info_for(parent).children.push_back(surface);
    }

    tools->info_for(session).surfaces++;

    if (surface_info.can_be_active())
    {
        surface->add_observer(std::make_shared<msh::SurfaceReadyObserver>(
            [this](std::shared_ptr<ms::Session> const& /*session*/,
                   std::shared_ptr<ms::Surface> const& /*surface*/)
                {
//                    select_active_surface(surface);
                },
            session,
            surface));
    }
}

void CanonicalWindowManagerPolicy::handle_modify_surface(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    msh::SurfaceSpecification const& modifications)
{
    auto& surface_info_old = tools->info_for(surface);

    auto surface_info = surface_info_old;

    if (modifications.parent.is_set())
        surface_info.parent = modifications.parent.value();

    if (modifications.type.is_set() &&
        surface_info.type != modifications.type.value())
    {
        auto const new_type = modifications.type.value();

        if (!surface_info.can_morph_to(new_type))
        {
            throw std::runtime_error("Unsupported surface type change");
        }

        surface_info.type = new_type;

        if (surface_info.must_not_have_parent())
        {
            if (modifications.parent.is_set())
                throw std::runtime_error("Target surface type does not support parent");

            surface_info.parent.reset();
        }
        else if (surface_info.must_have_parent())
        {
            if (!surface_info.parent.lock())
                throw std::runtime_error("Target surface type requires parent");
        }

        surface->configure(mir_surface_attrib_type, new_type);
    }

    #define COPY_IF_SET(field)\
        if (modifications.field.is_set())\
            surface_info.field = modifications.field.value()

    COPY_IF_SET(min_width);
    COPY_IF_SET(min_height);
    COPY_IF_SET(max_width);
    COPY_IF_SET(max_height);
    COPY_IF_SET(min_width);
    COPY_IF_SET(width_inc);
    COPY_IF_SET(height_inc);
    COPY_IF_SET(min_aspect);
    COPY_IF_SET(max_aspect);

    #undef COPY_IF_SET

    std::swap(surface_info, surface_info_old);

    if (modifications.name.is_set())
        surface->rename(modifications.name.value());

    if (modifications.streams.is_set())
    {
        auto v = modifications.streams.value();
        std::vector<msh::StreamSpecification> l (v.begin(), v.end());
        session->configure_streams(*surface, l);
    }

    if (modifications.width.is_set() || modifications.height.is_set())
    {
        auto new_size = surface->size();

        if (modifications.width.is_set())
            new_size.width = modifications.width.value();

        if (modifications.height.is_set())
            new_size.height = modifications.height.value();

//        auto top_left = surface->top_left();
//
//        surface_info.constrain_resize(
//            surface,
//            top_left,
//            new_size,
//            false,
//            false,
//            display_area);
//
//        apply_resize(surface, top_left, new_size);
    }

//    if (modifications.input_shape.is_set())
//        surface->set_input_region(modifications.input_shape.value());
}

void CanonicalWindowManagerPolicy::handle_delete_surface(std::shared_ptr<ms::Session> const& session, std::weak_ptr<ms::Surface> const& surface)
{
    auto& info = tools->info_for(surface);

    if (auto const parent = info.parent.lock())
    {
        auto& siblings = tools->info_for(parent).children;

        for (auto i = begin(siblings); i != end(siblings); ++i)
        {
            if (surface.lock() == i->lock())
            {
                siblings.erase(i);
                break;
            }
        }
    }

    if (!--tools->info_for(session).surfaces && session == tools->focused_session())
    {
//        active_surface_.reset();
        tools->focus_next_session();
//        select_active_surface(tools->focused_surface());
    }
}

int CanonicalWindowManagerPolicy::handle_set_state(std::shared_ptr<ms::Surface> const& surface, MirSurfaceState value)
{
    return surface->configure(mir_surface_attrib_state, value);

//    auto& info = tools->info_for(surface);
//
//    switch (value)
//    {
//    case mir_surface_state_restored:
//    case mir_surface_state_maximized:
//    case mir_surface_state_vertmaximized:
//    case mir_surface_state_horizmaximized:
//    case mir_surface_state_fullscreen:
//    case mir_surface_state_hidden:
//    case mir_surface_state_minimized:
//        break;
//
//    default:
//        return info.state;
//    }
//
//    if (info.state == mir_surface_state_restored)
//    {
//        info.restore_rect = {surface->top_left(), surface->size()};
//    }
//
//    if (info.state == value)
//    {
//        return info.state;
//    }
//
//    auto const old_pos = surface->top_left();
//    Displacement movement;
//
//    switch (value)
//    {
//    case mir_surface_state_restored:
//        movement = info.restore_rect.top_left - old_pos;
//        surface->resize(info.restore_rect.size);
//        break;
//
//    case mir_surface_state_maximized:
//        movement = display_area.top_left - old_pos;
//        surface->resize(display_area.size);
//        break;
//
//    case mir_surface_state_horizmaximized:
//        movement = Point{display_area.top_left.x, info.restore_rect.top_left.y} - old_pos;
//        surface->resize({display_area.size.width, info.restore_rect.size.height});
//        break;
//
//    case mir_surface_state_vertmaximized:
//        movement = Point{info.restore_rect.top_left.x, display_area.top_left.y} - old_pos;
//        surface->resize({info.restore_rect.size.width, display_area.size.height});
//        break;
//
//    case mir_surface_state_fullscreen:
//    {
//        Rectangle rect{old_pos, surface->size()};
//        display_layout->size_to_output(rect);
//        movement = rect.top_left - old_pos;
//        surface->resize(rect.size);
//        break;
//    }
//
//    case mir_surface_state_hidden:
//    case mir_surface_state_minimized:
//        surface->hide();
//        return info.state = value;
//
//    default:
//        break;
//    }
//
//    // TODO It is rather simplistic to move a tree WRT the top_left of the root
//    // TODO when resizing. But for more sophistication we would need to encode
//    // TODO some sensible layout rules.
//    move_tree(surface, movement);
//
//    return info.state = value;
}

//void CanonicalWindowManagerPolicy::drag(Point cursor)
//{
//    select_active_surface(tools->surface_at(old_cursor));
//    drag(active_surface(), cursor, old_cursor, display_area);
//    old_cursor = cursor;
//}

bool CanonicalWindowManagerPolicy::handle_keyboard_event(MirKeyboardEvent const* /*event*/)
{
//    auto const action = mir_keyboard_event_action(event);
//    auto const scan_code = mir_keyboard_event_scan_code(event);
//    auto const modifiers = mir_keyboard_event_modifiers(event) & modifier_mask;
//
//    if (action == mir_keyboard_action_down && scan_code == KEY_F11)
//    {
//        switch (modifiers)
//        {
//        case mir_input_event_modifier_alt:
//            toggle(mir_surface_state_maximized);
//            return true;
//
//        case mir_input_event_modifier_shift:
//            toggle(mir_surface_state_vertmaximized);
//            return true;
//
//        case mir_input_event_modifier_ctrl:
//            toggle(mir_surface_state_horizmaximized);
//            return true;
//
//        default:
//            break;
//        }
//    }
//    else if (action == mir_keyboard_action_down && scan_code == KEY_F4)
//    {
//        if (auto const session = tools->focused_session())
//        {
//            switch (modifiers)
//            {
//            case mir_input_event_modifier_alt:
//                kill(session->process_id(), SIGTERM);
//                return true;
//
//            case mir_input_event_modifier_ctrl:
//                if (auto const surf = session->default_surface())
//                {
//                    surf->request_client_surface_close();
//                    return true;
//                }
//
//            default:
//                break;
//            }
//        }
//    }
//    else if (action == mir_keyboard_action_down &&
//            modifiers == mir_input_event_modifier_alt &&
//            scan_code == KEY_TAB)
//    {
//        tools->focus_next_session();
//        if (auto const surface = tools->focused_surface())
//            select_active_surface(surface);
//
//        return true;
//    }
//    else if (action == mir_keyboard_action_down &&
//            modifiers == mir_input_event_modifier_alt &&
//            scan_code == KEY_GRAVE)
//    {
//        if (auto const prev = tools->focused_surface())
//        {
//            if (auto const app = tools->focused_session())
//                select_active_surface(app->surface_after(prev));
//        }
//
//        return true;
//    }

    return false;
}

bool CanonicalWindowManagerPolicy::handle_touch_event(MirTouchEvent const* /*event*/)
{
//    auto const count = mir_touch_event_point_count(event);
//
//    long total_x = 0;
//    long total_y = 0;
//
//    for (auto i = 0U; i != count; ++i)
//    {
//        total_x += mir_touch_event_axis_value(event, i, mir_touch_axis_x);
//        total_y += mir_touch_event_axis_value(event, i, mir_touch_axis_y);
//    }
//
//    Point const cursor{total_x/count, total_y/count};
//
//    bool is_drag = true;
//    for (auto i = 0U; i != count; ++i)
//    {
//        switch (mir_touch_event_action(event, i))
//        {
//        case mir_touch_action_up:
//            return false;
//
//        case mir_touch_action_down:
//            is_drag = false;
//
//        case mir_touch_action_change:
//            continue;
//        }
//    }
//
//    if (is_drag && count == 3)
//    {
//        drag(cursor);
//        return true;
//    }
//    else
//    {
//        click(cursor);
//        return false;
//    }
    return false;
}

bool CanonicalWindowManagerPolicy::handle_pointer_event(MirPointerEvent const* /*event*/)
{
//    auto const action = mir_pointer_event_action(event);
//    auto const modifiers = mir_pointer_event_modifiers(event) & modifier_mask;
//    Point const cursor{
//        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
//        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};
//
//    if (action == mir_pointer_action_button_down)
//    {
//        click(cursor);
//        return false;
//    }
//    else if (action == mir_pointer_action_motion &&
//             modifiers == mir_input_event_modifier_alt)
//    {
//        if (mir_pointer_event_button_state(event, mir_pointer_button_primary))
//        {
//            drag(cursor);
//            return true;
//        }
//
//        if (mir_pointer_event_button_state(event, mir_pointer_button_tertiary))
//        {
//            resize(cursor);
//            return true;
//        }
//    }

    return false;
}

//void CanonicalWindowManagerPolicy::toggle(MirSurfaceState state)
//{
//    if (auto const surface = active_surface())
//    {
//        auto& info = tools->info_for(surface);
//
//        if (info.state == state)
//            state = mir_surface_state_restored;
//
//        auto const value = handle_set_state(surface, MirSurfaceState(state));
//        surface->configure(mir_surface_attrib_state, value);
//    }
//}
//
//void CanonicalWindowManagerPolicy::select_active_surface(std::shared_ptr<ms::Surface> const& surface)
//{
//    if (!surface)
//    {
//        if (active_surface_.lock())
//            tools->set_focus_to({}, {});
//
//        active_surface_.reset();
//        return;
//    }
//
//    auto const& info_for = tools->info_for(surface);
//
//    if (info_for.can_be_active())
//    {
//        tools->set_focus_to(info_for.session.lock(), surface);
//        raise_tree(surface);
//        active_surface_ = surface;
//    }
//    else
//    {
//        // Cannot have input focus - try the parent
//        if (auto const parent = info_for.parent.lock())
//            select_active_surface(parent);
//    }
//}

//auto CanonicalWindowManagerPolicy::active_surface() const
//-> std::shared_ptr<ms::Surface>
//{
//    if (auto const surface = active_surface_.lock())
//        return surface;
//
//    if (auto const session = tools->focused_session())
//    {
//        if (auto const surface = session->default_surface())
//            return surface;
//    }
//
//    return std::shared_ptr<ms::Surface>{};
//}

//bool CanonicalWindowManagerPolicy::resize(std::shared_ptr<ms::Surface> const& surface, Point cursor, Point old_cursor, Rectangle bounds)
//{
//    if (!surface || !surface->input_area_contains(old_cursor))
//        return false;
//
//    auto const top_left = surface->top_left();
//    Rectangle const old_pos{top_left, surface->size()};
//
//    auto anchor = top_left;
//
//    for (auto const& corner : {
//        old_pos.top_right(),
//        old_pos.bottom_left(),
//        old_pos.bottom_right()})
//    {
//        if ((old_cursor - anchor).length_squared() <
//            (old_cursor - corner).length_squared())
//        {
//            anchor = corner;
//        }
//    }
//
//    bool const left_resize = anchor.x != top_left.x;
//    bool const top_resize  = anchor.y != top_left.y;
//    int const x_sign = left_resize? -1 : 1;
//    int const y_sign = top_resize?  -1 : 1;
//
//    auto const delta = cursor-old_cursor;
//
//    Size new_size{old_pos.size.width + x_sign*delta.dx, old_pos.size.height + y_sign*delta.dy};
//
//    Point new_pos = top_left + left_resize*delta.dx + top_resize*delta.dy;
//
//
//    auto const& surface_info = tools->info_for(surface);
//
//    surface_info.constrain_resize(surface, new_pos, new_size, left_resize, top_resize, bounds);
//
//    apply_resize(surface, new_pos, new_size);
//
//    return true;
//}

//void msh::QtmirSurfaceInfo::constrain_resize(
//    std::shared_ptr<ms::Surface> const& surface,
//    Point& requested_pos,
//    Size& requested_size,
//    bool const left_resize,
//    bool const top_resize,
//    Rectangle const& /*bounds*/) const
//{
//    Point new_pos = requested_pos;
//    Size new_size = requested_size;
//
//    if (min_aspect.is_set())
//    {
//        auto const ar = min_aspect.value();
//
//        auto const error = new_size.height.as_int()*long(ar.width) - new_size.width.as_int()*long(ar.height);
//
//        if (error > 0)
//        {
//            // Add (denominator-1) to numerator to ensure rounding up
//            auto const width_correction  = (error+(ar.height-1))/ar.height;
//            auto const height_correction = (error+(ar.width-1))/ar.width;
//
//            if (width_correction < height_correction)
//            {
//                new_size.width = new_size.width + DeltaX(width_correction);
//            }
//            else
//            {
//                new_size.height = new_size.height - DeltaY(height_correction);
//            }
//        }
//    }
//
//    if (max_aspect.is_set())
//    {
//        auto const ar = max_aspect.value();
//
//        auto const error = new_size.width.as_int()*long(ar.height) - new_size.height.as_int()*long(ar.width);
//
//        if (error > 0)
//        {
//            // Add (denominator-1) to numerator to ensure rounding up
//            auto const height_correction = (error+(ar.width-1))/ar.width;
//            auto const width_correction  = (error+(ar.height-1))/ar.height;
//
//            if (width_correction < height_correction)
//            {
//                new_size.width = new_size.width - DeltaX(width_correction);
//            }
//            else
//            {
//                new_size.height = new_size.height + DeltaY(height_correction);
//            }
//        }
//    }
//
//    if (min_width > new_size.width)
//        new_size.width = min_width;
//
//    if (min_height > new_size.height)
//        new_size.height = min_height;
//
//    if (max_width < new_size.width)
//        new_size.width = max_width;
//
//    if (max_height < new_size.height)
//        new_size.height = max_height;
//
//    if (width_inc.is_set())
//    {
//        auto const width = new_size.width.as_int() - min_width.as_int();
//        auto inc = width_inc.value().as_int();
//        if (width % inc)
//            new_size.width = min_width + DeltaX{inc*(((2L*width + inc)/2)/inc)};
//    }
//
//    if (height_inc.is_set())
//    {
//        auto const height = new_size.height.as_int() - min_height.as_int();
//        auto inc = height_inc.value().as_int();
//        if (height % inc)
//            new_size.height = min_height + DeltaY{inc*(((2L*height + inc)/2)/inc)};
//    }
//
//    if (left_resize)
//        new_pos.x += new_size.width - requested_size.width;
//
//    if (top_resize)
//        new_pos.y += new_size.height - requested_size.height;
//
//    // placeholder - constrain onscreen
//
//    switch (state)
//    {
//    case mir_surface_state_restored:
//        break;
//
//    // "A vertically maximised surface is anchored to the top and bottom of
//    // the available workspace and can have any width."
//    case mir_surface_state_vertmaximized:
//        new_pos.y = surface->top_left().y;
//        new_size.height = surface->size().height;
//        break;
//
//    // "A horizontally maximised surface is anchored to the left and right of
//    // the available workspace and can have any height"
//    case mir_surface_state_horizmaximized:
//        new_pos.x = surface->top_left().x;
//        new_size.width = surface->size().width;
//        break;
//
//    // "A maximised surface is anchored to the top, bottom, left and right of the
//    // available workspace. For example, if the launcher is always-visible then
//    // the left-edge of the surface is anchored to the right-edge of the launcher."
//    case mir_surface_state_maximized:
//    default:
//        new_pos.x = surface->top_left().x;
//        new_pos.y = surface->top_left().y;
//        new_size.width = surface->size().width;
//        new_size.height = surface->size().height;
//        break;
//    }
//
//    requested_pos = new_pos;
//    requested_size = new_size;
//}

//void CanonicalWindowManagerPolicy::apply_resize(
//    std::shared_ptr<ms::Surface> const& surface,
//    Point const& new_pos,
//    Size const& new_size) const
//{
//    surface->resize(new_size);
//
//    // TODO It is rather simplistic to move a tree WRT the top_left of the root
//    // TODO when resizing. But for more sophistication we would need to encode
//    // TODO some sensible layout rules.
//    move_tree(surface, new_pos-surface->top_left());
//}

//bool CanonicalWindowManagerPolicy::drag(std::shared_ptr<ms::Surface> surface, Point to, Point from, Rectangle /*bounds*/)
//{
//    if (!surface)
//        return false;
//
//    if (!surface->input_area_contains(from))
//        return false;
//
//    auto movement = to - from;
//
//    // placeholder - constrain onscreen
//
//    switch (tools->info_for(surface).state)
//    {
//    case mir_surface_state_restored:
//        break;
//
//    // "A vertically maximised surface is anchored to the top and bottom of
//    // the available workspace and can have any width."
//    case mir_surface_state_vertmaximized:
//        movement.dy = DeltaY(0);
//        break;
//
//    // "A horizontally maximised surface is anchored to the left and right of
//    // the available workspace and can have any height"
//    case mir_surface_state_horizmaximized:
//        movement.dx = DeltaX(0);
//        break;
//
//    // "A maximised surface is anchored to the top, bottom, left and right of the
//    // available workspace. For example, if the launcher is always-visible then
//    // the left-edge of the surface is anchored to the right-edge of the launcher."
//    case mir_surface_state_maximized:
//    case mir_surface_state_fullscreen:
//    default:
//        return true;
//    }
//
//    move_tree(surface, movement);
//
//    return true;
//}

//void CanonicalWindowManagerPolicy::move_tree(std::shared_ptr<ms::Surface> const& root, Displacement movement) const
//{
//    root->move_to(root->top_left() + movement);
//
//    for (auto const& child: tools->info_for(root).children)
//    {
//        move_tree(child.lock(), movement);
//    }
//}

//void CanonicalWindowManagerPolicy::raise_tree(std::shared_ptr<scene::Surface> const& root) const
//{
//    SurfaceSet surfaces;
//    std::function<void(std::weak_ptr<scene::Surface> const& surface)> const add_children =
//        [&,this](std::weak_ptr<scene::Surface> const& surface)
//        {
//            auto const& info_for = tools->info_for(surface);
//            surfaces.insert(begin(info_for.children), end(info_for.children));
//            for (auto const& child : info_for.children)
//                add_children(child);
//        };
//
//    surfaces.insert(root);
//    add_children(root);
//
//    tools->raise(surfaces);
//}

MirWindowManagerImpl::MirWindowManagerImpl(const std::shared_ptr<mir::shell::DisplayLayout> &displayLayout) :
    m_displayLayout{displayLayout}
{
    qCDebug(QTMIR_MIR_MESSAGES) << "MirWindowManagerImpl::MirWindowManagerImpl";
}

void MirWindowManagerImpl::add_session(std::shared_ptr<ms::Session> const& /*session*/)
{
}

void MirWindowManagerImpl::remove_session(std::shared_ptr<ms::Session> const& /*session*/)
{
}

auto MirWindowManagerImpl::add_surface(
    std::shared_ptr<ms::Session> const& session,
    ms::SurfaceCreationParameters const& requestParameters,
    std::function<mir::frontend::SurfaceId(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params)> const& build)
-> mir::frontend::SurfaceId
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

    qCDebug(QTMIR_MIR_MESSAGES) << "MirWindowManagerImpl::add_surface(): size requested ("
                                << requestParameters.size.width.as_int() << "," << requestParameters.size.height.as_int() << ") and placed ("
                                << placedParameters.size.width.as_int() << "," << placedParameters.size.height.as_int() << ")";

    tracepoint(qtmirserver, surfacePlacementEnd);

    return build(session, placedParameters);
}

void MirWindowManagerImpl::remove_surface(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::weak_ptr<ms::Surface> const& /*surface*/)
{
}

void MirWindowManagerImpl::add_display(mir::geometry::Rectangle const& /*area*/)
{
}

void MirWindowManagerImpl::remove_display(mir::geometry::Rectangle const& /*area*/)
{
}

bool MirWindowManagerImpl::handle_keyboard_event(MirKeyboardEvent const* /*event*/)
{
    return false;
}

bool MirWindowManagerImpl::handle_touch_event(MirTouchEvent const* /*event*/)
{
    return false;
}

bool MirWindowManagerImpl::handle_pointer_event(MirPointerEvent const* /*event*/)
{
    return false;
}

int MirWindowManagerImpl::set_surface_attribute(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& surface,
    MirSurfaceAttrib attrib,
    int value)
{
    return surface->configure(attrib, value);
}

void MirWindowManagerImpl::modify_surface(const std::shared_ptr<mir::scene::Session>&, const std::shared_ptr<mir::scene::Surface>&, const mir::shell::SurfaceSpecification&)
{
    // TODO support surface modifications
}

auto MirWindowManager::create(const std::shared_ptr<mir::shell::DisplayLayout> &displayLayout)
        -> std::unique_ptr<MirWindowManager>
{
    return std::make_unique<MirWindowManagerImpl>(displayLayout);
}
