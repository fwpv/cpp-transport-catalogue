
#include "json_builder.h"

#include <exception>
#include <utility>
#include <variant>

using namespace std::literals;

namespace json {

BaseContext::BaseContext(Builder& builder)
    : builder_(builder) {
}

Node BaseContext::Build() {
    return builder_.Build();
}

DictValueContext BaseContext::Key(std::string key) {
    Node::Value& dict_value = builder_.GetCurrentValue();
    if (!std::holds_alternative<Dict>(dict_value)) {
        throw std::logic_error("Attempt to insert Key into not a Dict"s);
    } else {
        return DictValueContext(*this, std::move(key));
    }
}

ArrayValueContext BaseContext::Value(Node::Value value) {
    Node::Value& array_value = builder_.GetCurrentValue();
    if (!std::holds_alternative<Array>(array_value)) {
        throw std::logic_error("Attempt to insert a Value not into an Array"s);
    }
    Array& arr = std::get<Array>(array_value);
    arr.emplace_back(std::move(value));
    return ArrayValueContext(*this);
}

DictItemContext BaseContext::StartDict() {
    Node::Value& array_value = builder_.GetCurrentValue();
    if (!std::holds_alternative<Array>(array_value)
        && !std::holds_alternative<Dict>(array_value)) {
        throw std::logic_error("An attempt to Start Dict not into an Array or Dict"s);
    }
    Array& arr = std::get<Array>(array_value);
    arr.emplace_back(Dict{});
    builder_.PutNodeOnStack(&arr.back());
    return DictItemContext(*this);
}

ArrayValueContext BaseContext::StartArray() {
    Node::Value& array_value = builder_.GetCurrentValue();
    if (!std::holds_alternative<Array>(array_value)
        && !std::holds_alternative<Dict>(array_value)) {
        throw std::logic_error("Attempt to Start Array not into an Array or Dict"s);
    }
    Array& arr = std::get<Array>(array_value);
    arr.emplace_back(Array{});
    builder_.PutNodeOnStack(&arr.back());
    return ArrayValueContext(*this);
}

BaseContext BaseContext::EndDict() {
    Node::Value& dict_value = builder_.GetCurrentValue();
    if (!std::holds_alternative<Dict>(dict_value)) {
        throw std::logic_error("Attempt to End not a Dict"s);
    }
    builder_.PopCurrentValue();
    return BaseContext(*this);
}

BaseContext BaseContext::EndArray() {
    Node::Value& array_value = builder_.GetCurrentValue();
    if (!std::holds_alternative<Array>(array_value)) {
        throw std::logic_error("Attempt to End not a Array"s);
    }
    builder_.PopCurrentValue();
    return BaseContext(*this);
}

DictValueContext::DictValueContext(BaseContext base, std::string key)
    : BaseContext(base)
    , key_(std::move(key)) {
}

DictItemContext DictValueContext::Value(Node::Value value) {
    Node::Value& dict_value = builder_.GetCurrentValue();
    Dict& dict = std::get<Dict>(dict_value);
    auto it = dict.emplace(std::move(key_), std::move(value));
    if (!it.second) {
        throw std::logic_error("Attempt to insert a duplicate key"s);
    }
    return DictItemContext(*this);
}

DictItemContext DictValueContext::StartDict() {
    Node::Value& dict_value = builder_.GetCurrentValue();
    Dict& dict = std::get<Dict>(dict_value);
    auto it = dict.emplace(std::move(key_), Dict{});
    if (!it.second) {
        throw std::logic_error("Attempt to insert a duplicate key"s);
    } else {
        builder_.PutNodeOnStack(&it.first->second);
    }
    return DictItemContext(*this);
}

ArrayValueContext DictValueContext::StartArray() {
    Node::Value& dict_value = builder_.GetCurrentValue();
    Dict& dict = std::get<Dict>(dict_value);
    auto it = dict.emplace(std::move(key_), Array{});
    if (!it.second) {
        throw std::logic_error("Attempt to insert a duplicate key"s);
    } else {
        builder_.PutNodeOnStack(&it.first->second);
    }
    return ArrayValueContext(*this);
}

DictItemContext::DictItemContext(BaseContext base)
    : BaseContext(base) {
}

ArrayValueContext::ArrayValueContext(BaseContext base)
    : BaseContext(base) {
}

Node Builder::Build() {
    if (!ready_) {
        throw std::logic_error("Not finished builder"s);
    }
    //return std::move(*root_.get());
    return std::move(root_);
}

DictItemContext Builder::Key(std::string) {
    throw std::logic_error("Calling the Key method from outside Dict"s);
}

BaseContext Builder::Value(Node::Value value) {
    root_ = std::move(value);
    //root_ = std::make_shared<Node>(std::move(value));
    ready_ = true;
    return BaseContext(*this);
}

DictItemContext Builder::StartDict() {
    root_ = Dict{};
    //root_ = std::make_shared<Node>(Dict{});
    node_stack_.push_back(&root_);
    return DictItemContext(*this);
}

ArrayValueContext Builder::StartArray() {
    root_ = Array{};
    //root_ = std::make_shared<Node>(Array{});
    node_stack_.push_back(&root_);
    return ArrayValueContext(*this);
}
BaseContext Builder::EndDict() {
    throw std::logic_error("Dict is not defined"s);
}

BaseContext Builder::EndArray() {
    throw std::logic_error("Array is not defined"s);
}

Node::Value& Builder::GetCurrentValue() {
    if (node_stack_.empty()) {
        throw std::logic_error("Trying to get the missing value"s);
    }
    return node_stack_.back()->GetValue();
}

void Builder::PopCurrentValue() {
    node_stack_.pop_back();
    if (node_stack_.empty()) {
        ready_ = true;
    }
}

void Builder::PutNodeOnStack(Node* node) {
    node_stack_.push_back(node);
}

}  // namespace json