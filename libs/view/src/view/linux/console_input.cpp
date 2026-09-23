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
 * FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER
 * (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
 * BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY
 * LOSS) ARISING OUT OF THE USE OF OR INABILITY TO USE THE ENGINE AND/OR THE
 * SDK, EVEN IF VALVE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 * For commercial use, contact: sourceengine@valvesoftware.com
 */

#include "view/console_input.hpp"
#include "util/linux/system/error.hpp"
#include "util/logger.hpp"
#include "util/string.hpp"
#include "view/linux/terminal_settings.hpp"
#include "view/linux/tty_redirect.hpp"
#include "view/utf8.hpp"
#include <array>
#include <cctype>
#include <poll.h>
#include <string>
#include <string_view>
#include <unistd.h>

namespace {
    /// Reads a character from the standard input.
    int read_char_from_stdin()
    {
        int input_char = '\0';
        const auto bytes_read = ::read(STDIN_FILENO, &input_char, 1);

        if ((0 == bytes_read) || ('\0' == input_char)) {
            return 0;
        }

        if (-1 == bytes_read) {
            Util::log_error("Failed to read from terminal: {}", Util::get_last_error_string());
            return 0;
        }

        return input_char;
    }

    /// Returns the last sub-parameter of a CSI parameter string (e.g. "1;5" -> "5").
    std::string_view last_csi_subparameter(const std::string& parameters)
    {
        const auto position = parameters.rfind(';');

        return std::string_view{parameters}.substr((std::string::npos == position) ? 0 : (position + 1));
    }
}

namespace View {
    void ConsoleInput::handle_escape()
    {
        if (read_char_from_stdin() != '[') {
            return;
        }

        auto parameters = std::string{};
        auto final_byte = '\0';

        for (auto ch = read_char_from_stdin();; ch = read_char_from_stdin()) {
            if (0 == ch) {
                return;
            }

            if ((std::isdigit(static_cast<unsigned char>(ch)) != 0) || (';' == ch)) {
                parameters.push_back(static_cast<std::string::value_type>(ch));
                continue;
            }

            final_byte = static_cast<std::string::value_type>(ch);
            break;
        }

        if ('~' == final_byte) {
            if ("3;5" == parameters) {
                handle_ctrl_delete();
            }
            else if ("3" == parameters) {
                handle_delete();
            }

            return;
        }

        // Control is reported as the modifier code 5 (xterm-style "ESC[1;5D", urxvt-style "ESC[5D")
        const auto control = ("5" == last_csi_subparameter(parameters));

        switch (final_byte) {
            case 'A': handle_up_arrow(); break;
            case 'B': handle_down_arrow(); break;
            case 'C':
                if (control) {
                    handle_ctrl_right();
                }
                else {
                    handle_right_arrow();
                }

                break;
            case 'D':
                if (control) {
                    handle_ctrl_left();
                }
                else {
                    handle_left_arrow();
                }

                break;
            case 'F': handle_end(); break;
            case 'H': handle_home(); break;
            default: break;
        }
    }

    void ConsoleInput::read_input()
    {
        const ScopedTerminalSettings terminal_settings{};
        const ScopedTtyRedirect tty_redirect{};

        std::array<::pollfd, 1> pfd{{{STDIN_FILENO, POLLIN, 0}}};
        constexpr ::nfds_t pfd_size = 1;
        constexpr auto timeout_msec = 100;

        while (running_) {
            if (::poll(pfd.data(), pfd_size, timeout_msec) <= 0) {
                continue;
            }

            switch (const auto input_char = read_char_from_stdin(); input_char) {
                case '\0': continue;
                case '\n': handle_newline(); break;
                case '\x1B': handle_escape(); break;
                case '\x7F': handle_backspace(); break;
                case '\x08': handle_ctrl_backspace(); break;
                case '\x17': handle_ctrl_w(); break;
                case '\t': handle_tab(); break;
                default: handle_utf8_input(input_char); break;
            }
        }
    }

    void ConsoleInput::handle_utf8_input(const int first_byte)
    {
        if (first_byte < 0x80) {
            const auto character = static_cast<std::string::value_type>(first_byte);

            if (Util::str::is_printable_char(character)) {
                editor_->insert_char(character);
            }

            return;
        }

        const auto sequence_length = utf8::sequence_length(static_cast<unsigned char>(first_byte));

        if (0 == sequence_length) {
            return;
        }

        std::string sequence;
        sequence.reserve(sequence_length);
        sequence.push_back(static_cast<std::string::value_type>(first_byte));

        for (auto i = sequence_length - 1; i > 0; --i) {
            const auto continuation = read_char_from_stdin();

            if ((continuation < 0x80) || (continuation > 0xBF)) {
                return;
            }

            sequence.push_back(static_cast<std::string::value_type>(continuation));
        }

        editor_->insert_text(sequence);
    }
}
