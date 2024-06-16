// COMPATIBILITY: Linux

#include <iostream>
#include <algorithm>
#include <ostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <experimental/filesystem>
#include <cstdlib>
#include <iomanip>

#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "main.h"

std::string exec(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(reinterpret_cast<FILE *>(popen(cmd, "r")), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

std::string generate_readable_permissions(std::string permissions)
{
    std::string readable_permissions;
    if (permissions.size() == 3)
    {
        for (char c : permissions)
        {
            if (c == '7')
                readable_permissions += "rwx";
            else if (c == '6')
                readable_permissions += "rw-";
            else if (c == '5')
                readable_permissions += "r-x";
            else if (c == '4')
                readable_permissions += "r--";
            else if (c == '3')
                readable_permissions += "-wx";
            else if (c == '2')
                readable_permissions += "-w-";
            else if (c == '1')
                readable_permissions += "--x";
            else if (c == '0')
                readable_permissions += "---";
        }
    }

    return readable_permissions;
}

bool is_dir(std::string PATH) {
    struct stat md;
    if (stat(PATH.c_str(), &md) == 0)
    {
        return S_ISDIR(md.st_mode);
    }
    return false;
}

bool is_file(std::string PATH) {
    struct stat md;
    if (stat(PATH.c_str(), &md) == 0)
    {
        return S_ISREG(md.st_mode);
    }
    return false;
}

void change_directory(const std::vector<std::string> &arguments, std::string &current_path)
{
    if (arguments.empty())
    {
        std::cout << "cd: path required" << std::endl;
    }
    else
    {
        std::string PATH = current_path + "/" + arguments[0];

        struct stat md;

        if (stat(PATH.c_str(), &md) == 0)
        {
            // Make sure it is a directory
            if (is_dir(PATH))
            {
                char absolute_path[32767];
                if (realpath(PATH.c_str(), absolute_path) != nullptr)
                {
                    current_path = absolute_path;
                }
                else
                {
                    std::cerr << "cd: failed to get absolute path" << std::endl;
                }
            }
            else
            {
                std::cout << "cd: path is a file" << std::endl;
            }
        }
        else
        {
            std::cout << "invalid path" << std::endl;
        }
    }
}

struct Color
{
    std::string black = "\033[30m";
    std::string red = "\033[31m";
    std::string green = "\033[32m";
    std::string yellow = "\033[33m";
    std::string blue = "\033[34m";
    std::string magenta = "\033[35m";
    std::string cyan = "\033[36m";
    std::string white = "\033[37m";
    std::string reset = "\033[0m";
};

std::vector<std::string> split_string_with_whitespaces(std::string string)
{
    std::vector<std::string> result;
    std::istringstream iss(string);
    std::string token;
    while (iss >> token)
    {
        result.push_back(token);
    }
    return result;
}

int main(int argc, char *argv[])
{
    Color color_codes = Color{};

    std::string username = exec("whoami");

    // Remove trailing \n from whoami output.
    username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());

    std::string current_path = ("/home/" + username);

    // If command line argument "--usedir" is present, use that director as starting path instead.

    // Check if the command line argument "--usedir" is present
    if (argc > 1 && std::string(argv[1]) == "--usedir")
    {
        if (argc > 2)
        {
            current_path = argv[2];
        }
        else
        {
            std::cerr << "ERR: No directory specified after --usedir" << std::endl;
            return 1;
        }
    }

    while (true)
    {
        std::string prompt = "CShell$ [" + current_path + "] ";
        char *buf = readline(prompt.c_str());

        // If the buffer is null, then an EOF has been received (ctrl-d)
        if (!buf)
        {
            std::cout << std::endl;
            break;
        }

        // If the line has any text in it, save it on the history.
        if (buf[0] != '\0')
        {
            add_history(buf);
        }

        std::string shell_command(buf);
        free(buf);

        // Get the command name (first element when seperated with a whitespace)
        std::vector<std::string> tokens = split_string_with_whitespaces(shell_command);
        std::string command_name = tokens[0];

        // Check if the command is valid
        if (std::find(valid_commands.begin(), valid_commands.end(), tokens[0]) != valid_commands.end())
        {
            tokens.erase(tokens.begin());
            std::vector<std::string> arguments = tokens;

            if (command_name == "ls")
            {
                // ls command
                // List all files and directories in the current working directory.
                // Arguments:
                //         -s:      Include the file size in BYTES.
                //         -sr:     Readable file sizes (Bytes, KBytes, MBytes, GBytes, TBytes, PBytes)

                std::experimental::filesystem::directory_iterator dir_iter(current_path);
                for (const auto &entry : dir_iter)
                {
                    if (std::experimental::filesystem::is_directory(entry))
                    {
                        std::string file_name = entry.path().filename().string();
                        std::string permissions = std::to_string(static_cast<int>(std::experimental::filesystem::status(entry).permissions()));

                        // Get human readable permissions
                        std::string readable_permissions = generate_readable_permissions(permissions);

                        // Print out the file size if the -s argument is present

                        std::cout << color_codes.blue << "dir " << color_codes.yellow << readable_permissions << " " << color_codes.reset << file_name << std::endl;
                    }
                    else if (std::experimental::filesystem::is_regular_file(entry))
                    {

                        bool include_size = false;
                        if (std::find(arguments.begin(), arguments.end(), "-s") != arguments.end())
                        {
                            include_size = true;
                        }

                        bool include_readable_size = false;
                        if (std::find(arguments.begin(), arguments.end(), "-sr") != arguments.end())
                        {
                            include_readable_size = true;
                        }
                        // Get the file size if the -s argument is present
                        if (include_size)
                        {

                            int64_t file_size = std::experimental::filesystem::file_size(entry);

                            std::string file_name = entry.path().filename().string();
                            std::string permissions = std::to_string(static_cast<int>(std::experimental::filesystem::status(entry).permissions()));

                            // Get human readable permissions
                            std::string readable_permissions = generate_readable_permissions(permissions);

                            std::cout << color_codes.blue << "file " << color_codes.yellow << readable_permissions << color_codes.green << " " << file_size << "B " << color_codes.reset << file_name << std::endl;
                        }
                        else
                        {
                            std::string file_name = entry.path().filename().string();
                            std::string permissions = std::to_string(static_cast<int>(std::experimental::filesystem::status(entry).permissions()));

                            // Get human readable permissions
                            std::string readable_permissions = generate_readable_permissions(permissions);

                            std::cout << color_codes.blue << "file " << color_codes.yellow << readable_permissions << " " << color_codes.reset << file_name << std::endl;
                        }

                        if (include_readable_size)
                        {

                            int64_t file_size = std::experimental::filesystem::file_size(entry);

                            std::string file_name = entry.path().filename().string();
                            std::string permissions = std::to_string(static_cast<int>(std::experimental::filesystem::status(entry).permissions()));

                            // Select the data type for file size division
                            std::string data_type;
                            double readable_file_size = file_size;
                            if (readable_file_size >= 1024 * 1024 * 1024)
                            {
                                readable_file_size /= 1024 * 1024 * 1024;
                                data_type = "GB";
                            }
                            else if (readable_file_size >= 1024 * 1024)
                            {
                                readable_file_size /= 1024 * 1024;
                                data_type = "MB";
                            }
                            else if (readable_file_size >= 1024)
                            {
                                readable_file_size /= 1024;
                                data_type = "KB";
                            }
                            else
                            {
                                data_type = "B";
                            }

                            // Get human readable permissions
                            std::string readable_permissions = generate_readable_permissions(permissions);

                            std::cout << color_codes.blue << "file " << color_codes.yellow << readable_permissions << color_codes.green << " " << std::fixed << std::setprecision(1) << readable_file_size << data_type << color_codes.reset << " " << file_name << std::endl;
                        }
                    }
                }
            }
            else if (command_name == "clear")
            {
                // Works.
                system("clear");
            }
            else if (command_name == "wd")
            {
                std::cout << current_path << std::endl;
            }
            else if (command_name == "cd")
            {
                if (arguments.empty())
                {
                    std::cout << "cd: path required" << std::endl;
                }
                else
                {
                    change_directory(arguments, current_path);
                }
            } else if (command_name == "touch") {
                if (arguments.empty()) {
                    std::cout << "touch: file name required" << std::endl;
                } else {
                    std::string FILE_NAME = arguments[0];

                    // Check if the file already exists.
                    if (is_file(FILE_NAME)) {
                        std::cerr << "touch: file already exists " << FILE_NAME << std::endl; 
                    }


                    // create the file
                    std::ofstream CREATED_FILE(FILE_NAME);
                    if (!CREATED_FILE)
                    {
                        std::cerr << "touch: failed to create file " << FILE_NAME << std::endl;
                    }
                    else
                    {
                        CREATED_FILE.close();
                    }
                }
            }
        }
        else
        {
            std::cerr << "unknown command: " << shell_command << std::endl;
        }
    }
}