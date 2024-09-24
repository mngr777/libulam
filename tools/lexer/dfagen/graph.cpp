#include "graph.hpp"
#include "libulam/token.hpp"
#include "src/lexer/dfa.hpp"
#include <cassert>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <stdexcept>

namespace {

std::string tok_type_to_str(tok::Type type) {
    return std::string("tok::") + tok::type_to_str(type);
}

std::string cat_to_str(dfa::ClassFlags cls) {
#define CASE(cls)                                                              \
    case cls:                                                                  \
        return #cls
    switch (cls) {
        CASE(cls::None);
        CASE(cls::Any);
        CASE(cls::Alpha);
        CASE(cls::Digit);
        CASE(cls::Other);
        CASE(cls::Space);
        CASE(cls::Alnum);
        CASE(cls::Word);
        CASE(cls::WordNonAlnum); // really?
    default:
        throw std::invalid_argument(
            std::string("Invalid character class value ") +
            std::to_string(cls));
    }
}

} // namespace

std::string Graph::Edge::to_string() const {
    return std::string("{") + std::to_string((int)chr) + ", " +
           cat_to_str(cls) + ", " + std::to_string(next.lock()->index) + "}";
}

void Graph::Node::check_valid() {
    if (is_final) {
        // Final node, must not have outgoing edges
        if (edges.size() != 0) {
            throw std::logic_error(
                std::string("Final node `") + name + "' has outgoing edge");
        }
    } else {
        // Non-final node, must have catchall
        if (!catchall()) {
            throw std::logic_error(
                std::string("Non-final node `") + name +
                "' does not have a catch-all edge");
        }
    }
}

void Graph::Node::add_edge(NodePtr next, char chr, dfa::ClassFlags cls) {
    auto edge = std::make_shared<Edge>();
    edge->chr = chr;
    edge->cls = cls;
    edge->next = next;
    edges.push_back(edge);
    std::sort(edges.begin(), edges.end(), [](auto e1, auto e2) {
        if (e1->cls != e2->cls)
            return (e1->cls < e2->cls); // cls::None < ... < cls::Any
        if (e1->chr == '\0' || e2->chr == '\0')
            return e2->chr == '\0';
        if (e1->chr == '\n' || e2->chr == '\n')
            return e2->chr == '\n';
        return e1->chr < e2->chr;
    });
}

void Graph::Node::replace_catchall(NodePtr next) {
    auto edge = catchall();
    if (edge)
        remove_edge(edge);
    add_catchall(next);
}

void Graph::Node::remove_edge(Graph::EdgePtr edge) {
    auto it = std::find(edges.begin(), edges.end(), edge);
    assert(it != edges.end());
    edges.erase(it);
}

Graph::EdgePtr Graph::Node::edge_for(char ch) {
    for (auto edge : edges) {
        if (edge->chr == ch)
            return edge;
    }
    return nullptr;
}

Graph::EdgePtr Graph::Node::edge_for_class(dfa::ClassFlags cls) {
    for (auto edge : edges) {
        if (edge->cls == cls)
            return edge;
    }
    return nullptr;
}

std::string Graph::Node::to_string() const {
    return std::string("{") + (is_final ? "true" : "false") + ", " +
           tok_type_to_str(type) + ", " + std::to_string(first_edge_index) +
           "}";
}

Graph::Graph() {
    // End of name
    _shared.name_end = make_node("[name]" /* fin */, true, tok::Name);
    // Reading a name
    assert(_shared.name_end);
    _shared.name = make_node("[name]", false, tok::Name);
    // (name) -- word --> (name)
    _shared.name->add_class_edge(_shared.name, cls::Word);
    // (name) -- non-word --> (end-of-name)
    _shared.name->add_catchall(_shared.name_end);

    // Start nodes for names (or keywords)
    assert(_shared.name);
    for (char ch = 'A'; ch <= 'z'; ++ch) {
        if (!std::isalpha(ch))
            continue;
        auto node = make_node({ch}, false, tok::Name);
        // (letter) -- word --> (name)
        node->add_class_edge(_shared.name, cls::Word);
        // (letter) -- non-word --> (end-of-name)
        node->add_catchall(_shared.name_end);
        _start[ch] = node;
    }

    // NOTE:
    // * accepting anything alphanumeic for numbers to be parsed later
    // * dot is not allowed, no FP numbers

    // End of number
    _shared.number_end = make_node("[number]" /* fin */, true, tok::Number);
    // Reading a number
    assert(_shared.number_end);
    _shared.number = make_node("[number]", false, tok::Number);
    // (number) -- alnum --> (number)
    _shared.number->add_class_edge(_shared.number, cls::Alnum);
    // (number) -- non-alnum --> (end of number)
    _shared.number->add_catchall(_shared.number_end);

    // Start nodes for numbers
    assert(_shared.number);
    for (char ch = '0'; ch <= '9'; ++ch) {
        auto node = make_node({ch}, false, tok::Number);
        _start[ch] = node;
        // (digit) -- alnum --> (number)
        node->add_class_edge(_shared.number, cls::Alnum);
        // (digit) -- non-alnum --> (end-of-number)
        node->add_catchall(_shared.number_end);
    }

    // End of string
    _shared.string_end = make_node("[string]" /* fin */, true, tok::String);
    // Closing `"'
    _shared.string_close = make_node("[string\"]", false, tok::String);
    _shared.string_close->add_catchall(_shared.string_end);
    // String
    _shared.string = make_node("[string]", false, tok::String);
    _shared.string_escape = make_node("[string\\]", false, tok::String);
    // (string) -- " --> (string end)
    _shared.string->add_edge(_shared.string_close, '"');
    // (string) -- \n --> (string end), unterminated string
    _shared.string->add_edge(_shared.string_end, '\n');
    // (string) -- \0 --> (string end), unterminated string
    _shared.string->add_edge(_shared.string_end, '\0');
    // (string) -- \ --> (string escape)
    _shared.string->add_edge(_shared.string_escape, '\\');
    // (string) -- anything else --> (string)
    _shared.string->add_catchall(_shared.string);
    // (string escape) -- \n --> (string end), unterminated string
    _shared.string_escape->add_edge(_shared.string_end, '\n');
    // (string escape) -- \0 --> (string end), unterminated string
    _shared.string_escape->add_edge(_shared.string_end, '\0');
    // (string escape) -- anything else --> (string)
    _shared.string_escape->add_catchall(_shared.string);
    _start['"'] = _shared.string;

    // End of invalid keyword (starts with @, but doesn't match a keyword)
    _shared.invalid_kw_end = make_node("[invalid keyword]" /* fin */, true);
    // Reading invalid keyword
    _shared.invalid_kw = make_node("[invalid keyword]", false);
    // (invalid kw) -- word --> (invalid kw)
    _shared.invalid_kw->add_class_edge(_shared.invalid_kw, cls::Word);
    // (invalid kw) -- non-word -> (end of invalid kw)
    _shared.invalid_kw->add_class_edge(_shared.invalid_kw_end, cls::Any);

    // Invalid string
    _shared.invalid = make_node("[invalid]" /* fin */, true);
}

void Graph::add(const std::string& str, tok::Type type) {
    assert(!str.empty());

    // Get last matching node
    auto res = find(str);
    auto cur = res.first;
    auto pos = res.second;
    if (!cur) {
        // Make start node
        char first_char = str[0]; // TODO: check 7-bit
        cur = make_node(str.substr(0, pos + 1));
        _start[first_char] = cur;
        init_node(cur, str, pos, type);
        ++pos;
    } else if (pos == str.size()) {
        // Last node is full match
        init_node(cur, str, pos - 1, type);
    }

    // Add nodes for the rest of chars
    for (; pos < str.size(); ++pos) {
        auto next = make_node(str.substr(0, pos + 1));
        cur->add_edge(next, str[pos]);
        init_node(next, str, pos, type);
        cur = next;
    }
}

void Graph::add_comment(const std::string& open, const std::string& close) {
    // TODO
}

std::pair<Graph::NodePtr, std::size_t> Graph::find(const std::string& str) {
    assert(!str.empty());

    // Get start node
    std::size_t pos = 0;
    char first_char = str[pos]; // TODO: check 7-bit
    NodePtr cur = _start[first_char];
    if (!_start[first_char])
        return {nullptr, 0};
    ++pos;

    // Follow exact edges
    for (; pos < str.size(); ++pos) {
        char ch = str[pos];
        auto edge = cur->edge_for(ch);
        if (!edge)
            return {cur, pos};
        cur = edge->next.lock();
    }
    assert(cur && pos == str.size());
    return {cur, pos};
}

void Graph::gen(std::ostream& os) {
    NodeList nodes{std::make_shared<Node>("placeholder for initial state")};
    EdgeList edges;
    // Place nodes and edges
    for_each([&nodes, &edges](NodePtr node) {
        if (node->index != 0)
            return false; // stop recursion
        // Place node
        node->index = nodes.size();
        assert(node->index != 0);
        nodes.push_back(node);
        // Validate
        node->check_valid();
        // Place edges
        node->first_edge_index = edges.size();
        for (auto edge : node->edges) {
            edges.push_back(edge);
        }
        return true;
    });

    // Output start table
    os << "unsigned StartStates[" << _start.size() << "] = {";
    char ch = 0;
    while (true) {
        bool is_last = (ch == 127);
        if (ch % 16 == 0)
            os << "\n    ";
        os << std::setw(4) << std::right << (_start[ch] ? _start[ch]->index : 0)
           << (!is_last ? "," : "\n");
        if (!is_last) {
            ++ch;
        } else
            break;
    }
    os << "};\n\n";

    // Output nodes
    os << "Node NodeList[" << nodes.size() << "] = {\n";
    // /* n:<idx> */ {<result>, <token>, <first_edge>}, /* <node-name> */
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        std::string node_str = node->to_string();
        if (i + 1 < nodes.size())
            node_str += ",";
        os << "    /* " << std::setw(5) << std::right
           << (std::string("n:") + std::to_string(i)) << " */ " << std::setw(28)
           << std::left << node_str;
        os << " /* " << node->name;
        if (node->is_final)
            os << " (fin)";
        os << " */\n";
    }
    os << "};\n\n";

    // Output edges
    // /* e:<idx> */ {<char-dec>, <char-category-flags>, <next-node-idx>} /* ->
    // <node-name>*/
    os << "Edge EdgeList[" << edges.size() << "] = {\n";
    for (std::size_t i = 0; i < edges.size(); ++i) {
        const auto& edge = edges[i];
        const auto next = edge->next.lock();
        std::string edge_str{edge->to_string()};
        if (i + 1 < edges.size())
            edge_str += ",";
        os << "    /* " << std::setw(5) << std::right
           << (std::string("e:") + std::to_string(i)) << " */ " << std::setw(22)
           << std::left << edge_str;
        auto name = std::string(next->name) + (next->is_final ? " (fin)" : "");
        os << " /* " << std::setw(10) << name << "*/\n";
    }
    os << "};\n";

    // Reset indices
    for (auto node : nodes) {
        node->index = 0;
        node->first_edge_index = 0;
    }
}

void Graph::init_node(
    NodePtr node,
    const std::string& str,
    const std::size_t pos,
    const tok::Type type) {
    assert(str.size() > 0);
    assert(pos < str.size());

    bool is_match = (pos + 1 == str.size());
    bool is_name_or_kw = std::isalpha(str[0]);
    bool is_at_keyword = (str[0] == '@');

    // Anythinig alphanumeric and not matching a keyword is a name
    if (is_name_or_kw && !node->edge_for_class(cls::Alnum)) {
        assert(_shared.name);
        node->add_class_edge(_shared.name, cls::Alnum);
    }

    if (is_match) {
        // Add `*' edge and final node
        // Replace `*' edge if already present, i.e. current string is a
        // substring of some other string
        if (type == tok::Add || type == tok::Sub) {
            // Special case: '+' and '-' are signs if next char is a digit
            // NOTE: maybe '+' sign is not needed and can be unary op instead?
            node->add_class_edge(
                make_node(str + " sign", true, tok::PlusSign), cls::Digit);
            node->add_catchall(make_node(str, true, type));

        } else {
            node->replace_catchall(make_node(str, true, type));
        }

    } else if (is_name_or_kw) {
        // Non-alphanumeric, not a match: end of name.
        // May still be a match for a substring keyword, hence the check
        if (!node->catchall()) {
            assert(_shared.name_end);
            node->add_catchall(_shared.name_end);
        }
    } else if (is_at_keyword) {
        if (!node->catchall()) {
            assert(_shared.invalid_kw);
            node->add_catchall(_shared.invalid_kw);
        }
    }
}

Graph::NodePtr
Graph::make_node(std::string name, bool is_final, const tok::Type type) {
    auto node = std::make_shared<Node>(std::move(name));
    node->is_final = is_final;
    node->type = type;
    _nodes.push_back(node);
    return node;
}

void Graph::for_each(const NodeRecCb& cb) {
    cb(_shared.name);
    cb(_shared.name_end);
    cb(_shared.number);
    cb(_shared.number_end);
    cb(_shared.string);
    cb(_shared.string_escape);
    cb(_shared.string_close);
    cb(_shared.string_end);
    cb(_shared.invalid_kw);
    cb(_shared.invalid_kw_end);
    cb(_shared.invalid);
    for (auto it : _shared.comments)
        cb(it.second);
    // NOTE: using _start instead of _nodes for better enumeration order
    char ch = 0;
    while (true) {
        if (_start[ch])
            for_each_rec(_start[ch], cb);
        if (ch < 127) {
            ++ch;
        } else
            break;
    }
}

// TODO: the itself function is currently supposed to check for infinite
// recursion
void Graph::for_each_rec(NodePtr node, const NodeRecCb& cb) {
    if (cb(node)) {
        for (auto edge : node->edges)
            for_each_rec(edge->next.lock(), cb);
    }
}
