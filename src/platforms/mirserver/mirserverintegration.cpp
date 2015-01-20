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
 * Authors: Gerry Boland <gerry.boland@canonical.com>
 *          Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#include "mirserverintegration.h"

#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qgenericunixservices_p.h>

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformaccessibility.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <QGuiApplication>
#include <QStringList>
#include <QOpenGLContext>

#if QT_VERSION < QT_VERSION_CHECK(5, 2, 0)
#include <private/qguiapplication_p.h>
#endif

#include <QDebug>

// Mir
#include <mir/graphics/display.h>
#include <mir/graphics/display_buffer.h>

// std
#include <csignal>

// local
#include "clipboard.h"
#include "miropenglcontext.h"
#include "nativeinterface.h"
#include "offscreensurface.h"
#include "qmirserver.h"
#include "screen.h"
#include "screencontroller.h"
#include "screenwindow.h"
#include "services.h"
#include "ubuntutheme.h"

namespace mg = mir::graphics;
using qtmir::Clipboard;

MirServerIntegration::MirServerIntegration()
    : QObject()
    , m_accessibility(new QPlatformAccessibility())
    , m_fontDb(new QGenericUnixFontDatabase())
    , m_services(new Services)
#if QT_VERSION < QT_VERSION_CHECK(5, 2, 0)
    , m_eventDispatcher(createUnixEventDispatcher())
#endif
    , m_qmirServer(nullptr)
    , m_nativeInterface(nullptr)
    , m_clipboard(new Clipboard)
{
    // Start Mir server only once Qt has initialized its event dispatcher, see initialize()

    QStringList args = QCoreApplication::arguments();
    // convert arguments back into argc-argv form that Mir wants
    char **argv;
    argv = new char*[args.size() + 1];
    for (int i = 0; i < args.size(); i++) {
        argv[i] = new char[strlen(args.at(i).toStdString().c_str())+1];
        memcpy(argv[i], args.at(i).toStdString().c_str(), strlen(args.at(i).toStdString().c_str())+1);
    }
    argv[args.size()] = '\0';

    // For access to sensors, qtmir uses qtubuntu-sensors. qtubuntu-sensors reads the
    // UBUNTU_PLATFORM_API_BACKEND variable to decide if to load a valid sensor backend or not.
    // For it to function we need to ensure a valid backend has been specified
    if (qEnvironmentVariableIsEmpty("UBUNTU_PLATFORM_API_BACKEND")) {
        if (qgetenv("DESKTOP_SESSION").contains("mir")) {
            qputenv("UBUNTU_PLATFORM_API_BACKEND", "desktop_mirclient");
        } else {
            qputenv("UBUNTU_PLATFORM_API_BACKEND", "touch_mirclient");
        }
    }

    m_mirServer = QSharedPointer<MirServer>(
                      new MirServer(args.length(), const_cast<const char**>(argv)));

#if QT_VERSION < QT_VERSION_CHECK(5, 2, 0)
    QGuiApplicationPrivate::instance()->setEventDispatcher(eventDispatcher_);
    initialize();
#endif

    m_inputContext = QPlatformInputContextFactory::create();
}

MirServerIntegration::~MirServerIntegration()
{
    delete m_nativeInterface;
    delete m_qmirServer;
}

bool MirServerIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    case SharedGraphicsCache: return true;
    case BufferQueueingOpenGL: return true;
    case MultipleWindows: return true; // multi-monitor support
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    case WindowManagement: return false; // platform has no WM, as this implements the WM!
    case NonFullScreenWindows: return false;
#endif
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *MirServerIntegration::createPlatformWindow(QWindow *window) const
{
    qDebug() << "createPlatformWindow" << window;

    QWindowSystemInterface::flushWindowSystemEvents();

    // User may have specified an unused Screen for this Window
    QScreen *qscreen = window->screen();
//    if (qscreen) {
//        auto screen = static_cast<Screen*>(qscreen->handle());
//        if (screen->window()) {
//            qDebug() << "Specified Screen already has a QWindow/ScreenWindow attached, overriding";
//        }
//    }

    // If Screen was not specified, just grab an unused one, if available
    Screen *screen = m_qmirServer->screenController()->getUnusedScreen();
    if (!screen) {
        qDebug() << "No available Screens to create a new QWindow/ScreenWindow for";
        return nullptr;
    }
    qscreen = screen->screen();
    window->setScreen(qscreen);

    auto platformWindow = new ScreenWindow(window);
    qDebug() << "New" << window << "is backed by a" << screen << "with geometry" << screen->geometry();
    return platformWindow;
}

QPlatformBackingStore *MirServerIntegration::createPlatformBackingStore(QWindow *window) const
{
    qDebug() << "createPlatformBackingStore" << window;
    return nullptr;
}

QPlatformOpenGLContext *MirServerIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    qDebug() << "createPlatformOpenGLContext" << context;
    return new MirOpenGLContext(m_mirServer, context->format());
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
QAbstractEventDispatcher *MirServerIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}
#endif

void MirServerIntegration::initialize()
{
    // Creates instance of and start the Mir server in a separate thread
    m_qmirServer = new QMirServer(m_mirServer);
    connect(m_qmirServer->screenController(), &ScreenController::screenAdded,
            [&](QPlatformScreen *screen) { screenAdded(screen); });
    m_qmirServer->run();

    m_nativeInterface = new NativeInterface(m_mirServer);

    m_clipboard->setupDBusService();
}

QPlatformAccessibility *MirServerIntegration::accessibility() const
{
    return m_accessibility.data();
}

QPlatformFontDatabase *MirServerIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QStringList MirServerIntegration::themeNames() const
{
    return QStringList(UbuntuTheme::name);
}

QPlatformTheme *MirServerIntegration::createPlatformTheme(const QString& name) const
{
    Q_UNUSED(name);
    return new UbuntuTheme;
}

QPlatformServices *MirServerIntegration::services() const
{
    return m_services.data();
}

QPlatformNativeInterface *MirServerIntegration::nativeInterface() const
{
    return m_nativeInterface;
}

QPlatformClipboard *MirServerIntegration::clipboard() const
{
    return m_clipboard.data();
}

QPlatformOffscreenSurface *MirServerIntegration::createPlatformOffscreenSurface(
        QOffscreenSurface *surface) const
{
    return new OffscreenSurface(surface);
}

