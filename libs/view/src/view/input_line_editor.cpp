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
#include "view/utf8.hpp"
#include <cassert>

namespace {
    /// Checks whether the character is a word delimiter.
    bool is_space(const char32_t ch)
    {
        return (U' ' == ch) || (U'\t' == ch);
    }
}

namespace View {
    InputLineEditor::InputLineEditor(std::ostream* stream) : stream_(stream)
    {
        assert(stream_ != nullptr);
    }

    void InputLineEditor::insert_char(const char ch)
    {
        insert_text(std::string_view{&ch, 1});
    }

    void InputLineEditor::insert_text(const std::string_view text)
    {
        if (text.empty()) {
            return;
        }

        const auto characters = utf8::decode(text);

        if (characters.empty()) {
            return;
        }

        const std::lock_guard lock{mutex_};

        line_.insert(cursor_, characters);
        print_tail();

        cursor_ += characters.length();

        const auto tail_length = line_.length() - cursor_;

        for (auto i = tail_length; i > 0; --i) {
            *stream_ << '\b';
        }
    }

    void InputLineEditor::backspace()
    {
        const std::lock_guard lock{mutex_};

        if ((0 == cursor_) || line_.empty()) {
            return;
        }

        --cursor_;
        line_.erase(cursor_, 1);

        *stream_ << '\b';
        print_tail();
        *stream_ << ' ';

        const auto tail_length = line_.length() - cursor_;

        for (auto i = tail_length + 1; i > 0; --i) {
            *stream_ << '\b';
        }
    }

    void InputLineEditor::delete_char()
    {
        const std::lock_guard lock{mutex_};

        if (cursor_ >= line_.length()) {
            return;
        }

        line_.erase(cursor_, 1);
        print_tail();
        *stream_ << ' ';

        const auto tail_length = line_.length() - cursor_;

        for (auto i = tail_length + 1; i > 0; --i) {
            *stream_ << '\b';
        }
    }

    void InputLineEditor::left()
    {
        const std::lock_guard lock{mutex_};

        if (0 == cursor_) {
            return;
        }

        *stream_ << '\b';
        --cursor_;
    }

    void InputLineEditor::right()
    {
        const std::lock_guard lock{mutex_};

        if (line_.length() == cursor_) {
            return;
        }

        utf8::append_to_stream(*stream_, line_[cursor_]);
        ++cursor_;
    }

    void InputLineEditor::home()
    {
        const std::lock_guard lock{mutex_};

        if (cursor_ != 0) {
            cursor_ = 0;
            *stream_ << '\r';
        }
    }

    void InputLineEditor::end()
    {
        const std::lock_guard lock{mutex_};

        print_tail();
        cursor_ = line_.length();
    }

    void InputLineEditor::word_left()
    {
        const std::lock_guard lock{mutex_};

        auto target = cursor_;

        while ((0 < target) && is_space(line_[target - 1])) {
            --target;
        }

        while ((0 < target) && !is_space(line_[target - 1])) {
            --target;
        }

        while (cursor_ > target) {
            *stream_ << '\b';
            --cursor_;
        }
    }

    void InputLineEditor::word_right()
    {
        const std::lock_guard lock{mutex_};

        const auto length = line_.length();
        auto target = cursor_;

        while ((target < length) && is_space(line_[target])) {
            ++target;
        }

        while ((target < length) && !is_space(line_[target])) {
            ++target;
        }

        while (cursor_ < target) {
            utf8::append_to_stream(*stream_, line_[cursor_]);
            ++cursor_;
        }
    }

    void InputLineEditor::kill_prev_word()
    {
        const std::lock_guard lock{mutex_};

        if (0 == cursor_) {
            return;
        }

        auto start = cursor_;

        while ((0 < start) && is_space(line_[start - 1])) {
            --start;
        }

        while ((0 < start) && !is_space(line_[start - 1])) {
            --start;
        }

        if (start == cursor_) {
            return;
        }

        const auto removed = cursor_ - start;
        line_.erase(start, removed);
        cursor_ = start;

        for (auto i = removed; i > 0; --i) {
            *stream_ << '\b';
        }

        print_tail();

        for (auto i = removed; i > 0; --i) {
            *stream_ << ' ';
        }

        const auto tail_length = line_.length() - cursor_;

        for (auto i = tail_length + removed; i > 0; --i) {
            *stream_ << '\b';
        }
    }

    void InputLineEditor::kill_next_word()
    {
        const std::lock_guard lock{mutex_};

        const auto length = line_.length();

        if (cursor_ >= length) {
            return;
        }

        auto end = cursor_;

        while ((end < length) && is_space(line_[end])) {
            ++end;
        }

        while ((end < length) && !is_space(line_[end])) {
            ++end;
        }

        if (end == cursor_) {
            return;
        }

        const auto removed = end - cursor_;
        line_.erase(cursor_, removed);
        print_tail();

        for (auto i = removed; i > 0; --i) {
            *stream_ << ' ';
        }

        const auto tail_length = line_.length() - cursor_;

        for (auto i = tail_length + removed; i > 0; --i) {
            *stream_ << '\b';
        }
    }

    void InputLineEditor::set_line(const std::string_view text)
    {
        const auto characters = utf8::decode(text);

        const std::lock_guard lock{mutex_};

        if (!line_.empty()) {
            *stream_ << "\r\x1B[K";
        }

        line_ = characters;
        cursor_ = line_.length();
        *stream_ << utf8::encode(line_);
    }

    std::string InputLineEditor::line() const
    {
        const std::lock_guard lock{mutex_};
        return utf8::encode(line_);
    }

    std::string InputLineEditor::submit_line()
    {
        const std::lock_guard lock{mutex_};

        *stream_ << '\n';

        auto input = std::move(line_);
        line_.clear();
        cursor_ = 0;

        return utf8::encode(input);
    }

    void InputLineEditor::redraw()
    {
        *stream_ << '\r' << utf8::encode(line_) << '\r';

        if (cursor_ > 0) {
            *stream_ << "\x1B[" << cursor_ << 'C';
        }
    }

    void InputLineEditor::print_tail()
    {
        *stream_ << utf8::encode(line_.substr(cursor_));
    }
}
