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

#include "view/utf8.hpp"
#include <gtest/gtest.h>

namespace {
    TEST(Utf8Test, SequenceLength)
    {
        EXPECT_EQ(View::utf8::sequence_length(0x41), 1);
        EXPECT_EQ(View::utf8::sequence_length(0xC3), 2);
        EXPECT_EQ(View::utf8::sequence_length(0xD0), 2);
        EXPECT_EQ(View::utf8::sequence_length(0xE2), 3);
        EXPECT_EQ(View::utf8::sequence_length(0xF0), 4);
        EXPECT_EQ(View::utf8::sequence_length(0x80), 0);
        EXPECT_EQ(View::utf8::sequence_length(0xBF), 0);
        EXPECT_EQ(View::utf8::sequence_length(0xF8), 0);
        EXPECT_EQ(View::utf8::sequence_length(0xFF), 0);
    }

    TEST(Utf8Test, DecodeAscii)
    {
        EXPECT_EQ(View::utf8::decode("abc 123"), U"abc 123");
    }

    TEST(Utf8Test, DecodeTwoByteSequence)
    {
        EXPECT_EQ(View::utf8::decode("\xD0\xBF"), U"\x43F");
        EXPECT_EQ(View::utf8::decode("привет"), U"привет");
    }

    TEST(Utf8Test, DecodeThreeByteSequence)
    {
        EXPECT_EQ(View::utf8::decode("\xE2\x82\xAC"), U"\x20AC");
    }

    TEST(Utf8Test, DecodeFourByteSequence)
    {
        EXPECT_EQ(View::utf8::decode("\xF0\x9F\x98\x80"), U"\x1F600");
    }

    TEST(Utf8Test, DecodeDropsInvalidSequences)
    {
        EXPECT_EQ(View::utf8::decode("\xFF"), U"");
        EXPECT_EQ(View::utf8::decode("\x80"), U"");
        EXPECT_EQ(View::utf8::decode("\xD0"), U"");
        EXPECT_EQ(View::utf8::decode("\xD0\x28"), U"");
        EXPECT_EQ(View::utf8::decode("ok\xFF"), U"ok");
    }

    TEST(Utf8Test, DecodeDropsOverlongSequences)
    {
        EXPECT_EQ(View::utf8::decode("\xC0\x80"), U"");
        EXPECT_EQ(View::utf8::decode("\xE0\x80\x80"), U"");
    }

    TEST(Utf8Test, DecodeDropsSurrogates)
    {
        EXPECT_EQ(View::utf8::decode("\xED\xA0\x80"), U"");
    }

    TEST(Utf8Test, EncodeDecodeRoundtrip)
    {
        const auto text = std::string{"cmd причина 2 😀"};

        EXPECT_EQ(View::utf8::encode(View::utf8::decode(text)), text);
    }

    TEST(Utf8Test, EncodeSingleCodepoints)
    {
        std::string output;

        View::utf8::append(output, U'a');
        View::utf8::append(output, U'\x44F');
        View::utf8::append(output, U'\x20AC');
        View::utf8::append(output, U'\x1F600');

        EXPECT_EQ(output, std::string{"a\xD1\x8F\xE2\x82\xAC\xF0\x9F\x98\x80"});
    }
}
