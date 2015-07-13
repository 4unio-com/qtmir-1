/*
 * Copyright (C) 2013-2015 Canonical, Ltd.
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

#include <QCoreApplication>
#include <QOpenGLContext>

#if QT_VERSION < QT_VERSION_CHECK(5, 2, 0)
#include <private/qguiapplication_p.h>
#endif

#include <QDebug>

// Mir
#include <mir/graphics/display.h>
#include <mir/graphics/display_configuration.h>

// local
#include "clipboard.h"
#include "display.h"
#include "displaywindow.h"
#include "miropenglcontext.h"
#include "nativeinterface.h"
#include "qmirserver.h"
#include "services.h"
#include "ubuntutheme.h"

namespace mg = mir::graphics;
using qtmir::Clipboard;

MirServerIntegration::MirServerIntegration()
    : m_accessibility(new QPlatformAccessibility())
    , m_fontDb(new QGenericUnixFontDatabase())
    , m_services(new Services)
#if QT_VERSION < QT_VERSION_CHECK(5, 2, 0)
    , m_eventDispatcher(createUnixEventDispatcher())
#endif
    , m_mirServer(new QMirServer(QCoreApplication::arguments()))
    , m_display(nullptr)
    , m_nativeInterface(nullptr)
    , m_clipboard(new Clipboard)
{
    // For access to sensors, qtmir uses qtubuntu-sensors. qtubuntu-sensors reads the
    // UBUNTU_PLATFORM_API_BACKEND variable to decide if to load a valid sensor backend or not.
    // For it to function we need to ensure a valid backend has been specified
    if (qEnvironmentVariableIsEmpty("UBUNTU_PLATFORM_API_BACKEND")) {
        if (qgetenv("DESKTOP_SESSION").contains("mir") || !qEnvironmentVariableIsSet("ANDROID_DATA")) {
            qputenv("UBUNTU_PLATFORM_API_BACKEND", "desktop_mirclient");
        } else {
            qputenv("UBUNTU_PLATFORM_API_BACKEND", "touch_mirclient");
        }
    }

    // If Mir shuts down, quit.
    QObject::connect(m_mirServer.data(), &QMirServer::stopped,
                     QCoreApplication::instance(), &QCoreApplication::quit);

#if QT_VERSION < QT_VERSION_CHECK(5, 2, 0)
    QGuiApplicationPrivate::instance()->setEventDispatcher(eventDispatcher_);
    initialize();
#endif

    m_inputContext = QPlatformInputContextFactory::create();
}

MirServerIntegration::~MirServerIntegration()
{
    delete m_nativeInterface;
    delete m_display;
}

bool MirServerIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    case SharedGraphicsCache: return true;
    case BufferQueueingOpenGL: return true;
    case MultipleWindows: return false; // multi-monitor support
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    case WindowManagement: return false; // platform has no WM, as this implements the WM!
    case NonFullScreenWindows: return false;
#endif
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *MirServerIntegration::createPlatformWindow(QWindow *window) const
{
    QWindowSystemInterface::flushWindowSystemEvents();

    DisplayWindow* displayWindow = nullptr;

    auto const mirServer = m_mirServer->mirServer().lock();
    mg::DisplayBuffer* first_buffer{nullptr};
    mg::DisplaySyncGroup* first_group{nullptr};
    if (mirServer) {
        mirServer->the_display()->for_each_display_sync_group([&](mg::DisplaySyncGroup &group) {
            if (!first_group) {
                first_group = &group;
            }
            group.for_each_display_buffer([&](mg::DisplayBuffer &buffer) {
                if (!first_buffer) {
                    first_buffer = &buffer;
                }
            });
        });
    }

    // FIXME(gerry) this will go very bad for >1 display buffer
    if (first_group && first_buffer)
        displayWindow = new DisplayWindow(window, first_group, first_buffer);

    if (!displayWindow)
        return nullptr;

    //displayWindow->requestActivateWindow();
    return displayWindow;
}

QPlatformBackingStore *MirServerIntegration::createPlatformBackingStore(QWindow *window) const
{
    qDebug() << "createPlatformBackingStore" << window;
    return nullptr;
}

QPlatformOpenGLContext *MirServerIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    qDebug() << "createPlatformOpenGLContext" << context;
    return new MirOpenGLContext(m_mirServer->mirServer(), context->format());
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
    if (!m_mirServer->start()) {
        exit(2);
    }

    m_display = new Display(m_mirServer->mirServer().data()->the_display()->configuration());
    m_nativeInterface = new NativeInterface(m_mirServer->mirServer());

    for (QPlatformScreen *screen : m_display->screens())
        screenAdded(screen);

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
