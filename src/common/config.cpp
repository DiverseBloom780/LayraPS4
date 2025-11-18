         return std::lexicographical_compare(
                a.path_str.begin(), a.path_str.end(), b.path_str.begin(), b.path_str.end(),
                [](char a_char, char b_char) {
                    return std::tolower(a_char) < std::tolower(b_char);
                });
        });

        for (const auto& entry : sorted_dirs) {
            install_dirs.push_back(entry.path_str);
            install_dirs_enabled.push_back(entry.enabled);
        }

        // Non game-specific entries
        data["General"]["enableDiscordRPC"] = Config::enableDiscordRPC;
        data["General"]["sysModulesPath"] = string{fmt::UTF(Config::sys_modules_path.u8string()).data};
