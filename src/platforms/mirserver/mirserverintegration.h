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

#ifndef MIRSERVERINTEGRATION_H
#define MIRSERVERINTEGRATION_H

// qt
#include <qpa/qplatformintegration.h>

// local
#include "mirserver.h"

class Display;
class NativeInterface;
class MirServer;
class QMirServer;

namespace qtmir {
    class Clipboard;
}

class MirServerIntegration : public QPlatformIntegration
{
public:
    MirServerIntegration();
    ~MirServerIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;

    QAbstractEventDispatcher *createEventDispatcher() const override;
    void initialize() override;

    QPlatformClipboard *clipboard() const override;

    QPlatformInputContext* inputContext() const override { return m_inputContext; }

    QPlatformFontDatabase *fontDatabase() const override;
    QStringList themeNames() const override;
    QPlatformTheme* createPlatformTheme(const QString& name) const override;
    QPlatformServices *services() const override;

    QPlatformAccessibility *accessibility() const override;

    QPlatformNativeInterface *nativeInterface() const override;

private:
    QScopedPointer<QPlatformAccessibility> m_accessibility;
    QScopedPointer<QPlatformFontDatabase> m_fontDb;
    QScopedPointer<QPlatformServices> m_services;

    QScopedPointer<QMirServer> m_mirServer;

    Display *m_display;
    NativeInterface *m_nativeInterface;
    QPlatformInputContext* m_inputContext;
    QScopedPointer<qtmir::Clipboard> m_clipboard;
};

#endif // MIRSERVERINTEGRATION_H
