#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xs::core {

    struct Value {
        enum class Type { Int, Float, Bool, String };

        Type type{ Type::Int };
        int i{ 0 };
        float f{ 0.0f };
        bool b{ false };
        std::string s;

        static Value make_int(int v) {
            Value x; x.type = Type::Int; x.i = v; return x;
        }
        static Value make_float(float v) {
            Value x; x.type = Type::Float; x.f = v; return x;
        }
        static Value make_bool(bool v) {
            Value x; x.type = Type::Bool; x.b = v; return x;
        }
        static Value make_string(const std::string& v) {
            Value x; x.type = Type::String; x.s = v; return x;
        }

        bool equals(const Value& other) const {
            if (type != other.type) return false;
            switch (type) {
            case Type::Int:    return i == other.i;
            case Type::Float:  return f == other.f;
            case Type::Bool:   return b == other.b;
            case Type::String: return s == other.s;
            default:           return false;
            }
        }
    };

    class Variable final {
    public:
        using Callback = std::function<void(const Value&)>;

        Variable() : m_value(Value::make_int(0)) {}
        explicit Variable(const Value& v) : m_value(v) {}

        const Value& get() const { return m_value; }

        void set(const Value& v) {
            if (m_value.equals(v)) return;
            m_value = v;
            notify();
        }

        std::size_t subscribe(Callback cb) {
            const std::size_t id = ++m_nextId;
            m_subs.push_back(Subscriber{ id, cb });
            if (cb) cb(m_value);
            return id;
        }

        void unsubscribe(std::size_t id) {
            for (std::size_t k = 0; k < m_subs.size(); ++k) {
                if (m_subs[k].id == id) {
                    m_subs.erase(m_subs.begin() + static_cast<long>(k));
                    return;
                }
            }
        }

    private:
        struct Subscriber {
            std::size_t id{};
            Callback cb{};
        };

        void notify() {
            for (auto& s : m_subs) {
                if (s.cb) s.cb(m_value);
            }
        }

        Value m_value;
        std::vector<Subscriber> m_subs;
        std::size_t m_nextId{ 0 };
    };

    class VariableStore final {
    public:
        bool has(const std::string& name) const {
            return m_vars.find(name) != m_vars.end();
        }

        Variable& ensure(const std::string& name, const Value& initial) {
            auto it = m_vars.find(name);
            if (it == m_vars.end()) {
                it = m_vars.insert(std::make_pair(name, Variable(initial))).first;
            }
            return it->second;
        }

        Variable& at(const std::string& name) { return m_vars.at(name); }
        const Variable& at(const std::string& name) const { return m_vars.at(name); }

        void set(const std::string& name, const Value& value) {
            ensure(name, value).set(value);
        }

        Value get(const std::string& name) const {
            return m_vars.at(name).get();
        }

        bool get_bool(const std::string& name, bool fallback) const {
            if (!has(name)) return fallback;
            Value v = get(name);
            return (v.type == Value::Type::Bool) ? v.b : fallback;
        }

        float get_float(const std::string& name, float fallback) const {
            if (!has(name)) return fallback;
            Value v = get(name);
            if (v.type == Value::Type::Float) return v.f;
            if (v.type == Value::Type::Int) return static_cast<float>(v.i);
            return fallback;
        }

        std::string get_string(const std::string& name, const std::string& fallback) const {
            if (!has(name)) return fallback;
            Value v = get(name);
            return (v.type == Value::Type::String) ? v.s : fallback;
        }

    private:
        std::unordered_map<std::string, Variable> m_vars;
    };

}