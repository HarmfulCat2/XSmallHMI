#include <SFML/Graphics.hpp>

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "xs_core.hpp"

namespace xs::ui {
    
    struct Theme {
        sf::Color bg{ sf::Color(22, 22, 26) };
        sf::Color panel{ sf::Color(34, 34, 40) };
        sf::Color border{ sf::Color(90, 90, 105) };
        sf::Color text{ sf::Color(235, 235, 240) };
        sf::Color hint{ sf::Color(170, 170, 185) };
        sf::Color accent{ sf::Color(80, 160, 255) };
    };

    static std::string value_to_string(const xs::core::Value& v) {
        switch (v.type) {
        case xs::core::Value::Type::Int:
            return std::to_string(v.i);
        case xs::core::Value::Type::Float: {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << v.f;
            return oss.str();
        }
        case xs::core::Value::Type::Bool:
            return v.b ? "true" : "false";
        case xs::core::Value::Type::String:
            return v.s;
        default:
            return "";
        }
    }

    class Widget {
    public:
        virtual ~Widget() = default;

        virtual void handle_event(const sf::Event& e, const sf::RenderWindow& window) = 0;
        virtual void update(float dt) { (void)dt; }
        virtual void draw(sf::RenderTarget& target) const = 0;

        virtual void set_position(const sf::Vector2f& p) { m_pos = p; }
        virtual void set_size(const sf::Vector2f& s) { m_size = s; }

        sf::Vector2f position() const { return m_pos; }
        sf::Vector2f size() const { return m_size; }

        bool contains(const sf::Vector2f& p) const {
            return (p.x >= m_pos.x && p.x <= m_pos.x + m_size.x &&
                p.y >= m_pos.y && p.y <= m_pos.y + m_size.y);
        }

        void set_enabled(bool enabled) { m_enabled = enabled; }
        bool enabled() const { return m_enabled; }

    protected:
        sf::Vector2f m_pos{ 0.f, 0.f };
        sf::Vector2f m_size{ 0.f, 0.f };
        bool m_enabled{ true };
    };

    class Label final : public Widget {
    public:
        Label(const sf::Font& font, unsigned int charSize, const Theme& theme)
            : m_theme(theme) {
            m_text.setFont(font);
            m_text.setCharacterSize(charSize);
            m_text.setFillColor(m_theme.text);
            m_size = sf::Vector2f(300.f, static_cast<float>(charSize) + 10.f);
        }

        void set_prefix(const std::string& p) { m_prefix = p; rebuild(); }
        void set_text(const std::string& t) { m_valueText = t; rebuild(); }

        void bind_to(xs::core::VariableStore& store, const std::string& varName) {
            store.ensure(varName, xs::core::Value::make_string(""));
            xs::core::Variable& var = store.at(varName);

            m_subId = var.subscribe([this](const xs::core::Value& v) {
                this->set_text(value_to_string(v));
                });
        }

        void handle_event(const sf::Event&, const sf::RenderWindow&) override {
            
        }

        void draw(sf::RenderTarget& target) const override {
            target.draw(m_text);
        }

        void set_position(const sf::Vector2f& p) override {
            Widget::set_position(p);
            m_text.setPosition(m_pos);
        }

    private:
        void rebuild() {
            std::string combined = m_prefix;
            if (!combined.empty()) combined += " ";
            combined += m_valueText;
            m_text.setString(combined);
        }

    private:
        Theme m_theme;
        sf::Text m_text;
        std::string m_prefix;
        std::string m_valueText;
        std::size_t m_subId{ 0 };
    };

    class Button final : public Widget {
    public:
        Button(const sf::Font& font, unsigned int charSize, const Theme& theme)
            : m_theme(theme) {
            m_box.setFillColor(m_theme.panel);
            m_box.setOutlineThickness(1.f);
            m_box.setOutlineColor(m_theme.border);

            m_text.setFont(font);
            m_text.setCharacterSize(charSize);
            m_text.setFillColor(m_theme.text);

            m_size = sf::Vector2f(200.f, 40.f);
            m_box.setSize(m_size);
        }

        void set_caption(const std::string& s) {
            m_caption = s;
            m_text.setString(m_caption);
            center_text();
        }

        void set_on_click(std::function<void()> fn) { m_onClick = std::move(fn); }

        void bind_toggle_bool(xs::core::VariableStore& store, const std::string& varName) {
            store.ensure(varName, xs::core::Value::make_bool(false));
            xs::core::Variable& var = store.at(varName);

            m_subId = var.subscribe([this](const xs::core::Value& v) {
                m_isOn = (v.type == xs::core::Value::Type::Bool) ? v.b : false;
                refresh_style();
                });

            set_on_click([&store, varName]() {
                const bool cur = store.get_bool(varName, false);
                store.set(varName, xs::core::Value::make_bool(!cur));
                });
        }

        void handle_event(const sf::Event& e, const sf::RenderWindow& window) override {
            if (!enabled()) return;

            if (e.type == sf::Event::MouseMoved) {
                const sf::Vector2i mp = sf::Mouse::getPosition(window);
                m_hover = contains(sf::Vector2f(static_cast<float>(mp.x), static_cast<float>(mp.y)));
                refresh_style();
            }

            if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                const sf::Vector2f p(static_cast<float>(e.mouseButton.x), static_cast<float>(e.mouseButton.y));
                if (contains(p)) {
                    m_pressed = true;
                    refresh_style();
                }
            }

            if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left) {
                const sf::Vector2f p(static_cast<float>(e.mouseButton.x), static_cast<float>(e.mouseButton.y));
                const bool wasPressed = m_pressed;
                m_pressed = false;
                refresh_style();

                if (wasPressed && contains(p)) {
                    if (m_onClick) m_onClick();
                }
            }
        }

        void draw(sf::RenderTarget& target) const override {
            target.draw(m_box);
            target.draw(m_text);
        }

        void set_position(const sf::Vector2f& p) override {
            Widget::set_position(p);
            m_box.setPosition(m_pos);
            center_text();
        }

        void set_size(const sf::Vector2f& s) override {
            Widget::set_size(s);
            m_box.setSize(m_size);
            center_text();
        }

    private:
        void center_text() {
            const sf::FloatRect tb = m_text.getLocalBounds();
            const float x = m_pos.x + (m_size.x - tb.width) * 0.5f - tb.left;
            const float y = m_pos.y + (m_size.y - tb.height) * 0.5f - tb.top;
            m_text.setPosition(sf::Vector2f(x, y));
        }

        void refresh_style() {
            if (!enabled()) {
                m_box.setFillColor(sf::Color(45, 45, 52));
                m_box.setOutlineColor(sf::Color(80, 80, 90));
                m_text.setFillColor(sf::Color(150, 150, 160));
                return;
            }

            m_box.setOutlineColor(m_isOn ? m_theme.accent : m_theme.border);

            if (m_pressed) {
                m_box.setFillColor(sf::Color(28, 28, 34));
            }
            else if (m_hover) {
                m_box.setFillColor(sf::Color(40, 40, 48));
            }
            else {
                m_box.setFillColor(m_theme.panel);
            }

            m_text.setFillColor(m_theme.text);
        }

    private:
        Theme m_theme;
        sf::RectangleShape m_box;
        sf::Text m_text;

        std::string m_caption{ "Button" };
        std::function<void()> m_onClick;

        bool m_hover{ false };
        bool m_pressed{ false };
        bool m_isOn{ false };

        std::size_t m_subId{ 0 };
    };

    class TextField final : public Widget {
    public:
        TextField(const sf::Font& font, unsigned int charSize, const Theme& theme)
            : m_theme(theme) {
            m_box.setFillColor(m_theme.panel);
            m_box.setOutlineThickness(1.f);
            m_box.setOutlineColor(m_theme.border);

            m_text.setFont(font);
            m_text.setCharacterSize(charSize);
            m_text.setFillColor(m_theme.text);

            m_hintText.setFont(font);
            m_hintText.setCharacterSize(charSize);
            m_hintText.setFillColor(m_theme.hint);

            m_caret.setSize(sf::Vector2f(1.f, static_cast<float>(charSize)));
            m_caret.setFillColor(m_theme.accent);

            m_size = sf::Vector2f(260.f, 40.f);
            m_box.setSize(m_size);
        }

        void set_hint(const std::string& s) {
            m_hint = s;
            m_hintText.setString(m_hint);
        }

        void set_text(const std::string& s) {
            m_value = s;
            if (m_caretPos > m_value.size()) m_caretPos = m_value.size();
            apply_text();
        }

        void bind_string(xs::core::VariableStore& store, const std::string& varName) {
            store.ensure(varName, xs::core::Value::make_string(""));
            xs::core::Variable& var = store.at(varName);

            m_subId = var.subscribe([this](const xs::core::Value& v) {
                if (!m_focused && v.type == xs::core::Value::Type::String) {
                    this->set_text(v.s);
                }
                });

            m_commit = [&store, varName, this]() {
                store.set(varName, xs::core::Value::make_string(this->m_value));
                };
        }

        void handle_event(const sf::Event& e, const sf::RenderWindow& window) override {
            if (!enabled()) return;

            if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                const sf::Vector2f p(static_cast<float>(e.mouseButton.x), static_cast<float>(e.mouseButton.y));
                const bool nowFocused = contains(p);

                if (m_focused && !nowFocused) {
                    if (m_commit) m_commit();
                }

                m_focused = nowFocused;
                refresh_style();

                if (m_focused) {
                    m_caretPos = m_value.size();
                    update_caret_position();
                }
            }

            if (!m_focused) return;

            if (e.type == sf::Event::TextEntered) {
                const sf::Uint32 code = e.text.unicode;

                if (code == 8) {
                    if (m_caretPos > 0 && !m_value.empty()) {
                        m_value.erase(m_caretPos - 1, 1);
                        --m_caretPos;
                        apply_text();
                    }
                    return;
                }

                if (code == 13) {
                    if (m_commit) m_commit();
                    m_focused = false;
                    refresh_style();
                    return;
                }

                if (code == 9) return;

                if (code >= 32 && code <= 126) {
                    if (m_value.size() < m_maxLen) {
                        m_value.insert(m_caretPos, 1, static_cast<char>(code));
                        ++m_caretPos;
                        apply_text();
                    }
                }
            }

            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Left) {
                    if (m_caretPos > 0) { --m_caretPos; update_caret_position(); }
                }
                else if (e.key.code == sf::Keyboard::Right) {
                    if (m_caretPos < m_value.size()) { ++m_caretPos; update_caret_position(); }
                }
                else if (e.key.code == sf::Keyboard::Escape) {
                    m_focused = false;
                    refresh_style();
                }
            }

            (void)window;
        }

        void update(float dt) override {
            if (!m_focused) { m_caretVisible = false; return; }
            m_blinkTimer += dt;
            if (m_blinkTimer >= 0.5f) {
                m_blinkTimer = 0.f;
                m_caretVisible = !m_caretVisible;
            }
        }

        void draw(sf::RenderTarget& target) const override {
            target.draw(m_box);

            if (!m_value.empty()) target.draw(m_text);
            else target.draw(m_hintText);

            if (m_focused && m_caretVisible) target.draw(m_caret);
        }

        void set_position(const sf::Vector2f& p) override {
            Widget::set_position(p);
            m_box.setPosition(m_pos);

            const float pad = 10.f;
            m_text.setPosition(sf::Vector2f(m_pos.x + pad, m_pos.y + 8.f));
            m_hintText.setPosition(sf::Vector2f(m_pos.x + pad, m_pos.y + 8.f));

            update_caret_position();
        }

        void set_size(const sf::Vector2f& s) override {
            Widget::set_size(s);
            m_box.setSize(m_size);
            update_caret_position();
        }

    private:
        void apply_text() {
            m_text.setString(m_value);
            update_caret_position();
        }

        void update_caret_position() {
            const float pad = 10.f;

            sf::Text tmp = m_text;
            tmp.setString(m_value.substr(0, m_caretPos));
            const sf::FloatRect bounds = tmp.getLocalBounds();

            const float x = m_pos.x + pad + bounds.width;
            const float y = m_text.getPosition().y;
            m_caret.setPosition(sf::Vector2f(x, y));
        }

        void refresh_style() {
            if (!enabled()) {
                m_box.setOutlineColor(sf::Color(80, 80, 90));
                return;
            }
            m_box.setOutlineColor(m_focused ? m_theme.accent : m_theme.border);
            m_blinkTimer = 0.f;
            m_caretVisible = m_focused;
        }

    private:
        Theme m_theme;

        sf::RectangleShape m_box;
        sf::Text m_text;
        sf::Text m_hintText;
        sf::RectangleShape m_caret;

        std::string m_hint{ "Enter text..." };
        std::string m_value;

        bool m_focused{ false };
        std::size_t m_caretPos{ 0 };
        std::size_t m_maxLen{ 32 };

        float m_blinkTimer{ 0.f };
        bool m_caretVisible{ false };

        std::size_t m_subId{ 0 };
        std::function<void()> m_commit;
    };

    class Panel final : public Widget {
    public:
        explicit Panel(const Theme& theme) : m_theme(theme) {
            m_box.setFillColor(m_theme.panel);
            m_box.setOutlineThickness(1.f);
            m_box.setOutlineColor(m_theme.border);
        }

        void add(const std::shared_ptr<Widget>& w) {
            m_children.push_back(w);
        }

        void handle_event(const sf::Event& e, const sf::RenderWindow& window) override {
            for (auto& w : m_children) w->handle_event(e, window);
        }

        void update(float dt) override {
            for (auto& w : m_children) w->update(dt);
        }

        void draw(sf::RenderTarget& target) const override {
            target.draw(m_box);
            for (auto& w : m_children) w->draw(target);
        }

        void set_position(const sf::Vector2f& p) override {
            Widget::set_position(p);
            m_box.setPosition(m_pos);
        }

        void set_size(const sf::Vector2f& s) override {
            Widget::set_size(s);
            m_box.setSize(m_size);
        }

    private:
        Theme m_theme;
        sf::RectangleShape m_box;
        std::vector<std::shared_ptr<Widget>> m_children;
    };

}

static std::string on_off(bool v) { return v ? "ON" : "OFF"; }

int main() {
    sf::RenderWindow window(
        sf::VideoMode(760, 420),
        "XSmall-HMI SCADA - IO Components (SFML)",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setVerticalSyncEnabled(true);

    sf::Font font;
    if (!font.loadFromFile("Roboto-Regular.ttf")) {
        std::cerr << "ERROR: Cannot load font: Roboto-Regular.ttf\n";
        std::cerr << "Put a TTF font near the exe and rename it to Roboto-Regular.ttf\n";
        return 1;
    }

    xs::core::VariableStore vars;
    vars.set("pump.enabled", xs::core::Value::make_bool(false));
    vars.set("operator.name", xs::core::Value::make_string("Ivan"));
    vars.set("temperature", xs::core::Value::make_float(23.50f));
    vars.set("pump.enabled.view", xs::core::Value::make_string("OFF"));
    vars.at("pump.enabled").subscribe([&vars](const xs::core::Value& v) {
        const bool b = (v.type == xs::core::Value::Type::Bool) ? v.b : false;
        vars.set("pump.enabled.view", xs::core::Value::make_string(on_off(b)));
        });

    const xs::ui::Theme theme;

    auto panel = std::make_shared<xs::ui::Panel>(theme);
    panel->set_position(sf::Vector2f(20.f, 20.f));
    panel->set_size(sf::Vector2f(720.f, 380.f));

    auto title = std::make_shared<xs::ui::Label>(font, 22, theme);
    title->set_position(sf::Vector2f(40.f, 35.f));
    title->set_text("IO Components Demo");
    panel->add(title);

    auto pumpLabel = std::make_shared<xs::ui::Label>(font, 18, theme);
    pumpLabel->set_prefix("Pump:");
    pumpLabel->set_position(sf::Vector2f(40.f, 85.f));
    pumpLabel->bind_to(vars, "pump.enabled.view");
    panel->add(pumpLabel);

    auto pumpBtn = std::make_shared<xs::ui::Button>(font, 18, theme);
    pumpBtn->set_position(sf::Vector2f(240.f, 78.f));
    pumpBtn->set_size(sf::Vector2f(220.f, 42.f));
    pumpBtn->set_caption("Toggle pump.enabled");
    pumpBtn->bind_toggle_bool(vars, "pump.enabled");
    panel->add(pumpBtn);

    auto tempLabel = std::make_shared<xs::ui::Label>(font, 18, theme);
    tempLabel->set_prefix("Temperature:");
    tempLabel->set_position(sf::Vector2f(40.f, 145.f));
    tempLabel->bind_to(vars, "temperature");
    panel->add(tempLabel);

    auto tempUp = std::make_shared<xs::ui::Button>(font, 18, theme);
    tempUp->set_position(sf::Vector2f(240.f, 138.f));
    tempUp->set_size(sf::Vector2f(220.f, 42.f));
    tempUp->set_caption("Temperature +0.25");
    tempUp->set_on_click([&vars]() {
        const float t = vars.get_float("temperature", 0.f);
        vars.set("temperature", xs::core::Value::make_float(t + 0.25f));
        });
    panel->add(tempUp);

    auto nameLabel = std::make_shared<xs::ui::Label>(font, 18, theme);
    nameLabel->set_prefix("Operator name:");
    nameLabel->set_position(sf::Vector2f(40.f, 215.f));
    nameLabel->bind_to(vars, "operator.name");
    panel->add(nameLabel);

    auto nameField = std::make_shared<xs::ui::TextField>(font, 18, theme);
    nameField->set_position(sf::Vector2f(240.f, 205.f));
    nameField->set_size(sf::Vector2f(320.f, 42.f));
    nameField->set_hint("Type name, press Enter...");
    nameField->bind_string(vars, "operator.name");
    panel->add(nameField);

    auto tip1 = std::make_shared<xs::ui::Label>(font, 16, theme);
    tip1->set_position(sf::Vector2f(40.f, 290.f));
    tip1->set_text("Variables: pump.enabled, operator.name, temperature");
    panel->add(tip1);

    auto tip2 = std::make_shared<xs::ui::Label>(font, 16, theme);
    tip2->set_position(sf::Vector2f(40.f, 320.f));
    tip2->set_text("Tip: click text field -> type -> Enter to commit");
    panel->add(tip2);

    sf::Clock clock;
    while (window.isOpen()) {
        const float dt = clock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                break;
            }
            panel->handle_event(event, window);
        }

        panel->update(dt);

        window.clear(theme.bg);
        panel->draw(window);
        window.display();
    }

    return 0;
}