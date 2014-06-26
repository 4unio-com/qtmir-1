/*
 * Copyright (C) 2013,2014 Canonical, Ltd.
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

// local
#include "application_manager.h"
#include "proc_info.h"
#include "application.h"
#include "desktopfilereader.h"
#include "dbuswindowstack.h"
#include "taskcontroller.h"
#include "upstart/applicationcontroller.h"

// QPA mirserver
#include "mirserverconfiguration.h"
#include "mirplacementstrategy.h"
#include "nativeinterface.h"
#include "sessionlistener.h"
#include "sessionauthorizer.h"
#include "taskcontroller.h"
#include "logging.h"

// mir
#include <mir/scene/surface.h>
#include <mir/scene/session.h>
#include <mir/shell/focus_controller.h>

// Qt
#include <QGuiApplication>
#include <QJSEngine>
#include <QJSValue>

// std
#include <csignal>

namespace ms = mir::scene;

Q_LOGGING_CATEGORY(QTMIR_APPLICATIONS, "qtmir.applications")

using namespace unity::shell::application;

namespace qtmir
{

namespace {

// FIXME: To be removed once shell has fully adopted short appIds!!
QString toShortAppIdIfPossible(const QString &appId) {
    QRegExp longAppIdMask("[a-z0-9][a-z0-9+.-]+_[a-zA-Z0-9+.-]+_[0-9][a-zA-Z0-9.+:~-]*");
    if (longAppIdMask.exactMatch(appId)) {
        qWarning() << "WARNING: long App ID encountered:" << appId;
        // input string a long AppId, chop the version string off the end
        QStringList parts = appId.split("_");
        return QString("%1_%2").arg(parts.first()).arg(parts.at(1));
    }
    return appId;
}

void connectToSessionListener(ApplicationManager *manager, SessionListener *listener)
{
    QObject::connect(listener, &SessionListener::sessionStarting,
                     manager, &ApplicationManager::onSessionStarting);
    QObject::connect(listener, &SessionListener::sessionStopping,
                     manager, &ApplicationManager::onSessionStopping);
    QObject::connect(listener, &SessionListener::sessionCreatedSurface,
                     manager, &ApplicationManager::onSessionCreatedSurface);
}

void connectToSessionAuthorizer(ApplicationManager *manager, SessionAuthorizer *authorizer)
{
    QObject::connect(authorizer, &SessionAuthorizer::requestAuthorizationForSession,
                     manager, &ApplicationManager::authorizeSession, Qt::BlockingQueuedConnection);
}

void connectToPlacementStrategy(ApplicationManager *manager, MirPlacementStrategy *strategy)
{
    QObject::connect(strategy, &MirPlacementStrategy::requestSizeForSurface,
                     manager, &ApplicationManager::determineSizeForNewSurface, Qt::BlockingQueuedConnection);
}

void connectToTaskController(ApplicationManager *manager, TaskController *controller)
{
    QObject::connect(controller, &TaskController::processStarting,
                     manager, &ApplicationManager::onProcessStarting);
    QObject::connect(controller, &TaskController::processStopped,
                     manager, &ApplicationManager::onProcessStopped);
    QObject::connect(controller, &TaskController::processFailed,
                     manager, &ApplicationManager::onProcessFailed);
    QObject::connect(controller, &TaskController::requestFocus,
                     manager, &ApplicationManager::onFocusRequested);
    QObject::connect(controller, &TaskController::requestResume,
                     manager, &ApplicationManager::onResumeRequested);
}

} // namespace



ApplicationManager* ApplicationManager::Factory::Factory::create(QJSEngine *jsEngine)
{
    NativeInterface *nativeInterface = dynamic_cast<NativeInterface*>(QGuiApplication::platformNativeInterface());

    if (!nativeInterface) {
        qCritical() << "ERROR: Unity.Application QML plugin requires use of the 'mirserver' QPA plugin";
        QGuiApplication::quit();
        return nullptr;
    }

    auto mirConfig = nativeInterface->m_mirConfig;

    SessionListener *sessionListener = static_cast<SessionListener*>(nativeInterface->nativeResourceForIntegration("SessionListener"));
    SessionAuthorizer *sessionAuthorizer = static_cast<SessionAuthorizer*>(nativeInterface->nativeResourceForIntegration("SessionAuthorizer"));

    QSharedPointer<upstart::ApplicationController> appController(new upstart::ApplicationController());
    QSharedPointer<TaskController> taskController(new TaskController(nullptr, appController));
    QSharedPointer<DesktopFileReader::Factory> fileReaderFactory(new DesktopFileReader::Factory());
    QSharedPointer<ProcInfo> procInfo(new ProcInfo());

    // FIXME: We should use a QSharedPointer to wrap this ApplicationManager object, which requires us
    // to use the data() method to pass the raw pointer to the QML engine. However the QML engine appears
    // to take ownership of the object, and deletes it when it wants to. This conflicts with the purpose
    // of the QSharedPointer, and a double-delete results. Trying QQmlEngine::setObjectOwnership on the
    // object no effect, which it should. Need to investigate why.
    ApplicationManager* appManager = new ApplicationManager(
                                             mirConfig,
                                             taskController,
                                             fileReaderFactory,
                                             procInfo,
                                             jsEngine
                                         );

    connectToSessionListener(appManager, sessionListener);
    connectToSessionAuthorizer(appManager, sessionAuthorizer);
    connectToTaskController(appManager, taskController.data());

    return appManager;
}


ApplicationManager* ApplicationManager::singleton(QJSEngine *jsEngine)
{
    static ApplicationManager* instance;
    if (!instance) {
        Factory appFactory;
        instance = appFactory.create(jsEngine);
    }
    return instance;
}

ApplicationManager::ApplicationManager(
        const QSharedPointer<MirServerConfiguration>& mirConfig,
        const QSharedPointer<TaskController>& taskController,
        const QSharedPointer<DesktopFileReader::Factory>& desktopFileReaderFactory,
        const QSharedPointer<ProcInfo>& procInfo,
        QJSEngine *jsEngine,
        QObject *parent)
    : ApplicationManagerInterface(parent)
    , m_mirConfig(mirConfig)
    , m_lifecycleExceptions(QStringList() << "com.ubuntu.music")
    , m_dbusWindowStack(new DBusWindowStack(this))
    , m_taskController(taskController)
    , m_desktopFileReaderFactory(desktopFileReaderFactory)
    , m_procInfo(procInfo)
    , m_jsEngine(jsEngine)
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::ApplicationManager (this=%p)" << this;
}

ApplicationManager::~ApplicationManager()
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::~ApplicationManager";
}

int ApplicationManager::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? m_applications.size() : 0;
}

QVariant ApplicationManager::data(const QModelIndex &index, int role) const
{
    if (index.row() >= 0 && index.row() < m_applications.size()) {
        Application *application = m_applications.at(index.row());
        switch (role) {
            case RoleAppId:
                return QVariant::fromValue(application->appId());
            case RoleName:
                return QVariant::fromValue(application->name());
            case RoleComment:
                return QVariant::fromValue(application->comment());
            case RoleIcon:
                return QVariant::fromValue(application->icon());
            case RoleStage:
                return QVariant::fromValue((int)application->stage());
            case RoleState:
                return QVariant::fromValue((int)application->state());
            case RoleFocused:
                return QVariant::fromValue(application->focused());
            case RoleScreenshot:
                return QVariant::fromValue(application->screenshot());
            case RoleSurface:
                return QVariant::fromValue(application->surface());
            case RoleFullscreen:
                return QVariant::fromValue(application->fullscreen());
            default:
                return QVariant();
        }
    } else {
        return QVariant();
    }
}

Application* ApplicationManager::get(int index) const
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::get - index=" << index  << "count=" << m_applications.count();
    if (index < 0 || index >= m_applications.count()) {
        return nullptr;
    }
    return m_applications.at(index);
}

Application* ApplicationManager::findApplication(const QString &inputAppId) const
{
    const QString appId = toShortAppIdIfPossible(inputAppId);

    for (Application *app : m_applications) {
        if (app->appId() == appId) {
            return app;
        }
    }
    return nullptr;
}

QString ApplicationManager::focusedApplicationId() const
{
    return m_focusedApplicationId;
}

bool ApplicationManager::suspended() const
{
    return m_suspended;
}

void ApplicationManager::setSuspended(bool suspended)
{
    if (suspended == m_suspended) {
        return;
    }
    m_suspended = suspended;
    Q_EMIT suspendedChanged();

    if (m_suspended) {
        // TODO - save state of all apps, then suspend all
    } else {
        // TODO - restore state
    }
}

/**
 * @brief ApplicationManager::suspendApplication - suspend a running app, if permitted
 * @param inputAppId
 * @return True if application is running and can be lifecycle suspended, else false
 */
bool ApplicationManager::suspendApplication(const QString &inputAppId)
{
    const QString appId = toShortAppIdIfPossible(inputAppId);
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::suspendApplication - appId=" << appId;

    Application *application = findApplication(appId);
    if (application == nullptr)
        return false;

    // If present in exceptions list, do nothing and just return true.
    if (!m_lifecycleExceptions.filter(application->appId().section('_',0,0)).empty())
        return true;

    if (application->state() != Application::Running)
        return false;

    application->setState(Application::Suspended);
    return true;
}

/**
 * @brief ApplicationManager::resumeApplication - resumes a suspended application
 * @param inputAppId
 * @return True if application exists and is suspended or lifecycle stopped, else false
 */
bool ApplicationManager::resumeApplication(const QString &inputAppId)
{
    const QString appId = toShortAppIdIfPossible(inputAppId);
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::resumeApplication - appId=" << appId;

    Application *application = findApplication(appId);

    if (application == nullptr)
        return false;

    if (application->state() == Application::Running || application->state() == Application::Starting)
        return false;

    application->setState(Application::Running);
    return true;
}

/**
 * @brief ApplicationManager::startApplication launches an application identified by an "application id" or appId.
 *
 * Note: due to an implementation detail, appIds come in two forms:
 * * long appId: $(click_package)_$(application)_$(version)
 * * short appId: $(click_package)_$(application)
 * It is expected that the shell uses _only_ short appIds (but long appIds are accepted by this method for legacy
 * reasons - but be warned, this ability will be removed)
 *
 * Unless stated otherwise, we always use short appIds in this API.
 *
 * @param inputAppId AppId of application to launch (long appId supported)
 * @param arguments Command line arguments to pass to the application to be launched
 * @return Pointer to Application object representing the launched process. If process already running, return nullptr
 */
Application* ApplicationManager::startApplication(const QString &inputAppId,
                                                  const QStringList &arguments)
{
    QString appId = toShortAppIdIfPossible(inputAppId);
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::startApplication - this=" << this << "appId" << qPrintable(appId);

    Application *application = findApplication(appId);
    if (application) {
        qWarning() << "ApplicationManager::startApplication - application appId=" << appId << " already exists";
        return nullptr;
    }

    if (!m_taskController->start(appId, arguments)) {
        qWarning() << "Upstart failed to start application with appId" << appId;
        return nullptr;
    }

    application = new Application(
                m_taskController,
                m_desktopFileReaderFactory->createInstance(appId, m_taskController->findDesktopFileForAppId(appId)),
                Application::Starting,
                arguments,
                this);

    if (!application->isValid()) {
        qWarning() << "Unable to instantiate application with appId" << appId;
        return nullptr;
    }

    add(application);
    return application;
}

void ApplicationManager::onProcessStarting(const QString &appId)
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::onProcessStarting - appId=" << appId;

    Application *application = findApplication(appId);
    if (!application) { // then shell did not start this application, so ubuntu-app-launch must have - add to list
        application = new Application(
                    m_taskController,
                    m_desktopFileReaderFactory->createInstance(appId, m_taskController->findDesktopFileForAppId(appId)),
                    Application::Starting,
                    QStringList(), this);

        if (!application->isValid()) {
            qWarning() << "Unable to instantiate application with appId" << appId;
            return;
        }

        add(application);
    }
    else {
        qWarning() << "ApplicationManager::onProcessStarting application already found with appId" << appId;
    }
}

/**
 * @brief ApplicationManager::stopApplication - stop a running application and remove from list
 * @param inputAppId
 * @return True if running application was stopped, false if application did not exist or could not be stopped
 */
bool ApplicationManager::stopApplication(const QString &inputAppId)
{
    const QString appId = toShortAppIdIfPossible(inputAppId);
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::stopApplication - appId=" << appId;

    Application *application = findApplication(appId);
    if (!application) {
        qCritical() << "No such running application with appId" << appId;
        return false;
    }

    remove(application);
    m_dbusWindowStack->WindowDestroyed(0, appId);

    bool result = m_taskController->stop(application->longAppId());

    if (!result && application->pid() > 0) {
        qWarning() << "FAILED to ask Upstart to stop application with appId" << appId
                   << "Sending SIGTERM to process:" << application->pid();
        kill(application->pid(), SIGTERM);
        result = true;
    }

    delete application;
    return result;
}

/**
 * @brief ApplicationManager::moveToFront - move application to be the first entry of the model
 * @param inputAppId
 * @return True if application exists, else false
 */
bool ApplicationManager::moveToFront(const QString &inputAppId)
{
    const QString appId = toShortAppIdIfPossible(inputAppId);
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::moveToFront - appId=" << appId;

    Application *application = findApplication(appId);
    if (!application) {
        qCritical() << "No such running application with appId" << appId;
        return false;
    }
    int from = m_applications.indexOf(application);
    move(from, 0);
    return true;
}

/**
 * @brief ApplicationManager::registerSurfaceSizer - register a JS function to decide surface geometry
 * @param slot
 * Use this to register a Javascript function which is called whenever an application is asking Mir for
 * a new surface. The function is passed an object which has three properties:
 *     application - the Application object
 *     width - the requested surface width
 *     height - the rquested surface height
 * To override the width and/or height, this function must return an object with width & height properties
 * set to the desired values. Otherwise the application requested geometry will be used.
 */
void ApplicationManager::registerSurfaceSizer(const QJSValue slot)
{
    if (slot.isCallable()) {
        m_surfaceSizer = slot;
    } else {
        qDebug() << "ERROR: Attempting to pass a non-function to registerSurfaceSizer, is ignored";
    }
}

/**
 * @brief ApplicationManager::deregisterSurfaceSizer - deregister the JS function surface geometry decider
 */
void ApplicationManager::deregisterSurfaceSizer()
{
    m_surfaceSizer = QJSValue::UndefinedValue;
}


void ApplicationManager::onProcessFailed(const QString &appId, const bool duringStartup)
{
    /* Applications fail if they fail to launch, crash or are killed. If failed to start, must
     * immediately remove from list of applications. If crash or kill, instead we set flag on the
     * Application to indicate it can be resumed.
     */

    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::onProcessStopped - appId=" << appId << "duringStartup=" << duringStartup;

    Application *application = findApplication(appId);
    if (!application) {
        qWarning() << "ApplicationManager::onProcessFailed - upstart reports failure of application" << appId
                   << "that AppManager is not managing";
        return;
    }

    Q_UNUSED(duringStartup); // FIXME(greyback) upstart reports app that fully started up & crashes as failing during startup??
    if (application->state() == Application::Starting) {
        remove(application);
        m_dbusWindowStack->WindowDestroyed(0, application->appId());
        delete application;
    } else {
        // We need to set flags on the Application to say the app can be resumed, and thus should not be removed
        // from the list by onProcessStopped.
        application->setCanBeResumed(true);
        application->setPid(0);
    }
}

void ApplicationManager::onProcessStopped(const QString &appId)
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::onProcessStopped - appId=" << appId;
    Application *application = findApplication(appId);

    // if shell did not stop the application, but ubuntu-app-launch says it died, we assume the process has been
    // killed, so it can be respawned later. Only exception is if that application is focused or running
    // as then it most likely crashed. Update this logic when ubuntu-app-launch gives some failure info.
    if (application) {
        bool removeApplication = false;

        // The following scenario is the only time that we do NOT remove the application from the app list:
        if ((application->state() == Application::Suspended || application->state() == Application::Stopped)
                && application->pid() == 0 // i.e. onProcessFailed was called, which resets the PID of this application
                && application->canBeResumed()) {
            removeApplication = false;
        }

        if (removeApplication) {
            qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::onProcessStopped - removing appId=" << appId;
            remove(application);
            m_dbusWindowStack->WindowDestroyed(0, application->appId());
            delete application;
        }
    }
}

void ApplicationManager::onFocusRequested(const QString& appId)
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::onFocusRequested - appId=" << appId;

    Q_EMIT focusRequested(appId);
}

void ApplicationManager::onResumeRequested(const QString& appId)
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::onResumeRequested - appId=" << appId;

    Application *application = findApplication(appId);

    if (!application) {
        qCritical() << "ApplicationManager::onResumeRequested: No such running application" << appId;
        return;
    }

    // If app Stopped, trust that ubuntu-app-launch respawns it itself, and AppManager will
    // be notified of that through the onProcessStartReportReceived slot. Else resume.
    if (application->state() == Application::Suspended) {
        application->setState(Application::Running);
    }
}

bool ApplicationManager::updateScreenshot(const QString &inputAppId)
{
    const QString appId = toShortAppIdIfPossible(inputAppId);

    Application *application = findApplication(appId);

    if (!application) {
        qWarning() << "No such running application with appId=" << appId;
        return false;
    }

    application->updateScreenshot();
    QModelIndex appIndex = findIndex(application);
    Q_EMIT dataChanged(appIndex, appIndex, QVector<int>() << RoleScreenshot);
    return true;
}

void ApplicationManager::screenshotUpdated()
{
    if (sender()) {
        Application *application = static_cast<Application*>(sender());
        QModelIndex appIndex = findIndex(application);
        Q_EMIT dataChanged(appIndex, appIndex, QVector<int>() << RoleScreenshot);

        qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::screenshotUpdated: Received new screenshot for", application->appId();
    } else {
        qCDebug(QTMIR_APPLICATIONS) <<"ApplicationManager::screenshotUpdated: Received screenshotUpdated signal but application has disappeard.";
    }
}

void ApplicationManager::authorizeSession(const quint64 pid, bool &authorized)
{
    authorized = false; //to be proven wrong

    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::authorizeSession - pid=" << pid;

    for (Application *app : m_applications) {
        if (app->state() == Application::Starting
                && m_taskController->appIdHasProcessId(app->appId(), pid)) {
            app->setPid(pid);
            authorized = true;
            return;
        }
    }

    /*
     * Hack: Allow applications to be launched without being managed by upstart, where AppManager
     * itself manages processes executed with a "--desktop_file_hint=/path/to/desktopFile.desktop"
     * parameter attached. This exists until ubuntu-app-launch can notify shell any application is
     * and so shell should allow it. Also reads the --stage parameter to determine the desired stage
     */
    std::unique_ptr<ProcInfo::CommandLine> info = m_procInfo->commandLine(pid);
    if (!info) {
        qWarning() << "ApplicationManager REJECTED connection from app with pid" << pid
                   << "as unable to read the process command line";
        return;
    }

    if (info->startsWith("maliit-server") || info->contains("qt5/libexec/QtWebProcess")) {
        authorized = true;
        return;
    }

    boost::optional<QString> desktopFileName{ info->getParameter("--desktop_file_hint=") };

    if (!desktopFileName) {
        qCritical() << "ApplicationManager REJECTED connection from app with pid" << pid
                    << "as no desktop_file_hint specified";
        return;
    }

    qCDebug(QTMIR_APPLICATIONS) << "Process supplied desktop_file_hint, loading" << desktopFileName;

    // Guess appId from the desktop file hint
    QString appId = toShortAppIdIfPossible(desktopFileName.get().remove(QRegExp(".desktop$")).split('/').last());

    // FIXME: right now we support --desktop_file_hint=appId for historical reasons. So let's try that in
    // case we didn't get an existing .desktop file path
    DesktopFileReader* desktopData;
    if (QFileInfo(desktopFileName.get()).exists()) {
        desktopData = m_desktopFileReaderFactory->createInstance(appId, QFileInfo(desktopFileName.get()));
    } else {
        desktopData = m_desktopFileReaderFactory->createInstance(appId, m_taskController->findDesktopFileForAppId(appId));
    }

    if (!desktopData->loaded()) {
        delete desktopData;
        qCritical() << "ApplicationManager REJECTED connection from app with pid" << pid
                    << "as the file specified by the desktop_file_hint argument could not be opened";
        return;
    }

    // some naughty applications use a script to launch the actual application. Check for the
    // case where shell actually launched the script.
    Application *application = findApplication(desktopData->appId());
    if (application && application->state() == Application::Starting) {
        qCDebug(QTMIR_APPLICATIONS) << "Process with pid" << pid << "appeared, attaching to existing entry"
                                    << "in application list with appId:" << application->appId();
        delete desktopData;
        application->setSessionName(application->appId());
        application->setPid(pid);
        authorized = true;
        return;
    }

    // if stage supplied in CLI, fetch that
    Application::Stage stage = Application::MainStage;
    boost::optional<QString> stageParam = info->getParameter("--stage_hint=");

    if (stageParam && stageParam.get() == "side_stage") {
        stage = Application::SideStage;
    }

    qCDebug(QTMIR_APPLICATIONS) << "New process with pid" << pid << "appeared, adding new application to the"
                                << "application list with appId:" << desktopData->appId();

    QStringList arguments(info->asStringList());
    application = new Application(m_taskController, desktopData, Application::Starting, arguments, this);
    application->setPid(pid);
    application->setStage(stage);
    application->setCanBeResumed(false);
    add(application);
    authorized = true;
}

void ApplicationManager::determineSizeForNewSurface(mir::scene::Session const *session, QSize &size)
{
    Application* application = findApplicationWithSession(session);
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::determineSizeForNewSurface - application=" << application
                                << "session=" << session << "name=" << (session?(session->name().c_str()):"null");

    if (m_surfaceSizer.isCallable()) {
        QJSValue argument = m_jsEngine->newObject();
        argument.setProperty("width", size.width());
        argument.setProperty("height", size.height());
        if (application) {
            QJSValue jsApp = m_jsEngine->newQObject(application);
            argument.setProperty("application", jsApp);
        }

        QJSValue output = m_surfaceSizer.call(QJSValueList() << argument);
        if (output.isObject()) {
            QJSValue width = output.property("width");
            QJSValue height = output.property("height");
            if (width.isNumber())
                size.setWidth(width.toInt());
            if (height.isNumber())
                size.setHeight(height.toInt());
        } else {
            qDebug() << "ApplicationManager::determineSizeForNewSurface - unrecognised object returned from JS callback";
        }
    }
}

void ApplicationManager::onSessionStarting(std::shared_ptr<ms::Session> const& session)
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::onSessionStarting - sessionName=" <<  session->name().c_str();

    Application* application = findApplicationWithPid(session->process_id());
    if (application && application->state() != Application::Running) {
        application->setSession(session);
    } else {
        qCritical() << "ApplicationManager::onSessionStarting - unauthorized application!!";
    }
}

void ApplicationManager::onSessionStopping(std::shared_ptr<ms::Session> const& session)
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::onSessionStopping - sessionName=" << session->name().c_str();

    // in case application closed not by hand of shell, check again here:
    Application* application = findApplicationWithSession(session);
    if (application) {
        /* Can remove the application from the running apps list immediately in these curcumstances:
         *  1. application is not managed by upstart (this message from Mir is only notice the app has stopped, must do
         *     it here)
         *  2. application is managed by upstart, but has stopped before it managed to create a surface, we can assume
         *     it crashed on startup, and thus cannot be resumed - so remove it.
         *  3. application is managed by upstart and is in foreground (i.e. has Running state), if Mir reports the
         *     application disconnects, it either crashed or stopped itself. Either case, remove it.
         */
        if (!application->canBeResumed()
                || application->state() == Application::Starting
                || application->state() == Application::Running) {
            m_dbusWindowStack->WindowDestroyed(0, application->appId());
            remove(application);
            delete application;
        } else {
            // otherwise, we do not have enough information to make any changes to the model, so await events from
            // upstart to go further, but set the app state
            application->setState(Application::Stopped);
        }
    }
}

void ApplicationManager::onSessionCreatedSurface(ms::Session const* session,
                                                 std::shared_ptr<ms::Surface> const& surface)
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::onSessionCreatedSurface - sessionName=" << session->name().c_str();
    Q_UNUSED(surface);

    Application* application = findApplicationWithSession(session);
    if (application && application->state() == Application::Starting) {
        m_dbusWindowStack->WindowCreated(0, application->appId()); //FIXME(greyback) - SurfaceManager should do this
    }
}

void ApplicationManager::setFocused(Application *application)
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::setFocused - appId=" << application->appId();

    m_focusedApplicationId = application->appId();
    move(m_applications.indexOf(application), 0);
    Q_EMIT focusedApplicationIdChanged();
    m_dbusWindowStack->FocusedWindowChanged(0, application->appId(), application->stage()); //FIXME(greyback) - SurfaceManager should do this
    QModelIndex appIndex = findIndex(application);
    Q_EMIT dataChanged(appIndex, appIndex, QVector<int>() << RoleFocused);
}

Application* ApplicationManager::findApplicationWithSession(const std::shared_ptr<ms::Session> &session) const
{
    return findApplicationWithSession(session.get());
}

Application* ApplicationManager::findApplicationWithSession(const ms::Session *session) const
{
    for (Application *app : m_applications) {
        if (app->session().get() == session) {
            return app;
        }
    }
    return nullptr;
}

Application* ApplicationManager::findApplicationWithPid(const qint64 pid) const
{
    if (pid <= 0)
        return nullptr;

    for (Application *app : m_applications) {
        if (app->m_pid == pid) {
            return app;
        }
    }
    return nullptr;
}

QModelIndex ApplicationManager::findIndex(const Application *application) const
{
    for (int i = 0; i < m_applications.size(); ++i) {
        if (m_applications.at(i) == application) {
            return index(i);
        }
    }

    return QModelIndex();
}

void ApplicationManager::add(Application* application)
{
    Q_ASSERT(application != NULL);
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::add - appId=" << application->appId();

    beginInsertRows(QModelIndex(), 0, 0);
    m_applications.prepend(application);
    endInsertRows();
    Q_EMIT countChanged();
    Q_EMIT applicationAdded(application->appId());
}

void ApplicationManager::remove(Application *application)
{
    Q_ASSERT(application != NULL);
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::remove - appId=" << application->appId();

    int i = m_applications.indexOf(application);
    if (i != -1) {
        beginRemoveRows(QModelIndex(), i, i);
        m_applications.removeAt(i);
        endRemoveRows();
        Q_EMIT applicationRemoved(application->appId());
        Q_EMIT countChanged();
    }
}

void ApplicationManager::move(const int from, const int to)
{
    qCDebug(QTMIR_APPLICATIONS) << "ApplicationManager::move - from=" << from << "to=" << to;
    if (from == to) return;

    if (from >= 0 && from < m_applications.size() && to >= 0 && to < m_applications.size()) {
        QModelIndex parent;
        /* When moving an item down, the destination index needs to be incremented
           by one, as explained in the documentation:
           http://qt-project.org/doc/qt-5.0/qtcore/qabstractitemmodel.html#beginMoveRows */
        beginMoveRows(parent, from, from, parent, to + (to > from ? 1 : 0));
        m_applications.move(from, to);
        endMoveRows();
    }
}

QList<Application*> ApplicationManager::list() const
{
    return m_applications;
}

} // namespace qtmir
