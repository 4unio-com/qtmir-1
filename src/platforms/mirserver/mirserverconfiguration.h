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
 */

#ifndef MIRSERVERCONFIGURATION_H
#define MIRSERVERCONFIGURATION_H

#include <QObject>
#include <mir/server.h>

class QtEventFeeder;
class SessionListener;
class SessionAuthorizer;
class SurfaceConfigurator;
class PromptSessionListener;

class MirServerConfiguration : public QObject, protected mir::Server
{
    Q_OBJECT

    Q_PROPERTY(SessionAuthorizer* sessionAuthorizer READ sessionAuthorizer CONSTANT)
    Q_PROPERTY(SessionListener* sessionListener READ sessionListener CONSTANT)
    Q_PROPERTY(SurfaceConfigurator* surfaceConfigurator READ surfaceConfigurator CONSTANT)
    Q_PROPERTY(PromptSessionListener* promptSessionListener READ promptSessionListener CONSTANT)

public:
    MirServerConfiguration(int argc, char const* argv[], QObject* parent = 0);
    ~MirServerConfiguration() = default;

    /* mir specific */
    using mir::Server::the_session_listener;
    using mir::Server::the_prompt_session_listener;
    using mir::Server::the_surface_configurator;
    using mir::Server::the_session_authorizer;
    using mir::Server::the_gl_config;
    using mir::Server::the_display;
    using mir::Server::run;
    using mir::Server::stop;
    using mir::Server::set_terminator;
    using mir::Server::the_prompt_session_manager;

    /* qt specific */
    // getters
    SessionAuthorizer *sessionAuthorizer();
    SessionListener *sessionListener();
    PromptSessionListener *promptSessionListener();
    SurfaceConfigurator *surfaceConfigurator();

private:
    std::shared_ptr<QtEventFeeder> m_qtEventFeeder;
};

#endif // MIRSERVERCONFIGURATION_H
