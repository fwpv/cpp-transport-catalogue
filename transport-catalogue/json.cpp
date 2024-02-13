#include "json.h"

#include <string_view>
#include <array>

using namespace std::literals;

namespace json {

namespace {

// ---------- LoadNode -----------------

Node LoadNode(std::istream& input);

Node LoadNull(std::istream& input) {
    std::string str(4, '\0');
    input.read(str.data(), str.size());
    if (!input) {
        throw ParsingError("Failed to read null from stream"s);
    }
    if (str != "null"s) {
        throw ParsingError("Received "s + str + " instead of null as expected"s);
    }
    char c = input.peek();
    if (!input.eof() && !isspace(c) && c != ']' && c != '}' && c != ',') {
        throw ParsingError("Unexpected character '"s + c +"' after reading null"s);
    }
    return Node{nullptr};
}

template <bool state>
Node LoadBool(std::istream& input) {
    std::string str(state ? 4 : 5, '\0');
    input.read(str.data(), str.size());
    if (!input) {
        throw ParsingError("Failed to read boolean value from stream"s);
    }

    char c = input.peek();
    if (!input.eof() && !isspace(c) && c != ']' && c != '}' && c != ',') {
        throw ParsingError("Unexpected character '"s + c +"' after reading boolean value"s);
    }

    if (str == (state ? "true"s : "false"s)) {
        return Node{state};
    } else {
        throw ParsingError("String "s + str + " is not a boolean value"s);
    }
}

Node LoadNumber(std::istream& input) {
    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return Node{std::stoi(parsed_num)};
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return Node{std::stod(parsed_num)};
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
Node LoadString(std::istream& input) {
    using namespace std::literals;
    
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error"s);
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error"s);
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return Node{s};
}

Node LoadArray(std::istream& input) {
    Array result;

    char c = '\0';
    auto read_non_white_char = [&c, &input] {
        input >> c;
        if (!input) {
            throw ParsingError("Failed to read array from stream"s);
        }
    };

    while (true) {   
        read_non_white_char();
        if (c == ']') {
            break;
        }
        if (result.empty()) {
            input.putback(c);
        } else {
            if (c != ',') {
                throw ParsingError("Received '"s + c + "' instead of ']' or ',' as expected"s);
            }
        }
        result.emplace_back(LoadNode(input));
    }
    return Node(move(result));
}

Node LoadDict(std::istream& input) {
    Dict result;

    char c = '\0';
    auto read_non_white_char = [&c, &input] {
        input >> c;
        if (!input) {
            throw ParsingError("Failed to read map from stream"s);
        }
    };

    while (true) {   
        read_non_white_char();
        if (c == '}') {
            break;
        }
        if (!result.empty()) {
            if (c != ',') {
                throw ParsingError("Received '"s + c + "' instead of '}' or ',' as expected"s);
            }
            read_non_white_char();
        }
        if (c != '"') {
            throw ParsingError("Received '"s + c + "' instead of '\"' as expected"s);
        }
        std::string key = LoadString(input).AsString();

        read_non_white_char();
        if (c != ':') {
            throw ParsingError("Received '"s + c + "' instead of ':' as expected"s);
        }
        result.insert({std::move(key), LoadNode(input)});
    }
    return Node(std::move(result));
}

Node LoadNode(std::istream& input) {
    char c;
    if (!(input >> c)) {
        throw ParsingError("Unexpected EOF"s);
    }
    switch (c) {
        case '[':
            return LoadArray(input);
        case '{':
            return LoadDict(input);
        case '"':
            return LoadString(input);
        case 'n':
            input.putback(c);
            return LoadNull(input);
        case 't':
            input.putback(c);
            return LoadBool<true>(input);
        case 'f':
            input.putback(c);
            return LoadBool<false>(input);
        default:
            input.putback(c);
            return LoadNumber(input);
    }
}

// ---------- PrintNode -----------------

struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

void PrintNode(const Node& node, const PrintContext& ctx);

// Шаблон, подходящий для вывода double и int
template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx) {
    ctx.out << value;
}

// Перегрузка функции PrintValue для вывода значений null
void PrintValue(std::nullptr_t, const PrintContext& ctx) {
    ctx.out << "null"s;
}

// Перегрузка функции PrintValue для вывода значений bool
void PrintValue(bool value, const PrintContext& ctx) {
    ctx.out << std::boolalpha << value;
}

// Перегрузка функции PrintValue для вывода значений string
void PrintValue(const std::string& str, const PrintContext& ctx) {
    std::ostream& out = ctx.out;
    out.put('"');
    for (const char ch : str) {
        switch (ch) {
            case '\\':
                out << R"/(\\)/"s;
                break;
            case '"':
                out << R"/(\")/"s;
                break;
            case '\n':
                out << R"/(\n)/"s;
                break;
            case '\t':
                out << R"/(\t)/"s;
                break;
            case '\r':
                out << R"/(\r)/"s;
                break;
            default:
                out.put(ch);
                break;
        }
    }
    out.put('"');
}

// Перегрузка функции PrintValue для вывода значений Array
void PrintValue(const Array& array, const PrintContext& ctx) {
    std::ostream& out = ctx.out;
    out << "[\n"s;
    bool first = true;
    auto shifted_ctx = ctx.Indented();
    for (const Node& node : array) {
        if (first) {
            first = false;
        } else {
            out << ", "s;
        }
        shifted_ctx.PrintIndent();
        PrintNode(node, shifted_ctx);
    }
    out << "\n"s;
    ctx.PrintIndent();
    out << "]"s;
}

// Перегрузка функции PrintValue для вывода значений Dict
void PrintValue(const Dict& dict, const PrintContext& ctx) {
    std::ostream& out = ctx.out;
    out << "{\n"s;
    bool first = true;
    auto shifted_ctx = ctx.Indented();
    for (const auto& [str, node] : dict) {
        if (first) {
            first = false;
        } else {
            out << ", \n"s;
        }
        shifted_ctx.PrintIndent();
        PrintValue(str, ctx);
        out << ": "s;
        PrintNode(node, shifted_ctx);
    }
    out << "\n"s;
    ctx.PrintIndent();
    out << "}"s;
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit(
        [&ctx](const auto& value){ PrintValue(value, ctx); },
        node.GetValue());
} 

}  // namespace

bool Node::IsInt() const {
    return std::holds_alternative<int>(*this);
}

bool Node::IsDouble() const {
    return std::holds_alternative<int>(*this)
           || std::holds_alternative<double>(*this);
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(*this);
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(*this);
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(*this);
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(*this);
}

bool Node::IsMap() const {
    return std::holds_alternative<Dict>(*this);
}

int Node::AsInt() const {
    if (IsInt()) {
        return std::get<int>(*this);
    } else {
        throw std::logic_error("The node is not an integer"s);
    }
}

bool Node::AsBool() const {
    if (IsBool()) {
        return std::get<bool>(*this);
    } else {
        throw std::logic_error("The node is not a boolean"s);
    }
}

double Node::AsDouble() const {
    if (IsPureDouble()) {
        return std::get<double>(*this);
    } else if (IsInt()) {
        return static_cast<double>(std::get<int>(*this));
    } else {
        throw std::logic_error("The node is not a double"s);
    }
}

const std::string& Node::AsString() const {
    if (IsString()) {
        return std::get<std::string>(*this);
    } else {
        throw std::logic_error("The node is not a string"s);
    }
}

const Array& Node::AsArray() const {
    if (IsArray()) {
        return std::get<Array>(*this);
    } else {
        throw std::logic_error("The node is not an array"s);
    }
}

const Dict& Node::AsMap() const {
    if (IsMap()) {
        return std::get<Dict>(*this);
    } else {
        throw std::logic_error("The node is not a map"s);
    }
}

bool Node::operator==(const Node& rhs) const {
    return GetValue() == rhs.GetValue();
}

bool Node::operator!=(const Node& rhs) const {
    return !(*this == rhs);
}

Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& rhs) const {
    return root_ == rhs.root_;
}
bool Document::operator!=(const Document& rhs) const {
    return !(*this == rhs);
}

Document Load(std::istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), PrintContext{output});
}

}  // namespace json
