/*
 * Copyright (C) 2013 Canonical, Ltd.
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
 */

#include <Unity/Application/taskcontroller.h>

#include "mock_oom_controller.h"
#include "mock_process_controller.h"

#include <core/posix/fork.h>
#include <core/posix/linux/proc/process/oom_score_adj.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
using namespace qtmir;

struct TriggerableApplicationController : public ApplicationController
{
    void triggerApplicationStarted(const QString& appId)
    {
        Q_EMIT applicationStarted(appId);
    }

    void triggerApplicationStopped(const QString& appId)
    {
        Q_EMIT applicationStopped(appId);
    }

    void triggerApplicationFocusRequest(const QString& appId)
    {
        Q_EMIT applicationFocusRequest(appId);
    }

    MOCK_METHOD1(primaryPidForAppId, pid_t (const QString&));
    MOCK_METHOD2(appIdHasProcessId, bool(pid_t, const QString&));
    MOCK_CONST_METHOD1(findDesktopFileForAppId, QFileInfo(const QString &appId));

    MOCK_METHOD1(stopApplicationWithAppId, bool(const QString&));
    MOCK_METHOD2(startApplicationWithAppIdAndArgs, bool(const QString&, const QStringList&));
};
}

TEST(TaskController, startingAnApplicationCallsCorrectlyIntoApplicationController)
{
    using namespace ::testing;

    const QString appId{"com.canonical.does.not.exist"};

    testing::NiceMock<TriggerableApplicationController> appController;
    QSharedPointer<TriggerableApplicationController> appControllerPtr(
                &appController,
                [](ApplicationController*){});

    EXPECT_CALL(appController, startApplicationWithAppIdAndArgs(appId, QStringList())).Times(1).WillRepeatedly(Return(true));

    testing::NiceMock<MockOomController> oomController;
    QSharedPointer<ProcessController::OomController> oomControllerPtr(
                &oomController,
                [](ProcessController::OomController*){});

    NiceMock<testing::MockProcessController> processController(oomControllerPtr);
    QSharedPointer<ProcessController> processControllerPtr(
                &processController,
                [](ProcessController*){});

    TaskController taskController(nullptr,
                                  appControllerPtr,
                                  processControllerPtr);

    taskController.start(appId, QStringList());
}

TEST(TaskController, stoppingAnApplicationCallsCorrectlyIntoApplicationController)
{
    using namespace ::testing;

    const QString appId{"com.canonical.does.not.exist"};

    testing::NiceMock<TriggerableApplicationController> appController;
    QSharedPointer<TriggerableApplicationController> appControllerPtr(
                &appController,
                [](ApplicationController*){});

    EXPECT_CALL(appController, stopApplicationWithAppId(appId)).Times(1).WillRepeatedly(Return(true));

    testing::NiceMock<MockOomController> oomController;
    QSharedPointer<ProcessController::OomController> oomControllerPtr(
                &oomController,
                [](ProcessController::OomController*){});

    NiceMock<testing::MockProcessController> processController(oomControllerPtr);
    QSharedPointer<ProcessController> processControllerPtr(
                &processController,
                [](ProcessController*){});

    TaskController taskController(nullptr,
                                  appControllerPtr,
                                  processControllerPtr);

    taskController.stop(appId);
}

TEST(TaskController, suspendingAnApplicationAdjustsOomScoreForCorrectPid)
{
    using namespace ::testing;

    const QString appId{"com.canonical.does.not.exist"};

    testing::NiceMock<TriggerableApplicationController> appController;
    QSharedPointer<TriggerableApplicationController> appControllerPtr(
                &appController,
                [](ApplicationController*){});

    EXPECT_CALL(appController, primaryPidForAppId(appId)).Times(1).WillRepeatedly(Return(-1));

    testing::NiceMock<MockOomController> oomController;
    QSharedPointer<ProcessController::OomController> oomControllerPtr(
                &oomController,
                [](ProcessController::OomController*){});

    NiceMock<testing::MockProcessController> processController(oomControllerPtr);
    QSharedPointer<ProcessController> processControllerPtr(
                &processController,
                [](ProcessController*){});

    EXPECT_CALL(oomController, ensureProcessLikelyToBeKilled(-1)).Times(1);

    TaskController taskController(nullptr,
                                  appControllerPtr,
                                  processControllerPtr);

    taskController.suspend(appId);
}

TEST(TaskController, resumingAnApplicationAdjustsOomScoreForCorrectPid)
{
    using namespace ::testing;

    const QString appId{"com.canonical.does.not.exist"};

    testing::NiceMock<TriggerableApplicationController> appController;
    QSharedPointer<TriggerableApplicationController> appControllerPtr(
                &appController,
                [](ApplicationController*){});

    EXPECT_CALL(appController, primaryPidForAppId(appId)).Times(1).WillRepeatedly(Return(-1));

    testing::NiceMock<MockOomController> oomController;
    QSharedPointer<ProcessController::OomController> oomControllerPtr(
                &oomController,
                [](ProcessController::OomController*){});

    NiceMock<testing::MockProcessController> processController(oomControllerPtr);
    QSharedPointer<ProcessController> processControllerPtr(
                &processController,
                [](ProcessController*){});

    EXPECT_CALL(oomController, ensureProcessUnlikelyToBeKilled(-1)).Times(1);

    TaskController taskController(nullptr,
                                  appControllerPtr,
                                  processControllerPtr);

    taskController.resume(appId);
}

TEST(TaskController, aStartedApplicationIsOomScoreAdjusted)
{
    using namespace ::testing;

    const QString appId{"com.canonical.does.not.exist"};

    testing::NiceMock<TriggerableApplicationController> appController;
    QSharedPointer<TriggerableApplicationController> appControllerPtr(
                &appController,
                [](ApplicationController*){});
    EXPECT_CALL(appController, primaryPidForAppId(appId)).Times(1).WillRepeatedly(Return(42));

    testing::NiceMock<MockOomController> oomController;
    QSharedPointer<ProcessController::OomController> oomControllerPtr(
                &oomController,
                [](ProcessController::OomController*){});

    NiceMock<testing::MockProcessController> processController(oomControllerPtr);
    QSharedPointer<ProcessController> processControllerPtr(
                &processController,
                [](ProcessController*){});

    EXPECT_CALL(oomController, ensureProcessUnlikelyToBeKilled(42)).Times(1);

    TaskController taskController(nullptr,
                                  appControllerPtr,
                                  processControllerPtr);

    appControllerPtr->triggerApplicationStarted(appId);
}

TEST(TaskController, aFocusedApplicationIsOomScoreAdjusted)
{
    using namespace ::testing;

    const QString appId{"com.canonical.does.not.exist"};

    testing::NiceMock<TriggerableApplicationController> appController;
    QSharedPointer<TriggerableApplicationController> appControllerPtr(
                &appController,
                [](ApplicationController*){});
    EXPECT_CALL(appController, primaryPidForAppId(appId)).Times(1).WillRepeatedly(Return(42));

    testing::NiceMock<MockOomController> oomController;
    QSharedPointer<ProcessController::OomController> oomControllerPtr(
                &oomController,
                [](ProcessController::OomController*){});

    NiceMock<testing::MockProcessController> processController(oomControllerPtr);
    QSharedPointer<ProcessController> processControllerPtr(
                &processController,
                [](ProcessController*){});

    EXPECT_CALL(oomController, ensureProcessUnlikelyToBeKilled(42)).Times(1);

    TaskController taskController(nullptr,
                                  appControllerPtr,
                                  processControllerPtr);

    appControllerPtr->triggerApplicationFocusRequest(appId);
}

TEST(TaskController, oomControllerUpdatesOomScoreAdjCorrectly)
{
    ProcessController::OomController oomController;

    auto child = core::posix::fork([]()
    {
        while (true);

        return core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    EXPECT_GT(child.pid(), 0);

    core::posix::linux::proc::process::OomScoreAdj oomScoreAdj;
    child >> oomScoreAdj;

    core::posix::linux::proc::process::OomScoreAdj likelyOomScoreAdj, unlikelyOomScoreAdj;
    {
        oomController.ensureProcessLikelyToBeKilled(child.pid());
        child >> likelyOomScoreAdj;

        EXPECT_GE(likelyOomScoreAdj.value, oomScoreAdj.value);
    }

    {
        oomController.ensureProcessUnlikelyToBeKilled(child.pid());
        child >> unlikelyOomScoreAdj;

        EXPECT_LE(unlikelyOomScoreAdj.value, likelyOomScoreAdj.value);
    }
}
