#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace svg {

namespace detail {

    void HtmlEncodeString(std::ostream& out, std::string_view sv);

    template <typename T>
    inline void RenderValue(std::ostream& out, const T& value) {
        out << value;
    }

    template <>
    inline void RenderValue<std::string>(std::ostream& out, const std::string& s) {
        HtmlEncodeString(out, s);
    }

    template <typename AttrType>
    inline void RenderAttr(std::ostream& out, std::string_view name, const AttrType& value) {
        using namespace std::literals;
        out << name << "=\""sv;
        RenderValue(out, value);
        out.put('"');
    }

    template <typename AttrType>
    inline void RenderOptionalAttr(std::ostream& out, std::string_view name,
                                    const std::optional<AttrType>& value) {
        if (value) {
            RenderAttr(out, name, *value);
        }
    }

}  // namespace detail

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

struct Rgb {
    Rgb() = default;
    Rgb(uint8_t r, uint8_t g, uint8_t b)
        : red(r), green(g), blue(b) {
    }
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

std::ostream& operator<<(std::ostream& out, Rgb rgb);

// Выполняет линейную интерполяцию Rgb цвета от from до to в зависимости от t
Rgb Lerp(Rgb from, Rgb to, double t);

struct Rgba : public Rgb {
    Rgba() = default;
    Rgba(uint8_t r, uint8_t g, uint8_t b, double op)
        : Rgb(r, g, b), opacity(op) {
    }
    double opacity = 1.0;
};

std::ostream& operator<<(std::ostream& out, Rgba rgba);

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline const Color NoneColor{};

struct ColorPrinter {
    std::ostream& out;
    void operator()(std::monostate) const;
    void operator()(const std::string& str) const;
    void operator()(Rgb rgb) const;
    void operator()(Rgba rgba) const;
};

std::ostream& operator<<(std::ostream& out, Color value);

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

std::ostream& operator<<(std::ostream& out, StrokeLineCap value);

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& out, StrokeLineJoin value);

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }
    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_linecap_ = line_cap;
        return AsOwner();
    }
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_linejoin_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using detail::RenderOptionalAttr;
        using namespace std::literals;

        RenderOptionalAttr(out, " fill"sv, fill_color_);
        RenderOptionalAttr(out, " stroke"sv, stroke_color_);
        RenderOptionalAttr(out, " stroke-width"sv, stroke_width_);
        RenderOptionalAttr(out, " stroke-linecap"sv, stroke_linecap_);
        RenderOptionalAttr(out, " stroke-linejoin"sv, stroke_linejoin_);
    }

private:
    Owner& AsOwner() {
        // static_cast безопасно преобразует *this к Owner&,
        // если класс Owner — наследник PathProps
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_linecap_;
    std::optional<StrokeLineJoin> stroke_linejoin_;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;

    std::vector<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;

    Point position_;
    Point offset_;
    uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

class ObjectContainer {
public:
    /*
     Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
     Пример использования:
     Document doc;
     doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
    */
    template <typename Obj>
    void Add(Obj obj) {
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    } 

    // Добавляет в svg-документ объект-наследник svg::Object
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

protected:
    ~ObjectContainer() = default;
};

class Document : public ObjectContainer {
public:
    void AddPtr(std::unique_ptr<Object>&& obj) override;

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;

    // Прочие методы и данные, необходимые для реализации класса Document
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

class Drawable {
public:
    Drawable() = default;
    virtual ~Drawable() = default;

    // Реализует метод Draw интерфейса svg::Drawable
    virtual void Draw(svg::ObjectContainer& container) const = 0;
};

}  // namespace svg