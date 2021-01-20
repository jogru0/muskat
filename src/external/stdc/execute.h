#pragma once
#ifndef _MSC_VER //TODO: support MSVC --mka/20/03

#include <fcntl.h> //fcntl
#include <sys/wait.h> //waitpid
#include <unistd.h> //execXYZ

#include "arguments.h"

namespace stdc {

	class executable {
	private:
		bool process_given_as_path;
		stdc::arguments argv_container;
	
	public:
		template<typename Arg_Container>
		executable(std::string_view process, const Arg_Container &args, bool _process_given_as_path = true) :
			process_given_as_path{_process_given_as_path},
			argv_container{} {
			argv_container.push_back(process);
			argv_container.push_back_range(args.begin(), args.end());
		}

		//[[noreturn]] if sucessful, otherwise returns -1.
		auto take_over() {
			//TODO: Really okay to supply c_style_process twice? --jgr/20/03
			auto* argv = argv_container.c_args();
			auto called_path = argv[0]; //NOLINT

			if (process_given_as_path) {
				execv(called_path, argv);
			} else {
				execvp(called_path, argv);
			}
			//If we get here, the transformation to the other process was not successful.
			return -1;
		}

		auto operator()() -> std::pair<bool, int> {
			int pipefds[2]; //NOLINT
			
			//TODO: Können die beiden Sachen mit pipe2 kombiniert werden?
			if (pipe2(pipefds, O_CLOEXEC) != 0) {
				return {false, -4};
			};

			fcntl(pipefds[1], F_SETFD, fcntl(pipefds[1], F_GETFD) | FD_CLOEXEC); //NOLINT
			
			auto pid = fork();
			if (pid == -1) {
				//Fork failed.
				return {false, -2};
			}

			if (pid == 0) {
				//We are child.
				close(pipefds[0]);
				take_over();
				//If even this write fails, we can't do anything about it.
				[[maybe_unused]] auto write_result = write(pipefds[1], &errno, sizeof(int));
				_exit(127); //_exit should be used for childs. 127 means exec failed.
			}

			//We are parent.
			close(pipefds[1]);
			int err{};
			ssize_t count{};
			while ((count = read(pipefds[0], &err, sizeof(errno))) == -1) {
				if (errno != EAGAIN && errno != EINTR) {
					break;
				}
			}
			if (count != 0) {
				//Child failed to become other process.
				return {false, -1};
			}

			close(pipefds[0]);
			int status{};
			waitpid(pid, &status, 0);
			return {true, status};
		}
	

		friend auto operator<<(std::ostream &, const executable &) -> std::ostream &;
	};

	inline auto operator<<(std::ostream &output, const executable &exe) -> std::ostream & {
		const auto &argv = exe.argv_container;
		output << argv[0];
		for (auto it = std::next(argv.begin()); it != argv.end(); ++it) {
			output << ' ' << *it;
		}
		return output;
	}

	[[nodiscard]] inline auto to_string(const stdc::executable &exe) {
	auto stream = std::ostringstream{};
	stream << exe;
	return stream.str();
}
} //namespace stdc
#endif //_MSC_VER