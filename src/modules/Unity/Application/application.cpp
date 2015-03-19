/*
 * Copyright (C) 2013-2014 Canonical, Ltd.
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

// local
#include "application.h"
#include "application_manager.h"
#include "desktopfilereader.h"
#include "session.h"
#include "sharedwakelock.h"
#include "taskcontroller.h"

// common
#include <debughelpers.h>

// QPA mirserver
#include "logging.h"

// mir
#include <mir/scene/session.h>
#include <mir/scene/snapshot.h>

namespace ms = mir::scene;

namespace qtmir
{

Application::Application(const QSharedPointer<TaskController>& taskController,
                         const QSharedPointer<SharedWakelock>& sharedWakelock,
                         DesktopFileReader *desktopFileReader,
                         State state,
                         const QStringList &arguments,
                         ApplicationManager *parent)
    : ApplicationInfoInterface(desktopFileReader->appId(), parent)
    , m_taskController(taskController)
    , m_sharedWakelock(sharedWakelock)
    , m_desktopData(desktopFileReader)
    , m_pid(0)
    , m_stage((m_desktopData->stageHint() == "SideStage") ? Application::SideStage : Application::MainStage)
    , m_state(state)
    , m_focused(false)
    , m_canBeResumed(true)
    , m_arguments(arguments)
    , m_session(nullptr)
{
    qCDebug(QTMIR_APPLICATIONS) << "Application::Application - appId=" << desktopFileReader->appId() << "state=" << state;

    // FIXME(greyback) need to save long appId internally until ubuntu-app-launch can hide it from us
    m_longAppId = desktopFileReader->file().remove(QRegExp(".desktop$")).split('/').last();

    // FIXME: This is a hack. Remove once we have a real implementation for knowing the supported
    // orientations of an app
    m_supportedOrientations = PortraitOrientation
        | LandscapeOrientation
        | InvertedPortraitOrientation
        | InvertedLandscapeOrientation;
}

Application::~Application()
{
    qCDebug(QTMIR_APPLICATIONS) << "Application::~Application";

    delete m_session;
    delete m_desktopData;
}

bool Application::isValid() const
{
    return m_desktopData->loaded();
}

QString Application::desktopFile() const
{
    return m_desktopData->file();
}

QString Application::appId() const
{
    return m_desktopData->appId();
}

QString Application::name() const
{
    return m_desktopData->name();
}

QString Application::comment() const
{
    return m_desktopData->comment();
}

QUrl Application::icon() const
{
    QString iconString = m_desktopData->icon();
    QString pathString = m_desktopData->path();

    if (QFileInfo(iconString).exists()) {
        return QUrl(iconString);
    } else if (QFileInfo(pathString + '/' + iconString).exists()) {
        return QUrl(pathString + '/' + iconString);
    } else {
        return QUrl("image://theme/" + iconString);
    }
}

QString Application::splashTitle() const
{
    return m_desktopData->splashTitle();
}

QUrl Application::splashImage() const
{
    if (m_desktopData->splashImage().isEmpty()) {
        return QUrl();
    } else {
        QFileInfo imageFileInfo(m_desktopData->path(), m_desktopData->splashImage());
        if (imageFileInfo.exists()) {
            return QUrl::fromLocalFile(imageFileInfo.canonicalFilePath());
        } else {
            qCWarning(QTMIR_APPLICATIONS)
                << QString("Application(%1).splashImage file does not exist: \"%2\". Ignoring it.")
                    .arg(appId()).arg(imageFileInfo.absoluteFilePath());

            return QUrl();
        }
    }
}

QColor Application::colorFromString(const QString &colorString, const char *colorName) const
{
    // NB: If a colour which is not fully opaque is specified in the desktop file, it will
    //     be ignored and the default colour will be used instead.
    QColor color;

    if (colorString.isEmpty()) {
        color.setRgba(qRgba(0, 0, 0, 0));
    } else {
        color.setNamedColor(colorString);

        if (color.isValid()) {
            // Force a fully opaque color.
            color.setAlpha(255);
        } else {
            color.setRgba(qRgba(0, 0, 0, 0));
            qCWarning(QTMIR_APPLICATIONS) << QString("Invalid %1: \"%2\"")
                .arg(colorName).arg(colorString);
        }
    }

    return color;
}

bool Application::splashShowHeader() const
{
    QString showHeader = m_desktopData->splashShowHeader();
    if (showHeader.toLower() == "true") {
        return true;
    } else {
        return false;
    }
}

QColor Application::splashColor() const
{
    QString colorStr = m_desktopData->splashColor();
    return colorFromString(colorStr, "splashColor");
}

QColor Application::splashColorHeader() const
{
    QString colorStr = m_desktopData->splashColorHeader();
    return colorFromString(colorStr, "splashColorHeader");
}

QColor Application::splashColorFooter() const
{
    QString colorStr = m_desktopData->splashColorFooter();
    return colorFromString(colorStr, "splashColorFooter");
}

QString Application::exec() const
{
    return m_desktopData->exec();
}

Application::Stage Application::stage() const
{
    return m_stage;
}

Application::Stages Application::supportedStages() const
{
    return m_supportedStages;
}

Application::State Application::state() const
{
    return m_state;
}

bool Application::focused() const
{
    return m_focused;
}

bool Application::fullscreen() const
{
    return m_session ? m_session->fullscreen() : false;
}

bool Application::canBeResumed() const
{
    return m_canBeResumed;
}

void Application::setCanBeResumed(const bool resume)
{
    m_canBeResumed = resume;
}

pid_t Application::pid() const
{
    return m_pid;
}

void Application::setPid(pid_t pid)
{
    m_pid = pid;
}

void Application::setArguments(const QStringList arguments)
{
    m_arguments = arguments;
}

void Application::setSession(Session *newSession)
{
    qCDebug(QTMIR_APPLICATIONS) << "Application::setSession - appId=" << appId() << "session=" << newSession;

    if (newSession == m_session)
        return;

    if (m_session) {
        m_session->disconnect(this);
        m_session->setApplication(nullptr);
        m_session->setParent(nullptr);
    }

    bool oldFullscreen = fullscreen();
    m_session = newSession;

    if (m_session) {
        m_session->setParent(this);
        m_session->setApplication(this);
        m_session->setState(state());

        connect(m_session, &SessionInterface::suspended, this, &Application::onSessionSuspended);
        connect(m_session, &SessionInterface::resumed, this, &Application::onSessionResumed);
        connect(m_session, &SessionInterface::fullscreenChanged, this, &Application::fullscreenChanged);

        if (oldFullscreen != fullscreen())
            Q_EMIT fullscreenChanged(fullscreen());
    }

    Q_EMIT sessionChanged(m_session);
}

void Application::setStage(Application::Stage stage)
{
    qCDebug(QTMIR_APPLICATIONS) << "Application::setStage - appId=" << appId() << "stage=" << stage;

    if (m_stage != stage) {
        if (stage | m_supportedStages) {
            return;
        }

        m_stage = stage;
        Q_EMIT stageChanged(stage);
    }
}

void Application::setState(Application::State state)
{
    qCDebug(QTMIR_APPLICATIONS) << "Application::setState - appId=" << appId() << "state=" << applicationStateToStr(state);
    if (m_state != state) {
        if (session()) {
            session()->setState((Session::State)state);
        } else {
            // If we have have no session, we may have to respawn it.
            switch (state)
            {
            case Session::State::Running:
                if (m_state == Session::State::Stopped) {
                    respawn();
                    state = Session::State::Starting;
                }
                break;
            default:
                break;
            }
        }
        m_state = state;
        Q_EMIT stateChanged(state);
    }
}

void Application::setFocused(bool focused)
{
    qCDebug(QTMIR_APPLICATIONS) << "Application::setFocused - appId=" << appId() << "focused=" << focused;
    if (focused) {
        holdWakelock(true);
    }

    if (m_focused != focused) {
        m_focused = focused;
        Q_EMIT focusedChanged(focused);
    }
}

void Application::onSessionSuspended()
{
    qCDebug(QTMIR_APPLICATIONS) << "Application::onSessionSuspended - appId=" << appId();
    m_taskController->suspend(longAppId());
    holdWakelock(false);
}

void Application::onSessionResumed()
{
    qCDebug(QTMIR_APPLICATIONS) << "Application::onSessionResumed - appId=" << appId();
    holdWakelock(true);
    m_taskController->resume(longAppId());
}

void Application::respawn()
{
    qCDebug(QTMIR_APPLICATIONS) << "Application::respawn - appId=" << appId();
    holdWakelock(true);
    m_taskController->start(appId(), m_arguments);
}

QString Application::longAppId() const
{
    return m_longAppId;
}

Application::SupportedOrientations Application::supportedOrientations() const
{
    return m_supportedOrientations;
}

Session* Application::session() const
{
    return m_session;
}

void Application::holdWakelock(bool enable) const
{
    if (appId() == "unity8-dash")
        return;

    if (enable) {
        m_sharedWakelock->acquire(this);
    } else {
        m_sharedWakelock->release(this);
    }
}

} // namespace qtmir
