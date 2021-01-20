#pragma once

#include <filesystem>

#include "range.h"

namespace stdc {
	namespace fs {
		using namespace std::filesystem;

		//order of iteration is unspecified, so we need it platform independent
		[[nodiscard]] inline auto get_ordered_folder_entries(const fs::path &folder_path)
			-> std::vector<fs::directory_entry>
		{
			auto directory_iterator = fs::directory_iterator(folder_path);
			auto ordered_entries = std::vector<fs::directory_entry>(
				fs::begin(directory_iterator), fs::end(directory_iterator)
			);
			std::sort(RANGE(ordered_entries));
			return ordered_entries;
		}

	} //namespace fs
} //namespace stdc
