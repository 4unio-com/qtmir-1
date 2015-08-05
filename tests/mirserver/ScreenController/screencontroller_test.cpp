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

#include <gtest/gtest.h>
#include "gmock_fixes.h"

#include "stub_display.h"
#include "mock_main_loop.h"
#include "qtcompositor.h"
#include "fake_displayconfigurationoutput.h"

#include "testable_screencontroller.h"
#include "screen.h"
#include "screenwindow.h"

#include <QGuiApplication>

using namespace ::testing;

namespace mg = mir::graphics;
namespace geom = mir::geometry;

class ScreenControllerTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    ScreenController *sc;
    std::shared_ptr<StubDisplay> display;
    std::shared_ptr<QtCompositor> compositor;
    QGuiApplication *app;
};

void ScreenControllerTest::SetUp()
{
    setenv("QT_QPA_PLATFORM", "minimal", 1);
    Screen::skipDBusRegistration = true;

    sc = new TestableScreenController;
    display = std::make_shared<StubDisplay>();
    compositor = std::make_shared<QtCompositor>();
    auto mainLoop = std::make_shared<MockMainLoop>();

    EXPECT_CALL(*display, register_configuration_change_handler(_,_))
        .Times(1);

    static_cast<TestableScreenController*>(sc)->do_init(display, compositor, mainLoop);

    int argc = 0;
    char **argv = nullptr;
    setenv("QT_QPA_PLATFORM", "minimal", 1);
    app = new QGuiApplication(argc, argv);
}

void ScreenControllerTest::TearDown()
{
    delete sc;
}

TEST_F(ScreenControllerTest, SingleScreenFound)
{
    // Set up display state
    std::vector<mg::DisplayConfigurationOutput> config{fake_output1};
    std::vector<MockDisplayBuffer*> bufferConfig; // only used to match buffer with display, unecessary here
    display->setFakeConfiguration(config, bufferConfig);

    sc->update();

    ASSERT_EQ(sc->screens().count(), 1);
    Screen* screen = sc->screens().first();
    EXPECT_EQ(screen->geometry(), QRect(0, 0, 150, 200));
}

TEST_F(ScreenControllerTest, MultipleScreenFound)
{
    std::vector<mg::DisplayConfigurationOutput> config{fake_output1, fake_output2};
    std::vector<MockDisplayBuffer*> bufferConfig; // only used to match buffer with display, unecessary here
    display->setFakeConfiguration(config, bufferConfig);

    sc->update();

    ASSERT_EQ(sc->screens().count(), 2);
    EXPECT_EQ(sc->screens().at(0)->geometry(), QRect(0, 0, 150, 200));
    EXPECT_EQ(sc->screens().at(1)->geometry(), QRect(500, 600, 1500, 2000));
}

TEST_F(ScreenControllerTest, ScreenAdded)
{
    std::vector<mg::DisplayConfigurationOutput> config{fake_output1};
    std::vector<MockDisplayBuffer*> bufferConfig; // only used to match buffer with display, unecessary here
    display->setFakeConfiguration(config, bufferConfig);

    sc->update();

    config.push_back(fake_output2);
    display->setFakeConfiguration(config, bufferConfig);

    ASSERT_EQ(sc->screens().count(), 1);
    EXPECT_EQ(sc->screens().at(0)->geometry(), QRect(0, 0, 150, 200));

    sc->update();

    ASSERT_EQ(sc->screens().count(), 2);
    EXPECT_EQ(sc->screens().at(0)->geometry(), QRect(0, 0, 150, 200));
    EXPECT_EQ(sc->screens().at(1)->geometry(), QRect(500, 600, 1500, 2000));
}

TEST_F(ScreenControllerTest, ScreenRemoved)
{
    std::vector<mg::DisplayConfigurationOutput> config{fake_output2, fake_output1};
    std::vector<MockDisplayBuffer*> bufferConfig; // only used to match buffer with display, unecessary here
    display->setFakeConfiguration(config, bufferConfig);

    sc->update();

    config.pop_back();
    display->setFakeConfiguration(config, bufferConfig);

    ASSERT_EQ(sc->screens().count(), 2);
    EXPECT_EQ(sc->screens().at(0)->geometry(), QRect(500, 600, 1500, 2000));
    EXPECT_EQ(sc->screens().at(1)->geometry(), QRect(0, 0, 150, 200));

    sc->update();

    ASSERT_EQ(sc->screens().count(), 1);
    EXPECT_EQ(sc->screens().at(0)->geometry(), QRect(500, 600, 1500, 2000));
}

TEST_F(ScreenControllerTest, CheckPrioritizedGetUnusedScreen)
{
    std::vector<mg::DisplayConfigurationOutput> config{fake_output2, fake_output1};
    std::vector<MockDisplayBuffer*> bufferConfig; // only used to match buffer with display, unecessary here
    display->setFakeConfiguration(config, bufferConfig);

    sc->update();

    auto screen = sc->getUnusedScreen();
    EXPECT_EQ(screen->outputType(), mg::DisplayConfigurationOutputType::lvds);
}

TEST_F(ScreenControllerTest, MatchBufferWithDisplay)
{
    std::vector<mg::DisplayConfigurationOutput> config{fake_output1};
    MockDisplayBuffer buffer1;
    std::vector<MockDisplayBuffer*> buffers {&buffer1};

    geom::Rectangle buffer1Geom{{0, 0}, {150, 200}};
    EXPECT_CALL(buffer1, view_area())
            .WillRepeatedly(Return(buffer1Geom));

    display->setFakeConfiguration(config, buffers);
    sc->update();

    ASSERT_EQ(sc->screens().count(), 1);
    EXPECT_CALL(buffer1, make_current());
    static_cast<StubScreen*>(sc->screens().at(0))->makeCurrent();
}

TEST_F(ScreenControllerTest, MultipleMatchBuffersWithDisplays)
{
    std::vector<mg::DisplayConfigurationOutput> config{fake_output1, fake_output2};
    MockDisplayBuffer buffer1, buffer2;
    std::vector<MockDisplayBuffer*> buffers {&buffer1, &buffer2};

    geom::Rectangle buffer1Geom{{500, 600}, {1500, 2000}};
    geom::Rectangle buffer2Geom{{0, 0}, {150, 200}};
    EXPECT_CALL(buffer1, view_area())
            .WillRepeatedly(Return(buffer1Geom));
    EXPECT_CALL(buffer2, view_area())
            .WillRepeatedly(Return(buffer2Geom));

    display->setFakeConfiguration(config, buffers);
    sc->update();

    ASSERT_EQ(sc->screens().count(), 2);
    EXPECT_CALL(buffer1, make_current());
    EXPECT_CALL(buffer2, make_current());
    static_cast<StubScreen*>(sc->screens().at(0))->makeCurrent();
    static_cast<StubScreen*>(sc->screens().at(1))->makeCurrent();
}