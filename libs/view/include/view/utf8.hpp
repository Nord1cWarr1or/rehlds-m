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

#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>

namespace View::utf8 {
    /**
     * @brief Determines the length of a UTF-8 sequence from its lead byte.
     *
     * @param lead The lead byte of the sequence.
     *
     * @return The sequence length (1-4), or 0 if the byte cannot start a sequence.
     */
    [[nodiscard]] inline std::size_t sequence_length(const unsigned char lead)
    {
        if (lead < 0x80U) {
            return 1;
        }

        if ((lead & 0xE0U) == 0xC0U) {
            return 2;
        }

        if ((lead & 0xF0U) == 0xE0U) {
            return 3;
        }

        if ((lead & 0xF8U) == 0xF0U) {
            return 4;
        }

        return 0;
    }

    /**
     * @brief Decodes UTF-8 encoded text into Unicode code points.
     *
     * Invalid, truncated, overlong, surrogate and out-of-range sequences are skipped.
     *
     * @param text The UTF-8 encoded text.
     *
     * @return The decoded code points.
     */
    [[nodiscard]] inline std::u32string decode(const std::string_view text)
    {
        std::u32string result;

        for (std::size_t i = 0; i < text.size();) {
            const auto lead = static_cast<unsigned char>(text[i]);
            const auto length = sequence_length(lead);

            if ((0 == length) || ((i + length) > text.size())) {
                ++i;
                continue;
            }

            auto codepoint = static_cast<char32_t>(lead);

            if (length > 1) {
                codepoint = static_cast<char32_t>(lead & (0xFFU >> (length + 1U)));
            }

            auto valid = true;

            for (std::size_t j = 1; j < length; ++j) {
                const auto continuation = static_cast<unsigned char>(text[i + j]);

                if ((continuation & 0xC0U) != 0x80U) {
                    valid = false;
                    break;
                }

                codepoint = (codepoint << 6U) | static_cast<char32_t>(continuation & 0x3FU);
            }

            const auto minimal = (1U == length) ? 0x0U : (0x1U << ((length - 1U) * 5U));

            if (valid && (codepoint >= minimal) && (codepoint <= 0x10FFFFU)
                && !((codepoint >= 0xD800U) && (codepoint <= 0xDFFFU))) {
                result.push_back(codepoint);
            }

            i += length;
        }

        return result;
    }

    /**
     * @brief Appends the UTF-8 encoding of a code point to the output string.
     *
     * @param output The string to append the encoding to.
     * @param codepoint The code point to encode.
     */
    inline void append(std::string& output, const char32_t codepoint)
    {
        if (codepoint < 0x80U) {
            output.push_back(static_cast<std::string::value_type>(codepoint));
            return;
        }

        if (codepoint < 0x800U) {
            output.push_back(static_cast<std::string::value_type>(0xC0U | (codepoint >> 6U)));
            output.push_back(static_cast<std::string::value_type>(0x80U | (codepoint & 0x3FU)));
            return;
        }

        if (codepoint < 0x10000U) {
            output.push_back(static_cast<std::string::value_type>(0xE0U | (codepoint >> 12U)));
            output.push_back(static_cast<std::string::value_type>(0x80U | ((codepoint >> 6U) & 0x3FU)));
            output.push_back(static_cast<std::string::value_type>(0x80U | (codepoint & 0x3FU)));
            return;
        }

        output.push_back(static_cast<std::string::value_type>(0xF0U | (codepoint >> 18U)));
        output.push_back(static_cast<std::string::value_type>(0x80U | ((codepoint >> 12U) & 0x3FU)));
        output.push_back(static_cast<std::string::value_type>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        output.push_back(static_cast<std::string::value_type>(0x80U | (codepoint & 0x3FU)));
    }

    /**
     * @brief Encodes Unicode code points as UTF-8.
     *
     * @param text The code points to encode.
     *
     * @return The UTF-8 encoded text.
     */
    [[nodiscard]] inline std::string encode(const std::u32string& text)
    {
        std::string output;

        for (const auto codepoint : text) {
            append(output, codepoint);
        }

        return output;
    }

    /**
     * @brief Writes the UTF-8 encoding of a code point to a stream.
     *
     * @param stream The stream to write the encoding to.
     * @param codepoint The code point to encode.
     */
    inline void append_to_stream(std::ostream& stream, const char32_t codepoint)
    {
        std::string bytes;
        append(bytes, codepoint);
        stream << bytes;
    }
}
