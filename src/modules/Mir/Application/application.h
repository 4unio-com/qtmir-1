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

#ifndef APPLICATION_H
#define APPLICATION_H

// std
#include <memory>

//Qt
#include <QtCore/QtCore>
#include <QImage>

// Unity API
#include <unity/shell/application/ApplicationInfoInterface.h>

class QImage;
class DesktopFileReader;
class TaskController;
namespace mir { namespace scene { class Session; }}

class Application : public unity::shell::application::ApplicationInfoInterface {
    Q_OBJECT
    Q_PROPERTY(QString desktopFile READ desktopFile CONSTANT)
    Q_PROPERTY(QString exec READ exec CONSTANT)
    Q_PROPERTY(bool fullscreen READ fullscreen NOTIFY fullscreenChanged)

public:
    Application(const QString &appId, State state, const QStringList &arguments, QObject *parent = 0);
    Application(DesktopFileReader *desktopFileReader, State state, const QStringList &arguments, QObject *parent = 0);
    virtual ~Application();

    // ApplicationInfoInterface
    QString appId() const override;
    QString name() const override;
    QString comment() const override;
    QUrl icon() const override;
    Stage stage() const override;
    Stages supportedStages() const override;
    State state() const override;
    bool focused() const override;

    bool setStage(Stage stage) override;

    bool isValid() const;
    QString desktopFile() const;
    QString exec() const;
    bool fullscreen() const;

public Q_SLOTS:
    void suspend();
    void resume();
    void respawn();

Q_SIGNALS:
    void fullscreenChanged();

private:
    pid_t pid() const;
    void setPid(pid_t pid);
    void setState(State state);
    void setFocused(bool focus);
    void setFullscreen(bool fullscreen);
    std::shared_ptr<mir::scene::Session> session() const;
    void setSession(const std::shared_ptr<mir::scene::Session>& session);
    void setSessionName(const QString& name);

    DesktopFileReader* m_desktopData;
    qint64 m_pid;
    Stage m_stage;
    Stages m_supportedStages;
    State m_state;
    bool m_focused;
    QUrl m_screenshot;
    QImage m_screenshotImage;
    bool m_fullscreen;
    std::shared_ptr<mir::scene::Session> m_session;
    QString m_sessionName;
    QStringList m_arguments;
    QTimer* m_suspendTimer;

    friend class ApplicationManager;
    friend class ApplicationScreenshotProvider;
    friend class MirSurfaceManager;
};

Q_DECLARE_METATYPE(Application*)

#endif  // APPLICATION_H
