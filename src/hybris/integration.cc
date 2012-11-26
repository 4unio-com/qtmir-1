// Copyright © 2012 Canonical Ltd
// FIXME(loicm) Add copyright notice here.

#include "integration.h"
#include "window.h"
#include "input.h"
#include "base/logging.h"
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatforminputcontext.h>

QHybrisIntegration::QHybrisIntegration()
    : screen_(new QHybrisScreen(static_cast<QHybrisBaseNativeInterface*>(platformIntegration())))
    , input_(NULL) {
  screenAdded(screen_);
  if (qEnvironmentVariableIsEmpty("QTHYBRIS_NO_INPUT")) {
    input_ = new QHybrisInput(this);
    inputContext_ = QPlatformInputContextFactory::create();
  }
  DLOG("QHybrisIntegration::QHybrisIntegration (this=%p)", this);
}

QHybrisIntegration::~QHybrisIntegration() {
  DLOG("QHybrisIntegration::~QHybrisIntegration");
  delete input_;
  delete inputContext_;
  delete screen_;
}

QPlatformWindow* QHybrisIntegration::createPlatformWindow(QWindow* window) const {
  DLOG("QHybrisIntegration::createPlatformWindow const (this=%p, window=%p)", this, window);
  return const_cast<QHybrisIntegration*>(this)->createPlatformWindow(window);
}

QPlatformWindow* QHybrisIntegration::createPlatformWindow(QWindow* window) {
  DLOG("QHybrisIntegration::createPlatformWindow (this=%p, window=%p)", this, window);
  QPlatformWindow* pWindow = new QHybrisWindow(window, static_cast<QHybrisScreen*>(screen_), input_);
  pWindow->requestActivateWindow();
  return pWindow;
}
