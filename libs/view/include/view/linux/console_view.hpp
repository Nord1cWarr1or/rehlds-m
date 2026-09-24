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

#pragma once

#include "util/log_output.hpp"
#include "view/base_view.hpp"
#include "view/input_line_editor.hpp"
#include <fmt/core.h>
#include <memory>

namespace View {
    class ConsoleView final : public BaseView, public Util::LogOutput {
      public:
        /**
         * @brief Constructs a new ConsoleView object.
         *
         * @param editor A shared pointer to the input line editor whose rendered
         * line must be preserved across console output.
         */
        explicit ConsoleView(const std::shared_ptr<InputLineEditor>& editor);

        /**
         * @brief Prints the given text.
         *
         * The console output of the engine is line-based but may arrive in
         * fragments (e.g. the echo command prints each argument separately,
         * expecting the console to concatenate them). Complete lines are written
         * with the input line preserved; a trailing fragment without a newline
         * is buffered until the rest of the line arrives, or until it exceeds
         * a reasonable size.
         *
         * @param text The text to be printed.
         *
         * @return 0 if the operation is successful.
         */
        int print(std::string_view text) override; // NOLINT(modernize-use-nodiscard)

        /**
         * @brief Displays the server status information.
         *
         * @param fps The current FPS of the server.
         * @param num_players The number of active players on the server.
         * @param max_players The maximum number of players allowed on the server.
         * @param map The name of the current map on the server.
         */
        void display_server_status(float fps, int num_players, int max_players, const std::string& map) override;

        /**
         * @brief Write a log message to the console.
         *
         * @param message The formatted log message to be written.
         */
        void write_log(std::string_view message) override;

      private:
        /// The input line editor whose rendered line is preserved across console output.
        std::shared_ptr<InputLineEditor> editor_;

        /// The output fragment waiting for the rest of its line.
        std::string pending_{};
    };

    inline ConsoleView::ConsoleView(const std::shared_ptr<InputLineEditor>& editor) : editor_(editor) {}

    inline void ConsoleView::display_server_status(
      const float /* fps */, const int /* num_players */, const int /* max_players */, const std::string& /* map */)
    {
        // No implementation on Linux
    }

    inline int ConsoleView::print(const std::string_view text)
    {
        if (editor_ != nullptr) {
            pending_.append(text.data(), text.size());

            std::size_t start = 0;

            for (auto newline = pending_.find('\n', start); newline != std::string::npos;
                 newline = pending_.find('\n', start)) {
                editor_->write_through(std::string_view{pending_}.substr(start, (newline - start) + 1));
                start = newline + 1;
            }

            pending_.erase(0, start);

            // Flush abnormally long unterminated output instead of buffering it forever
            constexpr auto max_pending_size = std::size_t{8192};

            if (pending_.size() > max_pending_size) {
                editor_->write_through(pending_ + '\n');
                pending_.clear();
            }

            return std::fflush(stdout);
        }

        fmt::print(text);

        return std::fflush(stdout);
    }

    inline void ConsoleView::write_log(const std::string_view message)
    {
        print(message);
    }
}
