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

#include "sharedwakelock.h"
#include "abstractdbusservicemonitor.h"
#include "logging.h"

#include <QDBusAbstractInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QFile>

namespace qtmir {

const int POWERD_SYS_STATE_ACTIVE = 1; // copied from private header file powerd.h
const char cookieFile[] = "/tmp/qtmir_powerd_cookie";

/**
 * @brief The Wakelock class - on creation acquires a system wakelock, on destruction releases it
 * Designed in the spirit of RAII. Should the PowerD service vanish from the bus, the wakelock
 * will be re-acquired when it re-joins the bus.
 */
class Wakelock : public AbstractDBusServiceMonitor
{
    Q_OBJECT
public:
    Wakelock() noexcept
        : AbstractDBusServiceMonitor("com.canonical.powerd", "/com/canonical/powerd", "com.canonical.powerd", SystemBus)
    {
        // (re-)acquire wake lock when powerd (re-)appears on the bus
        QObject::connect(this, &Wakelock::serviceAvailableChanged,
                         this, &Wakelock::acquireWakelock);

        if (!serviceAvailable()) {
            qWarning() << "com.canonical.powerd DBus interface not available, waiting for it";
            return;
        }

        // WORKAROUND: if shell crashed while it held a wakelock, due to bug lp:1409722 powerd will not have released
        // the wakelock for it. As workaround, we save the cookie to file and restore it if possible.
        QFile cookie(cookieFile);
        if (cookie.exists() && cookie.open(QFile::ReadOnly | QFile::Text)) {
            m_cookie = cookie.readAll();
        } else {
            acquireWakelock(true);
        }
    }

    virtual ~Wakelock() noexcept
    {
        QFile::remove(cookieFile);

        if (!serviceAvailable()) {
            qWarning() << "com.canonical.powerd DBus interface not available";
            return;
        }

        if (!m_cookie.isEmpty()) {
            dbusInterface()->asyncCall("clearSysState", m_cookie);
        }
        qCDebug(QTMIR_SESSIONS) << "Wakelock released";
    }

private Q_SLOTS:
    void acquireWakelock(bool available)
    {
        if (!available) {
            m_cookie.clear(); // clear cookie so that when powerd re-appears, new cookie will be set
            QFile::remove(cookieFile);
            return;
        }

        QDBusPendingCall pcall = dbusInterface()->asyncCall("requestSysState", "active", POWERD_SYS_STATE_ACTIVE);

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pcall, this);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                         this, &Wakelock::onWakeLockAcquired);
    }

    void onWakeLockAcquired(QDBusPendingCallWatcher *call)
    {
        if (m_cookie.isEmpty()) { // don't overwrite existing cookie
            QDBusPendingReply<QString> reply = *call;
            if (reply.isError()) {
                qCDebug(QTMIR_SESSIONS) << "Wakelock was NOT acquired, error:"
                                        << QDBusError::errorString(reply.error().type());
            } else {
                m_cookie = reply.argumentAt<0>();

                // see WORKAROUND above for why we save cookie to disk
                QFile cookie(cookieFile);
                cookie.open(QFile::WriteOnly | QFile::Text);
                cookie.write(m_cookie.toLatin1());

                qCDebug(QTMIR_SESSIONS) << "Wakelock acquired" << m_cookie;
            }
        }
        call->deleteLater();
    }

private:
    QString m_cookie;

    Q_DISABLE_COPY(Wakelock)
};

#include "sharedwakelock.moc"

/**
 * @brief SharedWakelock - allow a single wakelock instance to be shared between multiple owners
 *
 * QtMir has application management duties to perform even if display is off. To prevent device
 * going to deep sleep before QtMir is ready, have QtMir register a system wakelock when it needs to.
 *
 * This class allows multiple objects to own the wakelock simultaneously. The wakelock is first
 * registered when acquire has been called by one caller. Multiple callers may then share the
 * wakelock. The wakelock is only destroyed when all callers have called release.
 *
 * Note a caller cannot have multiple shares of the wakelock. Multiple calls to acquire are ignored.
 */

QObject* SharedWakelock::createWakelock()
{
    return new Wakelock;
}


void SharedWakelock::acquire(const QObject *caller)
{
    if (m_owners.contains(caller) || caller == nullptr) {
        return;
    }

    // register a slot to remove itself from owners list if destroyed
    QObject::connect(caller, &QObject::destroyed, this, &SharedWakelock::release);

    if (m_wakelock.isNull()) {
        m_wakelock.reset(createWakelock());
    }

    m_owners.insert(caller);
}

void SharedWakelock::release(const QObject *caller)
{
    if (!m_owners.remove(caller) || caller == nullptr) {
        return;
    }

    if (m_owners.empty() && m_wakelock) {
        m_wakelock.reset();
    }
}

} // namespace qtmir
