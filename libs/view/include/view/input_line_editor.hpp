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

#include <iostream>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>

namespace View {
    /**
     * @brief Maintains and renders the console input line.
     *
     * The input line is stored as Unicode code points and rendered as UTF-8, so
     * multi-byte characters (e.g. Cyrillic) are handled as single characters by
     * every editing operation. Cursor positions and word boundaries are counted
     * in characters; rendering assumes every character occupies one terminal
     * column (wide CJK characters are not supported).
     *
     * All state mutations and the corresponding terminal output are serialized
     * with an internal mutex. Console output paths (see \c ConsoleView::print)
     * use \c with_suspended to erase the rendered input line before writing and
     * redraw it afterwards, so asynchronous console output cannot visually
     * overwrite text that the user is typing.
     */
    class InputLineEditor final {
      public:
        /**
         * @brief Constructs a new InputLineEditor object.
         *
         * @param stream The output stream used to render the input line.
         */
        explicit InputLineEditor(std::ostream* stream = &std::cout);

        /// Move constructor.
        InputLineEditor(InputLineEditor&&) = delete;

        /// Copy constructor.
        InputLineEditor(const InputLineEditor&) = delete;

        /// Move assignment operator.
        InputLineEditor& operator=(InputLineEditor&&) = delete;

        /// Copy assignment operator.
        InputLineEditor& operator=(const InputLineEditor&) = delete;

        /**
         * @brief Inserts a single-byte character at the current cursor position.
         *
         * @param ch The character to insert.
         */
        void insert_char(char ch);

        /**
         * @brief Inserts UTF-8 encoded text at the current cursor position.
         *
         * Invalid UTF-8 sequences are skipped.
         *
         * @param text The UTF-8 encoded text to insert.
         */
        void insert_text(std::string_view text);

        /**
         * @brief Removes the character before the cursor.
         */
        void backspace();

        /**
         * @brief Removes the character at the cursor position.
         */
        void delete_char();

        /**
         * @brief Moves the cursor one character to the left.
         */
        void left();

        /**
         * @brief Moves the cursor one character to the right.
         */
        void right();

        /**
         * @brief Moves the cursor to the beginning of the line.
         */
        void home();

        /**
         * @brief Moves the cursor to the end of the line.
         */
        void end();

        /**
         * @brief Moves the cursor to the beginning of the word before the cursor.
         *
         * Words are delimited by whitespace.
         */
        void word_left();

        /**
         * @brief Moves the cursor to the end of the word after the cursor.
         *
         * Words are delimited by whitespace.
         */
        void word_right();

        /**
         * @brief Removes the word before the cursor.
         *
         * Any whitespace between the cursor and the word is removed as well.
         * Words are delimited by whitespace.
         */
        void kill_prev_word();

        /**
         * @brief Removes the word after the cursor.
         *
         * Any whitespace between the cursor and the word is removed as well.
         * Words are delimited by whitespace.
         */
        void kill_next_word();

        /**
         * @brief Replaces the whole input line (input history navigation).
         *
         * @param text The new input line (UTF-8 encoded).
         */
        void set_line(std::string_view text);

        /**
         * @brief Returns a copy of the current input line.
         *
         * @return The current input line (UTF-8 encoded).
         */
        [[nodiscard]] std::string line() const;

        /**
         * @brief Terminates the input line and returns its contents.
         *
         * @return The submitted input line (UTF-8 encoded).
         */
        std::string submit_line();

        /**
         * @brief Runs the given function with the input line temporarily erased from the screen.
         *
         * The input line state is preserved; it is redrawn (with the cursor restored) after
         * the function returns. When the input line is empty, the function output is written
         * as is.
         *
         * @param fn The function to invoke while the input line is suspended.
         */
        template <typename Function> void with_suspended(Function&& fn)
        {
            const std::lock_guard lock{mutex_};

            if (!line_.empty()) {
                *stream_ << "\r\x1B[K";
            }

            fn();

            if (!line_.empty()) {
                redraw();
            }

            stream_->flush();
        }

      private:
        /**
         * @brief Redraws the input line and restores the cursor position.
         *
         * The mutex must be held by the caller.
         */
        void redraw();

        /**
         * @brief Prints the part of the input line after the cursor.
         *
         * The mutex must be held by the caller.
         */
        void print_tail();

        /// Guards the input line state and rendering output.
        mutable std::mutex mutex_{};

        /// The output stream used to render the input line.
        std::ostream* stream_{};

        /// The current input line (one element per character).
        std::u32string line_{};

        /// The current cursor position in the input line (in characters).
        std::u32string::size_type cursor_{};
    };
}
