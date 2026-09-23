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

#include "view/input_line_editor.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <string>

namespace {
    class InputLineEditorTest : public testing::Test {
      protected:
        std::ostringstream stream{};
        View::InputLineEditor editor{&stream};
    };

    TEST_F(InputLineEditorTest, InsertCharAppendsToEmptyLine)
    {
        editor.insert_char('h');
        editor.insert_char('i');

        EXPECT_EQ(editor.line(), "hi");
        EXPECT_EQ(stream.str(), "hi");
    }

    TEST_F(InputLineEditorTest, InsertTextAtCursor)
    {
        editor.insert_text("ab");
        editor.left();
        stream.str("");

        editor.insert_char('c');

        EXPECT_EQ(editor.line(), "acb");
        EXPECT_NE(stream.str().find("cb"), std::string::npos);
    }

    TEST_F(InputLineEditorTest, BackspaceRemovesCharBeforeCursor)
    {
        editor.insert_text("abc");

        editor.backspace();

        EXPECT_EQ(editor.line(), "ab");
    }

    TEST_F(InputLineEditorTest, BackspaceOnEmptyLineDoesNothing)
    {
        editor.backspace();

        EXPECT_EQ(editor.line(), "");
        EXPECT_EQ(stream.str(), "");
    }

    TEST_F(InputLineEditorTest, DeleteCharRemovesCharAtCursor)
    {
        editor.insert_text("abc");
        editor.home();

        editor.delete_char();

        EXPECT_EQ(editor.line(), "bc");
    }

    TEST_F(InputLineEditorTest, HomeThenInsertPrepends)
    {
        editor.insert_text("ab");

        editor.home();
        editor.insert_char('c');

        EXPECT_EQ(editor.line(), "cab");
    }

    TEST_F(InputLineEditorTest, EndMovesCursorToLineEnd)
    {
        editor.insert_text("ab");
        editor.home();

        editor.end();
        editor.insert_char('c');

        EXPECT_EQ(editor.line(), "abc");
    }

    TEST_F(InputLineEditorTest, SetLineReplacesContent)
    {
        editor.insert_text("old");

        editor.set_line("new");

        EXPECT_EQ(editor.line(), "new");
        EXPECT_NE(stream.str().find("new"), std::string::npos);
    }

    TEST_F(InputLineEditorTest, SetLineToEmptyClearsScreenRow)
    {
        editor.insert_text("old");
        stream.str("");

        editor.set_line("");

        EXPECT_EQ(editor.line(), "");
        EXPECT_NE(stream.str().find("\x1B[K"), std::string::npos);
    }

    TEST_F(InputLineEditorTest, SubmitLineReturnsAndClears)
    {
        editor.insert_text("cmd");

        EXPECT_EQ(editor.submit_line(), "cmd");
        EXPECT_EQ(editor.line(), "");
        EXPECT_NE(stream.str().find("\n"), std::string::npos);
    }

    TEST_F(InputLineEditorTest, WithSuspendedErasesAndRedrawsLine)
    {
        editor.insert_text("cmd");
        stream.str("");

        editor.with_suspended([] {
        });

        const auto output = stream.str();
        EXPECT_NE(output.find("\x1B[K"), std::string::npos);
        EXPECT_NE(output.find("cmd"), std::string::npos);
    }

    TEST_F(InputLineEditorTest, WithSuspendedRestoresCursor)
    {
        editor.insert_text("abcd");
        editor.left();
        editor.left();
        stream.str("");

        editor.with_suspended([] {
        });

        EXPECT_NE(stream.str().find("\x1B[2C"), std::string::npos);
    }

    TEST_F(InputLineEditorTest, WithSuspendedWritesOutputBetweenEraseAndRedraw)
    {
        editor.insert_text("cmd");
        stream.str("");

        editor.with_suspended([this] {
            stream << "marker";
        });

        const auto output = stream.str();
        EXPECT_LT(output.find("\x1B[K"), output.find("marker"));
        EXPECT_LT(output.find("marker"), output.rfind("cmd"));
    }

    TEST_F(InputLineEditorTest, WithSuspendedOnEmptyLineWritesPlainly)
    {
        editor.with_suspended([this] {
            stream << "plain";
        });

        EXPECT_EQ(stream.str(), "plain");
    }

    TEST_F(InputLineEditorTest, WordLeftJumpsToWordStart)
    {
        editor.insert_text("foo bar");

        editor.word_left();
        editor.insert_char('X');

        EXPECT_EQ(editor.line(), "foo Xbar");
    }

    TEST_F(InputLineEditorTest, WordLeftSkipsSpacesBeforeWord)
    {
        editor.insert_text("foo  bar");

        editor.word_left();
        editor.insert_char('X');

        EXPECT_EQ(editor.line(), "foo  Xbar");
    }

    TEST_F(InputLineEditorTest, WordLeftAtLineStartDoesNothing)
    {
        editor.insert_text("foo bar");
        editor.home();

        editor.word_left();
        editor.insert_char('X');

        EXPECT_EQ(editor.line(), "Xfoo bar");
    }

    TEST_F(InputLineEditorTest, WordRightJumpsPastWord)
    {
        editor.insert_text("foo bar");
        editor.home();

        editor.word_right();
        editor.insert_char('X');

        EXPECT_EQ(editor.line(), "fooX bar");
    }

    TEST_F(InputLineEditorTest, WordRightSkipsSpacesAfterCursor)
    {
        editor.insert_text("foo  bar");
        editor.home();

        editor.word_right();
        editor.word_right();
        editor.insert_char('X');

        EXPECT_EQ(editor.line(), "foo  barX");
    }

    TEST_F(InputLineEditorTest, WordRightAtLineEndDoesNothing)
    {
        editor.insert_text("foo bar");

        editor.word_right();
        editor.insert_char('X');

        EXPECT_EQ(editor.line(), "foo barX");
    }

    TEST_F(InputLineEditorTest, KillPrevWordRemovesWordBeforeCursor)
    {
        editor.insert_text("foo bar");

        editor.kill_prev_word();

        EXPECT_EQ(editor.line(), "foo ");
    }

    TEST_F(InputLineEditorTest, KillPrevWordEatsSpacesBeforeCursor)
    {
        editor.insert_text("foo bar  ");

        editor.kill_prev_word();

        EXPECT_EQ(editor.line(), "foo ");
    }

    TEST_F(InputLineEditorTest, KillPrevWordOnEmptyLineDoesNothing)
    {
        editor.kill_prev_word();

        EXPECT_EQ(editor.line(), "");
        EXPECT_EQ(stream.str(), "");
    }

    TEST_F(InputLineEditorTest, KillNextWordRemovesWordAfterCursor)
    {
        editor.insert_text("foo bar");
        editor.home();

        editor.kill_next_word();

        EXPECT_EQ(editor.line(), " bar");
    }

    TEST_F(InputLineEditorTest, KillNextWordEatsSpacesAfterCursor)
    {
        editor.insert_text("foo  bar");
        editor.home();
        editor.word_right();

        editor.kill_next_word();

        EXPECT_EQ(editor.line(), "foo");
    }

    TEST_F(InputLineEditorTest, KillNextWordAtLineEndDoesNothing)
    {
        editor.insert_text("foo bar");

        editor.kill_next_word();

        EXPECT_EQ(editor.line(), "foo bar");
    }

    TEST_F(InputLineEditorTest, InsertCyrillicRoundtrip)
    {
        editor.insert_text("привет");

        EXPECT_EQ(editor.line(), "привет");
        EXPECT_NE(stream.str().find("привет"), std::string::npos);
    }

    TEST_F(InputLineEditorTest, InsertMixedAsciiAndCyrillic)
    {
        editor.insert_text("ban: причина 1");

        EXPECT_EQ(editor.line(), "ban: причина 1");
    }

    TEST_F(InputLineEditorTest, BackspaceRemovesWholeCyrillicChar)
    {
        editor.insert_text("прив");

        editor.backspace();

        EXPECT_EQ(editor.line(), "при");
    }

    TEST_F(InputLineEditorTest, CursorMovementCountsCodepoints)
    {
        editor.insert_text("привет");

        editor.left();
        editor.left();
        editor.insert_char('X');

        EXPECT_EQ(editor.line(), "привXет");
    }

    TEST_F(InputLineEditorTest, InsertTextAtCursorInsideCyrillic)
    {
        editor.insert_text("привет");

        editor.left();
        editor.left();
        editor.insert_text("XY");

        EXPECT_EQ(editor.line(), "привXYет");
    }

    TEST_F(InputLineEditorTest, DeleteCharRemovesWholeCyrillicChar)
    {
        editor.insert_text("привет");
        editor.home();

        editor.delete_char();

        EXPECT_EQ(editor.line(), "ривет");
    }

    TEST_F(InputLineEditorTest, FourByteCharRoundtripAndBackspace)
    {
        const auto emoji = std::string{"\xF0\x9F\x98\x80"};

        editor.insert_text(emoji);
        EXPECT_EQ(editor.line(), emoji);

        editor.backspace();
        EXPECT_EQ(editor.line(), "");
    }

    TEST_F(InputLineEditorTest, WordLeftStopsAtCyrillicWordStart)
    {
        editor.insert_text("причина тест");

        editor.word_left();
        editor.insert_char('X');

        EXPECT_EQ(editor.line(), "причина Xтест");
    }

    TEST_F(InputLineEditorTest, KillPrevWordWithCyrillic)
    {
        editor.insert_text("ban причина");

        editor.kill_prev_word();

        EXPECT_EQ(editor.line(), "ban ");
    }

    TEST_F(InputLineEditorTest, InvalidUtf8SequencesAreDropped)
    {
        editor.insert_text("ok");
        stream.str("");

        editor.insert_text("\xD0");
        editor.insert_text("\xFF");
        editor.insert_text("\xD0\x28");
        editor.insert_text("\x80");

        EXPECT_EQ(editor.line(), "ok");
        EXPECT_EQ(stream.str(), "");
    }

    TEST_F(InputLineEditorTest, OverlongAndSurrogateSequencesAreDropped)
    {
        editor.insert_text("\xC0\x80");
        editor.insert_text("\xED\xA0\x80");

        EXPECT_EQ(editor.line(), "");
    }

    TEST_F(InputLineEditorTest, SetLineWithCyrillic)
    {
        editor.set_line("тест 123");

        EXPECT_EQ(editor.line(), "тест 123");
    }

    TEST_F(InputLineEditorTest, SubmitLineWithCyrillic)
    {
        editor.insert_text("тест");

        EXPECT_EQ(editor.submit_line(), "тест");
        EXPECT_EQ(editor.line(), "");
    }

    TEST_F(InputLineEditorTest, SuspendedRestoreCountsCodepoints)
    {
        editor.insert_text("привет");

        editor.left();
        editor.left();
        stream.str("");

        editor.with_suspended([] {
        });

        EXPECT_NE(stream.str().find("\x1B[4C"), std::string::npos);
    }
}
