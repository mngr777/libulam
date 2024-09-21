#pragma once
#include "libulam/token.hpp"
#include "src/lexer/dfa.hpp"
#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace dfa = ulam::lex::dfa;
namespace tok = ulam::tok;

class Graph {
private:
    struct Node;
    struct Edge;
    using NodePtr = std::shared_ptr<Node>;
    using NodeWeakPtr = std::weak_ptr<Node>;
    using EdgePtr = std::shared_ptr<Edge>;

    using NodeRecCb = std::function<bool(NodePtr)>;
    using NodeMatchCb = std::function<void(NodePtr, bool /* is full match? */)>;

    using NodeList = std::vector<NodePtr>;
    using EdgeList = std::vector<EdgePtr>;

    struct Edge {
        char chr{'\0'};
        dfa::CatFlags cat{dfa::cat::None};
        NodeWeakPtr next;

        std::string to_string() const;
    };

    struct Node {
        Node(std::string name): name(std::move(name)) {}

        std::string name;
        dfa::Result result{dfa::Result::None};
        tok::Type type{tok::None};
        std::vector<EdgePtr> edges;
        std::size_t index{0};
        std::size_t first_edge_index{0};

        void check_valid();

        void add_edge(NodePtr next, char chr, dfa::CatFlags cat);

        void add_cat_edge(NodePtr next, dfa::CatFlags cat) {
            add_edge(next, '\0', cat);
        }
        void add_catchall(NodePtr next) { add_cat_edge(next, dfa::cat::Any); }

        void remove_edge(EdgePtr edge);

        EdgePtr edge_for(char ch);

        EdgePtr edge_for_cat(dfa::CatFlags cat);

        EdgePtr catchall() { return edge_for_cat(dfa::cat::Any); }

        std::string to_string() const;
    };

public:
    Graph();

    void add(const std::string& str, tok::Type type);

    void add_comment(const std::string& open, const std::string& close = "\n");

    // Last matching node, string position after last match
    std::pair<NodePtr, std::size_t> find(const std::string& str);

    void gen(std::ostream& os);

private:
    void init_node(
        NodePtr node,
        const std::string& str,
        const std::size_t pos,
        const tok::Type type);

    NodePtr make_node(
        std::string name,
        const dfa::Result result = dfa::Result::None,
        const tok::Type type = tok::None);

    void for_each(const NodeRecCb& cb);

    void for_each_rec(NodePtr node, const NodeRecCb& cb);

    // By closing sequence ('\n' or `*/')
    using CommentMap = std::unordered_map<std::string, NodePtr>;
    struct {
        // TODO: use dictionary
        NodePtr name;
        NodePtr name_end;
        NodePtr number;
        NodePtr number_end;
        NodePtr string;
        NodePtr string_escape;
        NodePtr string_close;
        NodePtr string_end;
        NodePtr invalid_kw;
        NodePtr invalid_kw_end;
        NodePtr invalid;
        CommentMap comments;
    } _shared;

    std::array<NodePtr, 128> _start;
    std::vector<NodePtr> _nodes;
};
