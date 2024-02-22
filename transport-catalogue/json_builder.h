#include "json.h"

#include <string>
#include <vector>

namespace json {

class Builder;
class DictValueContext;
class DictItemContext;
class ArrayValueContext;

class BaseContext {
public:
    BaseContext(Builder& builder);
    Node Build();

    DictValueContext Key(std::string key);
    ArrayValueContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayValueContext StartArray();
    BaseContext EndDict();
    BaseContext EndArray();

protected:
    Builder& builder_;
};

class DictValueContext : public BaseContext {
public:
    DictValueContext(BaseContext base, std::string key);
    Node Build() = delete;

    DictItemContext Key(std::string key) = delete;
    DictItemContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayValueContext StartArray();
    BaseContext EndDict() = delete;
    BaseContext EndArray() = delete;

private:
    std::string key_;
};

class DictItemContext : public BaseContext {
public:
    DictItemContext(BaseContext base);
    Node Build() = delete;

    BaseContext Value(Node::Value value) = delete;
    DictItemContext StartDict() = delete;
    ArrayValueContext StartArray() = delete;
};

class ArrayValueContext : public BaseContext {
public:
    ArrayValueContext(BaseContext base);
    Node Build() = delete;

    DictValueContext Key(std::string key) = delete;
    BaseContext EndDict() = delete;
};

class Builder {
public:
    Builder() = default;
    Builder(const Builder&) = default;
    Builder(Builder&&) = default;

    Node Build();

    DictItemContext Key(std::string key);
    BaseContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayValueContext StartArray();
    BaseContext EndDict();
    BaseContext EndArray();

    Node::Value& GetCurrentValue();
    void PopCurrentValue();
    void PutNodeOnStack(Node* node);

private:
    Node root_;
    std::vector<Node*> node_stack_;
    bool ready_ = false;
};

}  // namespace json