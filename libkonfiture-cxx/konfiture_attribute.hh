//
// konfiture_driver.hh
//
// Author:
//   Alexander Nusov 
//
// Copyright (C) 2014 Alexander Nusov
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef __KONFITURE_ATTRIBUTE_HH__
#define __KONFITURE_ATTRIBUTE_HH__

#include <iostream>
#include <ostream>
#include <string>
#include <regex>

namespace konfiture {

class KFAttribute {
public:
    std::string key;
    std::string value;

    KFAttribute(const std::string& k, const std::string& v): key(k), value(v) {}

    template<typename T>
    bool is() {
        if (std::is_same<T, std::string>::value) {
            return true;
        }
        if (std::is_arithmetic<T>::value) {
            if (std::is_same<T, bool>::value) {
                return std::regex_match(value, std::regex("^(true|false)$", std::regex_constants::icase));
            }
            else {
                return std::regex_match(value, std::regex("^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$"));
            }
        }

        return false;
    }

    template<typename T>
    T as() {
        if (std::is_floating_point<T>::value) {
            if (std::is_same<T, float>::value) {
                return T(std::stof(value));
            }
            else if (std::is_same<T, double>::value) {
                return T(std::stod(value));
            }
            else if (std::is_same<T, long double>::value) {
                return T(std::stold(value));
            }
        }
        if (std::is_integral<T>::value) {
            if (std::is_unsigned<T>::value) {
                if (std::is_same<T, unsigned long long>::value) {
                    return T(std::stoull(value));
                }
                else {
                    return T(std::stoul(value));
                }
            }
            else {
                if (std::is_same<T, long long>::value) {
                    return T(std::stoll(value));
                }
                else if (std::is_same<T, long>::value) {
                    return T(std::stol(value));
                }
                else {
                    return T(std::stoi(value));
                }
            }
        }
    }
};

template<>
inline bool KFAttribute::as<bool>() {
    return std::regex_match(value, std::regex("^true$", std::regex_constants::icase));
}

template<>
inline std::string KFAttribute::as<std::string>() {
    return value;
}

template<>
inline std::string& KFAttribute::as<std::string&>() {
    return value;
}
}

#endif
