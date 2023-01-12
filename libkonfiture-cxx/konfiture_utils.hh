#ifndef __KONFITURE_UTILS_HH__
#define __KONFITURE_UTILS_HH__

#include <string>

namespace konfiture {
    inline std::string kf_unescape(const std::string& s) {
        std::string res(s);
        std::string::size_type offset = 0;

        while (offset < res.length()) {
            offset = res.find("\\", offset);
            if (offset != std::string::npos) {
                res.erase(offset, 1);
                switch (res[offset]) {
                case 't':
                    res[offset] = '\t';
                    break;
                case 'r':
                    res[offset] = '\r';
                    break;
                case 'n':
                    res[offset] = '\n';
                }
                offset += 1;
            }
        }

        return res;
    }
}

#endif

