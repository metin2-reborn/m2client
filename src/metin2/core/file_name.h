//	CFilename Class
//	(c) 2003 IDV, Inc.
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//		IDV, Inc.
//		1233 Washington St. Suite 610
//		Columbia, SC 29201
//		Voice: (803) 799-1699
//		Fax:   (803) 931-0320
//		Web:   http://www.idvinc.com

#pragma once

#include <cctype>
#include <cstring>
#include <string>

namespace metin2::inline core {

    class file_name_helper {
      public:
        static void change_dos_path(std::string &str) {
            size_t length = str.length();

            for (size_t i = 0; i < length; ++i) {
                if (str.at(i) == '/')
                    str.at(i) = '\\';
            }
        }

        static void string_path(std::string &str) {
            size_t length = str.length();

            for (size_t i = 0; i < length; ++i) {
                if (str.at(i) == '\\')
                    str.at(i) = '/';
                else
                    str.at(i) = (char)tolower(str.at(i));
            }
        }

        static auto get_name(std::string &str) -> std::string;     // if filename is "/idv/code/file.cpp", it returns "file"
        static auto extension(std::string &str) -> std::string;    // if filename is "/idv/code/file.cpp", it returns "cpp"
        static auto path(std::string &str) -> std::string;         // if filename is "/idv/code/file.cpp", it returns "/idv/code"
        static auto no_extension(std::string &str) -> std::string; // if filename is "/idv/code/file.cpp", it returns "/idv/code/file"
        static auto no_path(std::string &str) -> std::string;      // if filename is "/idv/code/file.cpp", it returns "file.cpp"
    };

    inline auto file_name_helper::get_name(std::string &str) -> std::string {
        std::string name;

        size_t length = str.length();

        if (length > 0) {
            size_t extension_start_pos = length - 1;

            for (size_t i = length - 1; i > 0; i--) {
                if (str[i] == '.') {
                    extension_start_pos = i;
                }

                if (str[i] == '/') {
                    name = std::string(str.c_str() + i + 1);
                    name.resize(extension_start_pos - i - 1);
                    break;
                }
            }
        }

        return name;
    }

    inline auto file_name_helper::extension(std::string &str) -> std::string {
        std::string extension;

        size_t length = str.length();

        if (length > 0) {
            for (size_t i = length - 1; i > 0 && str[i] != '/'; i--)
                if (str[i] == '.') {
                    extension = std::string(str.c_str() + i + 1);
                    break;
                }
        }

        return extension;
    }

    inline auto file_name_helper::path(std::string &str) -> std::string {
        // everything up to and including the last separator (a separator at index 0 does not count)
        size_t length = str.length();
        for (size_t i = length; i-- > 1;) {
            if (str[i] == '/' || str[i] == '\\')
                return str.substr(0, i + 1);
        }
        return std::string();
    }

    inline auto file_name_helper::no_extension(std::string &str) -> std::string {
        std::size_t npos = str.find_last_of('.');

        if (std::string::npos != npos)
            return std::string(str, 0, npos);

        return str;
    }

    inline auto file_name_helper::no_path(std::string &str) -> std::string {
        // everything after the last separator (a separator at index 0 does not count)
        size_t length = str.length();
        for (size_t i = length; i-- > 1;) {
            if (str[i] == '/' || str[i] == '\\')
                return str.substr(i + 1);
        }
        return str;
    }
}
