/*
 * Copyright (C) 2014 Canonical, Ltd.
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

#include <Unity/Application/desktopfilereader.h>

#include <gtest/gtest.h>

#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QtGlobal>
#include <QString>

using namespace qtmir;

namespace {
    static void setLocale(const char *locale)
    {
        qputenv("LANGUAGE", locale);
        qputenv("LC_ALL", locale); // set these for GIO
        QLocale::setDefault(QString(locale)); // set for Qt
    }
}

TEST(DesktopFileReader, testReadsDesktopFile)
{
    using namespace ::testing;
    setLocale("C");

    QFileInfo fileInfo(QString(TEST_SOURCE_DIR) + "/calculator.desktop");
    DesktopFileReader::Factory readerFactory;
    DesktopFileReader *reader = readerFactory.createInstance("calculator", fileInfo);

    EXPECT_EQ(reader->loaded(), true);
    EXPECT_EQ(reader->appId(), "calculator");
    EXPECT_EQ(reader->name(), "Calculator");
    EXPECT_EQ(reader->exec(), "yes -p com.ubuntu.calculator_calculator_1.3.329 -- qmlscene -qt5 ubuntu-calculator-app.qml");
    EXPECT_EQ(reader->icon(), "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calculator/calculator-app@30.png");
    EXPECT_EQ(reader->path(), "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calculator");
    EXPECT_EQ(reader->comment(), "A simple calculator for Ubuntu.");
    EXPECT_EQ(reader->stageHint(), "SideStage");
    EXPECT_EQ(reader->splashColor(), "#aabbcc");
    EXPECT_EQ(reader->splashColorFooter(), "#deadbeefda");
    EXPECT_EQ(reader->splashColorHeader(), "purple");
    EXPECT_EQ(reader->splashImage(), "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calculator/calculator-app@30.png");
    EXPECT_EQ(reader->splashShowHeader(), "True");
    EXPECT_EQ(reader->splashTitle(), "Calculator 2.0");
}

TEST(DesktopFileReader, testReadsLocalizedDesktopFile)
{
    using namespace ::testing;
    setLocale("de");

    QFileInfo fileInfo(QString(TEST_SOURCE_DIR) + "/calculator.desktop");
    DesktopFileReader::Factory readerFactory;
    DesktopFileReader *reader = readerFactory.createInstance("calculator", fileInfo);

    EXPECT_EQ(reader->loaded(), true);
    EXPECT_EQ(reader->appId(), "calculator");
    EXPECT_EQ(reader->name(), "Taschenrechner");
    EXPECT_EQ(reader->exec(), "yes -p com.ubuntu.calculator_calculator_1.3.329 -- qmlscene -qt5 ubuntu-calculator-app.qml");
    EXPECT_EQ(reader->icon(), "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calculator/calculator-app@30.png");
    EXPECT_EQ(reader->path(), "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calculator");
    EXPECT_EQ(reader->comment(), "Ein einfach Tachenrechner für Ubuntu.");
    EXPECT_EQ(reader->stageHint(), "SideStage");
    EXPECT_EQ(reader->splashColor(), "#aabbcc");
    EXPECT_EQ(reader->splashColorFooter(), "#deadbeefda");
    EXPECT_EQ(reader->splashColorHeader(), "purple");
    EXPECT_EQ(reader->splashImage(), "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calculator/calculator-app@30.png");
    EXPECT_EQ(reader->splashShowHeader(), "True");
    EXPECT_EQ(reader->splashTitle(), "Taschenrechner 2.0");
}

TEST(DesktopFileReader, testMissingDesktopFile)
{
    using namespace ::testing;
    setLocale("C");

    QFileInfo fileInfo(QString(TEST_SOURCE_DIR) + "/missing.desktop");
    DesktopFileReader::Factory readerFactory;
    DesktopFileReader *reader = readerFactory.createInstance("calculator", fileInfo);

    EXPECT_EQ(reader->loaded(), false);
    EXPECT_EQ(reader->appId(), "calculator");
    EXPECT_EQ(reader->name(), "");
    EXPECT_EQ(reader->exec(), "");
    EXPECT_EQ(reader->icon(), "");
    EXPECT_EQ(reader->path(), "");
    EXPECT_EQ(reader->comment(), "");
    EXPECT_EQ(reader->stageHint(), "");
    EXPECT_EQ(reader->splashColor(), "");
    EXPECT_EQ(reader->splashColorFooter(), "");
    EXPECT_EQ(reader->splashColorHeader(), "");
    EXPECT_EQ(reader->splashImage(), "");
    EXPECT_EQ(reader->splashShowHeader(), "");
    EXPECT_EQ(reader->splashTitle(), "");
}

TEST(DesktopFileReader, testUTF8Characters)
{
    using namespace ::testing;
    setLocale("zh_CN");

    QFileInfo fileInfo(QString(TEST_SOURCE_DIR) + "/calculator.desktop");
    DesktopFileReader::Factory readerFactory;
    DesktopFileReader *reader = readerFactory.createInstance("calculator", fileInfo);

    EXPECT_EQ(reader->loaded(), true);
    EXPECT_EQ(reader->appId(), "calculator");
    EXPECT_EQ(reader->name(), "计算器");
    EXPECT_EQ(reader->comment(), "Ubuntu 简易计算器");
    EXPECT_EQ(reader->splashTitle(), "计算器 2.0");
}

TEST(DesktopFileReader, parseOrientations)
{
    using namespace ::testing;

    const Qt::ScreenOrientations defaultOrientations = Qt::PortraitOrientation | Qt::LandscapeOrientation
        | Qt::InvertedPortraitOrientation | Qt::InvertedLandscapeOrientation;
    bool ok;
    Qt::ScreenOrientations orientations;

    ok = DesktopFileReader::parseOrientations(QString(), orientations);
    ASSERT_TRUE(ok);
    EXPECT_EQ(defaultOrientations, orientations);

    ok = DesktopFileReader::parseOrientations("An invalid string!", orientations);
    ASSERT_FALSE(ok);
    EXPECT_EQ(defaultOrientations, orientations);

    ok = DesktopFileReader::parseOrientations("landscape", orientations);
    ASSERT_TRUE(ok);
    EXPECT_EQ(Qt::LandscapeOrientation, orientations);

    ok = DesktopFileReader::parseOrientations("   InvertedPortrait  , Portrait ", orientations);
    ASSERT_TRUE(ok);
    EXPECT_EQ(Qt::InvertedPortraitOrientation | Qt::PortraitOrientation, orientations);

    ok = DesktopFileReader::parseOrientations(",inverted-landscape, inverted_portrait, ", orientations);
    ASSERT_TRUE(ok);
    EXPECT_EQ(Qt::InvertedPortraitOrientation | Qt::InvertedLandscapeOrientation, orientations);

    ok = DesktopFileReader::parseOrientations(",inverted-landscape, some-invalid-text, ", orientations);
    ASSERT_FALSE(ok);
    EXPECT_EQ(defaultOrientations, orientations);

    ok = DesktopFileReader::parseOrientations("landscape;portrait", orientations);
    ASSERT_TRUE(ok);
    EXPECT_EQ(Qt::PortraitOrientation | Qt::LandscapeOrientation, orientations);

    ok = DesktopFileReader::parseOrientations("primary", orientations);
    ASSERT_TRUE(ok);
    EXPECT_EQ(Qt::PrimaryOrientation, orientations);

    ok = DesktopFileReader::parseOrientations("landscpe,primary", orientations);
    ASSERT_FALSE(ok);
    EXPECT_EQ(defaultOrientations, orientations);
}

TEST(DesktopFileReader, parseBoolean)
{
    using namespace ::testing;

    bool ok;
    bool boolean;

    ok = DesktopFileReader::parseBoolean(QString(), boolean);
    ASSERT_TRUE(ok);
    EXPECT_FALSE(boolean);

    ok = DesktopFileReader::parseBoolean("  Yes ", boolean);
    ASSERT_TRUE(ok);
    EXPECT_TRUE(boolean);

    ok = DesktopFileReader::parseBoolean("False", boolean);
    ASSERT_TRUE(ok);
    EXPECT_FALSE(boolean);

    ok = DesktopFileReader::parseBoolean("Hello World!", boolean);
    ASSERT_FALSE(ok);
    EXPECT_FALSE(boolean);
}
