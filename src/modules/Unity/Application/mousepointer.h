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

#ifndef QTMIR_MOUSEPOINTER_H
#define QTMIR_MOUSEPOINTER_H

// Qt
#include <QPointer>
#include <QWindow>

// platforms/mirserver
#include <mousepointerinterface.h>

namespace qtmir {

class MousePointer : public MousePointerInterface {
    Q_OBJECT
public:
    MousePointer(QQuickItem *parent = nullptr);

    void setQtCursorName(const QString &qtCursorName);

    QString cursorName() const override { return m_cursorName; }

    qreal hotspotX() const override {return m_hotspotX;}
    qreal hotspotY() const override {return m_hotspotY;}

public Q_SLOTS:
    void handleMouseEvent(ulong timestamp, QPointF movement, Qt::MouseButton buttons,
            Qt::KeyboardModifiers modifiers) override;

protected:
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private Q_SLOTS:
    void updateCursorName();

private:
    void registerWindow(QWindow *window);
    void updateHotspot();
    void setCursorName(const QString &cursorName);

    QPointer<QWindow> m_registeredWindow;
    QString m_cursorName;
    QString m_qtCursorName;
    int m_hotspotX;
    int m_hotspotY;
};

}

#endif // QTMIR_MOUSEPOINTER_H

