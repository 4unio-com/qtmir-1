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

#include <Unity/Application/session_interface.h>

#ifndef QTMIR_FAKE_SESSION_H
#define QTMIR_FAKE_SESSION_H

namespace qtmir {

class FakeSession : public SessionInterface
{
    Q_OBJECT
    
public:
    FakeSession();
    virtual ~FakeSession();

    // For QML use
    void release() override;

    QString name() const override;
    unity::shell::application::ApplicationInfoInterface* application() const override;
    MirSurfaceInterface* surface() const override;
    SessionInterface* parentSession() const override;
    SessionModel* childSessions() const override;
    State state() const override;
    bool fullscreen() const override;
    bool live() const override;

    std::shared_ptr<mir::scene::Session> session() const override;

    // For MirSurfaceItem and MirSurfaceManager use

    void setSurface(MirSurfaceInterface*) override;

    // For Application use

    void setApplication(unity::shell::application::ApplicationInfoInterface* app) override;
    void suspend() override;
    void resume() override;
    void stop() override;

    // For SessionManager use

    void addChildSession(SessionInterface*) override;
    void insertChildSession(uint, SessionInterface*) override;
    void removeChildSession(SessionInterface*) override;
    void foreachChildSession(std::function<void(SessionInterface* session)>) const override;

    std::shared_ptr<mir::scene::PromptSession> activePromptSession() const override;
    void foreachPromptSession(std::function<void(const std::shared_ptr<mir::scene::PromptSession>&)>) const override;

    void setFullscreen(bool) override;
    void setLive(const bool) override;
    void appendPromptSession(const std::shared_ptr<mir::scene::PromptSession>&) override;
    void removePromptSession(const std::shared_ptr<mir::scene::PromptSession>&) override;

    // For tests

    void setState(State state);

private:
    unity::shell::application::ApplicationInfoInterface* m_application;
    State m_state;
};

} // namespace qtmi

#endif // QTMIR_FAKE_SESSION_H
