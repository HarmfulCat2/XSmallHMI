#include <gtest/gtest.h>

#include "../src/xs_core.hpp"

TEST(Value, Equals_Int) {
    xs::core::Value a = xs::core::Value::make_int(10);
    xs::core::Value b = xs::core::Value::make_int(10);
    xs::core::Value c = xs::core::Value::make_int(11);
    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
}

TEST(Value, Equals_DifferentTypesAreNotEqual) {
    xs::core::Value a = xs::core::Value::make_int(10);
    xs::core::Value b = xs::core::Value::make_float(10.0f);
    EXPECT_FALSE(a.equals(b));
}

TEST(Variable, SubscribeImmediatelyReceivesCurrentValue) {
    xs::core::Variable v(xs::core::Value::make_bool(true));

    bool got = false;
    bool last = false;

    v.subscribe([&](const xs::core::Value& x) {
        got = true;
        last = (x.type == xs::core::Value::Type::Bool) ? x.b : false;
        });

    EXPECT_TRUE(got);
    EXPECT_TRUE(last);
}

TEST(Variable, SetNotifiesSubscribersOnlyOnChange) {
    xs::core::Variable v(xs::core::Value::make_int(1));

    int calls = 0;
    v.subscribe([&](const xs::core::Value&) { ++calls; });

    EXPECT_EQ(calls, 1);

    v.set(xs::core::Value::make_int(1));
    EXPECT_EQ(calls, 1);

    v.set(xs::core::Value::make_int(2));
    EXPECT_EQ(calls, 2);
}

TEST(VariableStore, EnsureCreatesIfMissing) {
    xs::core::VariableStore s;
    EXPECT_FALSE(s.has("a"));

    s.ensure("a", xs::core::Value::make_int(5));
    EXPECT_TRUE(s.has("a"));
    EXPECT_EQ(s.get("a").i, 5);
}

TEST(VariableStore, GetBoolFloatStringFallbacks) {
    xs::core::VariableStore s;

    EXPECT_EQ(s.get_bool("missing", true), true);
    EXPECT_FLOAT_EQ(s.get_float("missing", 1.5f), 1.5f);
    EXPECT_EQ(s.get_string("missing", "x"), "x");

    s.set("b", xs::core::Value::make_bool(false));
    s.set("f", xs::core::Value::make_float(2.25f));
    s.set("s", xs::core::Value::make_string("hello"));

    EXPECT_EQ(s.get_bool("b", true), false);
    EXPECT_FLOAT_EQ(s.get_float("f", 0.f), 2.25f);
    EXPECT_EQ(s.get_string("s", ""), "hello");
}