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

// Qt
#include <QObject>
#include <QCoreApplication>
#include <QDebug>

#include <mir/main_loop.h>

// local
#include "qmirserver.h"
#include "screencontroller.h"
#include "screen.h"


void MirServerWorker::run()
{
    auto const main_loop = server->the_main_loop();
    // By enqueuing the notification code in the main loop, we are
    // ensuring that the server has really and fully started before
    // leaving wait_for_startup().
    main_loop->enqueue(
        this,
        [&]
        {
            std::lock_guard<std::mutex> lock(mutex);
            mir_running = true;
            started_cv.notify_one();
        });

    server->run();
    Q_EMIT stopped();
}

bool MirServerWorker::wait_for_mir_startup()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    started_cv.wait_for(lock, std::chrono::seconds{10}, [&]{ return mir_running; });
    return mir_running;
}

QMirServer::QMirServer(int argc, const char *argv[], QObject *parent)
    : QObject(parent)
    , m_screenController(new ScreenController())
    , m_server(new MirServer(argc, argv, m_screenController))
    , m_mirServer(new MirServerWorker(m_server))
{
    m_mirServer->moveToThread(&m_mirThread);

    connect(this, &QMirServer::runServer, m_mirServer.data(), &MirServerWorker::run);
    connect(this, &QMirServer::stopServer, m_mirServer.data(), &MirServerWorker::stop);

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QMirServer::shutDownMirServer);
    connect(m_mirServer.data(), &MirServerWorker::stopped, this, &QMirServer::shutDownQApplication, Qt::DirectConnection); // since no event loop
}

QMirServer::~QMirServer()
{
    shutDownMirServer();
}

void QMirServer::run()
{
    m_mirThread.start(QThread::HighPriority);
    Q_EMIT runServer();

    if (!m_mirServer->wait_for_mir_startup())
    {
        qCritical() << "ERROR: QMirServer - Mir failed to start";
        QCoreApplication::quit();
    }
    m_screenController->init();
}

void QMirServer::stop()
{
    shutDownMirServer();
}

void QMirServer::shutDownMirServer()
{
    if (m_mirThread.isRunning()) {
        delete m_screenController;
        m_mirServer->stop();
        m_mirThread.wait();
    }
}

void QMirServer::shutDownQApplication()
{
    if (m_mirThread.isRunning()) {
        m_mirThread.quit();
    }

    // if unexpected mir server stop, better quit the QApplication
    if (!QCoreApplication::closingDown()) {
        QCoreApplication::quit();
    }
}

ScreenController* QMirServer::screenController() const
{
    return m_screenController;
}
