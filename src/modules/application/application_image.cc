// This file is part of QtUbuntu, a set of Qt components for Ubuntu.
// Copyright © 2013 Canonical Ltd.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; version 3.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "application_image.h"
#include "application.h"
#include "logging.h"
#include <QtGui/QPainter>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <ubuntu/ui/ubuntu_ui_session_service.h>

class ApplicationImageEvent : public QEvent {
 public:
  ApplicationImageEvent(QEvent::Type type, QImage image)
      : QEvent(type)
      , image_(image) {
    DLOG("ApplicationImageEvent::ApplicationImageEvent (this=%p, type=%d)", this, type);
  }
  ~ApplicationImageEvent() {
    DLOG("ApplicationImageEvent::~ApplicationImageEvent");
  }
  static const QEvent::Type type_;
  QImage image_;
};

const QEvent::Type ApplicationImageEvent::type_ =
    static_cast<QEvent::Type>(QEvent::registerEventType());

static void snapshotCallback(const void* pixels, unsigned int width, unsigned int height,
                             unsigned int stride, void* context) {
  // FIXME(loicm) stride from Ubuntu application API is wrong.
  Q_UNUSED(stride);
  DLOG("snapshotCallback (pixels=%p, width=%u, height=%u, stride=%u, context=%p)",
       pixels, width, height, stride, context);
  DASSERT(context != NULL);
  // Copy the pixels and post an event to the GUI thread so that we can safely schedule an update.
  ApplicationImage* applicationImage = static_cast<ApplicationImage*>(context);
  QImage image(static_cast<const uchar*>(pixels), width, height, width * 4,
               QImage::Format_ARGB32_Premultiplied);
  QCoreApplication::postEvent(
      applicationImage, new ApplicationImageEvent(
          ApplicationImageEvent::type_, image.rgbSwapped()));
}

ApplicationImage::ApplicationImage(QQuickPaintedItem* parent)
    : QQuickPaintedItem(parent)
    , source_(NULL) {
  DLOG("ApplicationImage::ApplicationImage (this=%p, parent=%p)", this, parent);
  setFillColor(QColor(0, 0, 0, 255));
  setOpaquePainting(true);
}

ApplicationImage::~ApplicationImage() {
  DLOG("ApplicationImage::~ApplicationImage");
}

void ApplicationImage::customEvent(QEvent* event) {
  DLOG("ApplicationImage::customEvent (this=%p, event=%p)", this, event);
  DASSERT(QThread::currentThread() == thread());
  ApplicationImageEvent* imageEvent = static_cast<ApplicationImageEvent*>(event);
  // Store the new image and schedule an update.
  image_ = imageEvent->image_;
  update();
}

void ApplicationImage::setSource(Application* source) {
  DLOG("ApplicationImage::setApplication (this=%p, source=%p)", this, source);
  if (source_ != source) {
    source_ = source;
    emit sourceChanged();
  }
}

void ApplicationImage::scheduleUpdate() {
  DLOG("ApplicationImage::scheduleUpdate (this=%p)", this);
  if (source_ != NULL && source_->state() == Application::Running)
    ubuntu_ui_session_snapshot_running_session_with_id(source_->handle(), snapshotCallback, this);
  else
    update();
}

void ApplicationImage::paint(QPainter* painter) {
  DLOG("ApplicationImage::paint (this=%p, painter=%p)", this, painter);
  if (source_ != NULL && source_->state() == Application::Running)
    painter->drawImage(QRect(0, 0, width(), height()), image_, image_.rect());
}

void ApplicationImage::onSourceDestroyed() {
  DLOG("ApplicationImage::onSourceDestroyed (this=%p)", this);
  source_ = NULL;
}
