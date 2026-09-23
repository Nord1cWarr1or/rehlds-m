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

#include "view/console_input.hpp"
#include "presenter/input_presenter.hpp"
#include "util/console.hpp"
#include "util/string.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <utility>
#include <vector>

namespace {
    /// Returns the length of the longest command in a vector of commands.
    std::string::size_type get_longest_command_length(const std::vector<std::string>& commands)
    {
        const auto& longest = std::max_element(commands.cbegin(), commands.cend(), [](const auto& a, const auto& b) {
            return a.length() < b.length();
        });

        return longest == commands.cend() ? 0 : longest->length();
    }

    /// Finds the common prefix among a vector of command strings.
    std::string find_common_command_prefix(const std::vector<std::string>& commands)
    {
        if (commands.empty()) {
            return "";
        }

        // Get the iterators for the first and last strings in the vector
        const auto first1 = commands.front().cbegin();
        const auto last1 = commands.front().cend();
        const auto first2 = commands.back().cbegin();
        const auto last2 = commands.back().cend();

        // Use std::mismatch to find the first mismatch between the first and last strings
        const auto& [first_mismatch, last_mismatch] = std::mismatch(first1, last1, first2, last2);

        // Return the common prefix as a substring of the first string
        return std::string{first1, first_mismatch};
    }

    /// Prints a list of matching commands to the console.
    void print_command_matches(const std::vector<std::string>& commands)
    {
        constexpr std::size_t default_console_width = 80;
        constexpr std::size_t min_console_width = 10;
        auto console_width = Util::get_console_width();

        if (console_width < min_console_width) {
            console_width = default_console_width;
        }

        const auto longest_command_length = get_longest_command_length(commands);
        const auto total_columns = (console_width - 1) / (longest_command_length + 1);
        std::size_t current_column = 0;

        for (const auto& command : commands) {
            ++current_column;

            if (current_column > total_columns) {
                current_column = 1;
                std::cout << '\n';
            }

            std::cout << Util::str::format("{:<{}}  ", command, longest_command_length);
        }
    }
}

namespace View {
    ConsoleInput::ConsoleInput(InputPresenterPtr input_presenter, std::shared_ptr<InputLineEditor> editor) :
      input_presenter_(std::move(input_presenter)),
      editor_(std::move(editor))
    {
        assert(input_presenter_ != nullptr);
        assert(editor_ != nullptr);
        start();
    }

    ConsoleInput::~ConsoleInput()
    {
        stop();
    }

    void ConsoleInput::start()
    {
        if (running_) {
            return;
        }

        running_ = true;
        input_thread_ = std::thread{&ConsoleInput::read_input, this};

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
    }

    void ConsoleInput::stop()
    {
        if (running_) {
            running_ = false;
            input_thread_.join();
        }
    }

    void ConsoleInput::handle_up_arrow()
    {
        if (const auto& previous_input = input_presenter_->history_previous(); previous_input) {
            if (!saved_input_) {
                saved_input_ = editor_->line();
            }

            editor_->set_line(*previous_input);
        }
    }

    void ConsoleInput::handle_down_arrow()
    {
        if (const auto& next_input = input_presenter_->history_next(); next_input || saved_input_) {
            if (next_input) {
                editor_->set_line(*next_input);
            }
            else {
                editor_->set_line(*saved_input_);
                saved_input_ = std::nullopt;
            }
        }
    }

    void ConsoleInput::handle_left_arrow()
    {
        editor_->left();
    }

    void ConsoleInput::handle_right_arrow()
    {
        editor_->right();
    }

    void ConsoleInput::handle_backspace()
    {
        editor_->backspace();
    }

    void ConsoleInput::handle_delete()
    {
        editor_->delete_char();
    }

    void ConsoleInput::handle_home()
    {
        editor_->home();
    }

    void ConsoleInput::handle_end()
    {
        editor_->end();
    }

    void ConsoleInput::handle_tab()
    {
        const auto& commands = input_presenter_->find_command_matches(editor_->line());

        if (commands.empty()) {
            return;
        }

        if (1U == commands.size()) {
            const auto completion = commands.front().substr(editor_->line().length()) + " ";
            editor_->insert_text(completion);
        }
        else {
            editor_->with_suspended([&commands] {
                std::cout << '\n';
                print_command_matches(commands);
                std::cout << '\n';
            });

            editor_->set_line(find_common_command_prefix(commands));
        }
    }

    void ConsoleInput::handle_newline()
    {
        input_presenter_->enqueue_input(editor_->submit_line());
    }

    void ConsoleInput::handle_char(const std::string::value_type ch)
    {
        if (!Util::str::is_printable_char(ch)) {
            return;
        }

        editor_->insert_char(ch);
    }
}
