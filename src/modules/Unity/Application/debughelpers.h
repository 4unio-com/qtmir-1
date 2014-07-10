/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UBUNTUGESTURES_DEBUG_HELPER_H
#define UBUNTUGESTURES_DEBUG_HELPER_H

#include <QString>

#include <mir_toolkit/common.h>

class QTouchEvent;

QString touchPointStateToString(Qt::TouchPointState state);
QString touchEventToString(const QTouchEvent *ev);

QString mirSurfaceAttribAndValueToString(MirSurfaceAttrib attrib, int value);
const char *mirSurfaceTypeToStr(int value);
const char *mirSurfaceStateToStr(int value);
const char *mirSurfaceFocusStateToStr(int value);
const char *mirSurfaceVisibilityToStr(int value);

#endif // UBUNTUGESTURES_DEBUG_HELPER_H
