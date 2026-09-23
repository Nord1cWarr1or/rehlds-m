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
#include <cassert>
#include <cctype>

namespace {
    /// Checks whether the character is a word delimiter.
    bool is_space(const char ch)
    {
        return std::isspace(static_cast<unsigned char>(ch)) != 0;
    }
}

namespace View {
    InputLineEditor::InputLineEditor(std::ostream* stream) : stream_(stream)
    {
        assert(stream_ != nullptr);
    }

    void InputLineEditor::insert_char(const char ch)
    {
        const std::lock_guard lock{mutex_};

        line_.insert(cursor_, 1, ch);
        *stream_ << (line_.c_str() + cursor_);
        ++cursor_;

        for (auto i = line_.length(); i > cursor_; --i) {
            *stream_ << '\b';
        }
    }

    void InputLineEditor::insert_text(const std::string_view text)
    {
        if (text.empty()) {
            return;
        }

        const std::lock_guard lock{mutex_};

        line_.insert(cursor_, text);
        *stream_ << (line_.c_str() + cursor_);
        cursor_ += text.length();

        for (auto i = line_.length(); i > cursor_; --i) {
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
        *stream_ << (line_.c_str() + cursor_);
        *stream_ << ' ';

        for (auto i = line_.length() + 1; i > cursor_; --i) {
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
        *stream_ << (line_.c_str() + cursor_);
        *stream_ << ' ';

        for (auto i = line_.length() + 1; i > cursor_; --i) {
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

        *stream_ << line_[cursor_];
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

        while (cursor_ < line_.length()) {
            *stream_ << line_[cursor_];
            ++cursor_;
        }
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
            *stream_ << line_[cursor_];
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

        *stream_ << (line_.c_str() + cursor_);

        for (auto i = removed; i > 0; --i) {
            *stream_ << ' ';
        }

        const auto total_length = line_.length();

        for (auto i = total_length + removed; i > cursor_; --i) {
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
        *stream_ << (line_.c_str() + cursor_);

        for (auto i = removed; i > 0; --i) {
            *stream_ << ' ';
        }

        const auto total_length = line_.length();

        for (auto i = total_length + removed; i > cursor_; --i) {
            *stream_ << '\b';
        }
    }

    void InputLineEditor::set_line(const std::string_view text)
    {
        const std::lock_guard lock{mutex_};

        if (!line_.empty()) {
            *stream_ << "\r\x1B[K";
        }

        line_ = text;
        cursor_ = line_.length();
        *stream_ << line_;
    }

    std::string InputLineEditor::line() const
    {
        const std::lock_guard lock{mutex_};
        return line_;
    }

    std::string InputLineEditor::submit_line()
    {
        const std::lock_guard lock{mutex_};

        *stream_ << '\n';

        auto input = std::move(line_);
        line_.clear();
        cursor_ = 0;

        return input;
    }

    void InputLineEditor::redraw()
    {
        *stream_ << '\r' << line_ << '\r';

        if (cursor_ > 0) {
            *stream_ << "\x1B[" << cursor_ << 'C';
        }
    }
}
