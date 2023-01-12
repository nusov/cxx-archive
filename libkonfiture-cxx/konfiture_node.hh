//
// konfiture_node.hh
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

#ifndef __KONFITURE_NODE_HH__
#define __KONFITURE_NODE_HH__

#include <algorithm>
#include <memory>
#include <string>
#include <list>

#include "konfiture_attribute.hh"

namespace konfiture {

class KFNode : public KFAttribute {
public:
    std::list<KFNode*>      nodes;
    std::list<KFAttribute*> attrs;
    bool                    has_value;

    KFNode(const std::string& k): KFAttribute(k, ""), has_value(false) { }
    KFNode(const std::string& k, const std::string& v): KFAttribute(k, v), has_value(true) {}
    
    bool has_child_nodes() {
        return (nodes.size() > 0);
    }

    ~KFNode() {
        std::for_each(attrs.begin(), attrs.end(), std::default_delete<KFAttribute>());
        std::for_each(nodes.begin(), nodes.end(), std::default_delete<KFNode>());
    }
};

}

#endif
