/*
 * Copyright (C) 2013-2016 Canonical, Ltd.
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

#ifndef NATIVEINTEGRATION_H
#define NATIVEINTEGRATION_H

// qt
#include <QSharedPointer>
#include <qpa/qplatformnativeinterface.h>

// local
#include "qmirserver.h"

class NativeInterface : public QPlatformNativeInterface
{
public:
    NativeInterface(QMirServer *);

    virtual void *nativeResourceForIntegration(const QByteArray &resource);

    QVariantMap windowProperties(QPlatformWindow *window) const override;
    QVariant windowProperty(QPlatformWindow *window, const QString &name) const override;
    QVariant windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const override;

    QWeakPointer<MirServer> mirServer() { return m_qMirServer->mirServer(); }

private:
    QMirServer *m_qMirServer;
};

#endif // NATIVEINTEGRATION_H
