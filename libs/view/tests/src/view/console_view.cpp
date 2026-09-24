/*
 * Half Life 1 SDK License
 * Copyright(c) Valve Corp
 *
 * DISCLAIMER OF WARRANTIES. THE HALF LIFE 1 SDK AND ANY OTHER MATERIAL
 * DOWNLOADED BY LICENSEE IS PROVIDED "AS IS". VALVE AND ITS SUPPLIERS
 * DISCLAIM ALL WARRANTIES WITH RESPECT TO THE SDK, EITHER EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY,
 * NON-INFRINGEMENT, TITLE AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * LIMITATION OF LIABILITY. IN NO EVENT SHALL VALVE OR ITS SUPPLIERS BE LIABLE
 * FOR ANY SPECIAL, INCIDENTAL, INDIRECT, CONSEQUENTIAL DAMAGES WHATSOEVER
 * (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
 * BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY
 * LOSS) ARISING OUT OF THE USE OF OR INABILITY TO USE THE ENGINE AND/OR THE
 * SDK, EVEN IF VALVE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 * For commercial use, contact: sourceengine@valvesoftware.com
 */

#include "view/console_view.hpp"

#ifndef _WIN32

#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>

namespace {
    class ConsoleViewTest : public testing::Test {
      protected:
        std::ostringstream stream{};
        std::shared_ptr<View::InputLineEditor> editor{std::make_shared<View::InputLineEditor>(&stream)};
        View::ConsoleView view{editor};
    };

    TEST_F(ConsoleViewTest, FragmentedEchoIsAssembledIntoOneLine)
    {
        view.print("Executing ");
        view.print("Admins ");
        view.print("Kit ");
        view.print("Configuration ");
        view.print("File");
        view.print("\n");

        EXPECT_EQ(stream.str(), "Executing Admins Kit Configuration File\n");
    }

    TEST_F(ConsoleViewTest, QuotedEchoProducesNoBlankLine)
    {
        view.print("test test ");
        view.print("\n");

        EXPECT_EQ(stream.str(), "test test \n");
    }

    TEST_F(ConsoleViewTest, CompleteLinesPassThrough)
    {
        view.print("first\nsecond\n");

        EXPECT_EQ(stream.str(), "first\nsecond\n");
    }

    TEST_F(ConsoleViewTest, GenuineEmptyLineIsPrinted)
    {
        view.print("hello\n");
        view.print("\n");

        EXPECT_EQ(stream.str(), "hello\n\n");
    }

    TEST_F(ConsoleViewTest, UnterminatedFragmentIsHeld)
    {
        view.print("partial line without newline");

        EXPECT_EQ(stream.str(), "");
    }

    TEST_F(ConsoleViewTest, HeldFragmentIsFlushedWhenLineCompletes)
    {
        view.print("partial ");
        view.print("line\n");
        view.print("next\n");

        EXPECT_EQ(stream.str(), "partial line\nnext\n");
    }

    TEST_F(ConsoleViewTest, InputLineIsPreservedAcrossOutput)
    {
        editor->insert_text("cmd");

        view.print("log line\n");

        EXPECT_EQ(editor->line(), "cmd");
        EXPECT_NE(stream.str().find("\x1B[K"), std::string::npos);
        EXPECT_NE(stream.str().find("log line"), std::string::npos);
        EXPECT_LT(stream.str().find("log line"), stream.str().rfind("cmd"));
    }

    TEST_F(ConsoleViewTest, LongUnterminatedOutputIsFlushed)
    {
        const auto chunk = std::string(4096, 'x');

        view.print(chunk);
        view.print(chunk);
        view.print(chunk);

        EXPECT_GT(stream.str().size(), 8192U);
        EXPECT_EQ(stream.str().back(), '\n');
    }
}

#endif // _WIN32
