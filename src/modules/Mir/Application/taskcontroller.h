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
 *
 * Authored by: Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */

#ifndef TASKCONTROLLER_H
#define TASKCONTROLLER_H

#include <QObject>

#include "application.h"
#include "applicationcontroller.h"
#include "processcontroller.h"

extern "C" {
    #include "ubuntu-app-launch.h"
}

namespace qtmir
{

class TaskController : public QObject
{
    Q_OBJECT
public:
    TaskController(
            QObject *parent,
            const QSharedPointer<ApplicationController> &appController,
            const QSharedPointer<ProcessController> &processController = QSharedPointer<ProcessController>(new ProcessController()));
    ~TaskController();

    bool start(const QString &appId, const QStringList &args);
    bool stop(const QString &appId);

    bool suspend(const QString &appId);
    bool resume(const QString &appId);

    bool appIdHasProcessId(const QString &appId, const quint64 pid);
    QFileInfo findDesktopFileForAppId(const QString &appId) const;

Q_SIGNALS:
    void processStarting(const QString &appId);
    void processStopped(const QString &appId);
    void processFailed(const QString &appId, const bool duringStartup);
    void requestFocus(const QString &appId);
    void requestResume(const QString &appId);

private Q_SLOTS:
    void onApplicationStarted(const QString &id);
    void onApplicationFocusRequest(const QString &id);
    void onApplicationResumeRequest(const QString &id);

    void onApplicationError(const QString &id, ApplicationController::Error error);

private:
    QSharedPointer<ApplicationController> m_appController;
    QSharedPointer<ProcessController> m_processController;
};

} // namespace qtmir

#endif // TASKCONTROLLER_H
