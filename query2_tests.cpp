#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <cmath>

#include "Query.h"

// Простая структура для тестов
struct Item {
    int value;
    std::string name;
    
    Item() : value(0), name("") {}
    Item(int v, const std::string& n) : value(v), name(n) {}
    
    int getValue() const { return value; }
    const std::string& getName() const { return name; }
    bool isActive() const { return value > 0; }
    void setValue(int v) { value = v; }
};

// Класс для тестирования указателей на методы
class TestClass {
public:
    TestClass(int v) : val(v) {}
    int getVal() const { return val; }
    bool isPositive() const { return val > 0; }
    void increment() { val++; }
private:
    int val;
};

// Вспомогательная функция для сравнения floating point
bool approxEqual(double a, double b, double epsilon = 1e-6) {
    return std::abs(a - b) < epsilon;
}

// Флаг для отслеживания результатов тестов
int testsPassed = 0;
int testsFailed = 0;

#define TEST(name) void name()
#define RUN_TEST(name) do { \
    std::cout << "Running " << #name << "... "; \
    try { \
        name(); \
        std::cout << "PASSED" << std::endl; \
        testsPassed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << std::endl; \
        testsFailed++; \
    } catch (...) { \
        std::cout << "FAILED: Unknown exception" << std::endl; \
        testsFailed++; \
    } \
} while(0)

#define ASSERT_TRUE(cond) if (!(cond)) throw std::runtime_error("Assertion failed: " #cond)
#define ASSERT_FALSE(cond) if (cond) throw std::runtime_error("Assertion failed: NOT " #cond)
#define ASSERT_EQ(a, b) if ((a) != (b)) throw std::runtime_error("Assertion failed: " #a " == " #b)
#define ASSERT_NEAR(a, b, eps) if (!approxEqual(a, b, eps)) throw std::runtime_error("Assertion failed: " #a " ~= " #b)

// ============================================================================
// Тесты для For()
// ============================================================================

TEST(test_for_with_vector) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto query = Query2::For(vec);
    ASSERT_EQ(query.Count(), 5u);
}

TEST(test_for_with_begin_end) {
    std::vector<int> vec = {10, 20, 30};
    auto query = Query2::For(vec.begin(), vec.end());
    ASSERT_EQ(query.Count(), 3u);
}

TEST(test_for_empty_container) {
    std::vector<int> vec;
    auto query = Query2::For(vec);
    ASSERT_EQ(query.Count(), 0u);
}

// ============================================================================
// Тесты для Where()
// ============================================================================

TEST(test_where_basic) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto result = Query2::For(vec).Where([](int x) { return x % 2 == 0; });
    ASSERT_EQ(result.Count(), 3u);
}

TEST(test_where_with_lambda_capture) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    int threshold = 3;
    auto result = Query2::For(vec).Where([threshold](int x) { return x > threshold; });
    ASSERT_EQ(result.Count(), 2u);
}

TEST(test_where_chained) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto result = Query2::For(vec)
        .Where([](int x) { return x % 2 == 0; })
        .Where([](int x) { return x > 5; });
    ASSERT_EQ(result.Count(), 3u); // 6, 8, 10
}

TEST(test_where_with_member_function_pointer) {
    std::vector<Item> items = {Item(1, "a"), Item(-1, "b"), Item(3, "c"), Item(0, "d")};
    auto result = Query2::For(items).Where(&Item::isActive);
    ASSERT_EQ(result.Count(), 2u); // только элементы с value > 0
}

TEST(test_where_not_null) {
    std::vector<std::shared_ptr<int>> ptrs = {
        std::make_shared<int>(1),
        nullptr,
        std::make_shared<int>(3),
        nullptr,
        std::make_shared<int>(5)
    };
    auto result = Query2::For(ptrs).Where(NotNull{});
    ASSERT_EQ(result.Count(), 3u);
}

// ============================================================================
// Тесты для Apply()
// ============================================================================

TEST(test_apply_basic) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec).Apply([](int x) { return x * 2; });
    ASSERT_EQ(result.Count(), 5u);
    
    std::vector<int> output;
    result.Insert(output);
    ASSERT_EQ(output[0], 2);
    ASSERT_EQ(output[4], 10);
}

TEST(test_apply_with_member_function) {
    std::vector<Item> items = {Item(1, "a"), Item(2, "b"), Item(3, "c")};
    auto result = Query2::For(items).Apply(&Item::getValue);
    
    std::vector<int> output;
    result.Insert(output);
    ASSERT_EQ(output[0], 1);
    ASSERT_EQ(output[1], 2);
    ASSERT_EQ(output[2], 3);
}

TEST(test_apply_chained_with_where) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec)
        .Where([](int x) { return x % 2 == 1; })  // нечётные: 1, 3, 5
        .Apply([](int x) { return x * 10; });     // 10, 30, 50
    
    ASSERT_EQ(result.Count(), 3u);
    
    std::vector<int> output;
    result.Insert(output);
    ASSERT_EQ(output[0], 10);
    ASSERT_EQ(output[1], 30);
    ASSERT_EQ(output[2], 50);
}

// ============================================================================
// Тесты для Count() и CountIf()
// ============================================================================

TEST(test_count_basic) {
    std::vector<int> vec = {1, 2, 2, 3, 2, 4};
    auto query = Query2::For(vec);
    ASSERT_EQ(query.Count(2), 3u);
    ASSERT_EQ(query.Count(5), 0u);
}

TEST(test_count_if) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto query = Query2::For(vec);
    ASSERT_EQ(query.CountIf([](int x) { return x % 2 == 0; }), 3u);
}

TEST(test_count_empty) {
    std::vector<int> vec;
    auto query = Query2::For(vec);
    ASSERT_EQ(query.Count(), 0u);
}

// ============================================================================
// Тесты для Accumulate()
// ============================================================================

TEST(test_accumulate_basic) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Accumulate(), 15);
}

TEST(test_accumulate_empty) {
    std::vector<int> vec;
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Accumulate(), 0);
}

TEST(test_accumulate_after_filter) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto result = Query2::For(vec).Where([](int x) { return x % 2 == 0; });
    ASSERT_EQ(result.Accumulate(), 12); // 2 + 4 + 6
}

// ============================================================================
// Тесты для Average()
// ============================================================================

TEST(test_average_basic) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec);
    ASSERT_NEAR(result.Average(), 3.0, 1e-6);
}

TEST(test_average_empty) {
    std::vector<int> vec;
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Average(), 0);
}

TEST(test_average_after_transform) {
    std::vector<int> vec = {1, 2, 3};
    auto result = Query2::For(vec).Apply([](int x) { return x * 10; });
    ASSERT_NEAR(result.Average(), 20.0, 1e-6);
}

// ============================================================================
// Тесты для Median()
// ============================================================================

TEST(test_median_odd) {
    std::vector<int> vec = {3, 1, 4, 1, 5};
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Median(), 3); // отсортировано: 1, 1, 3, 4, 5
}

TEST(test_median_even) {
    std::vector<int> vec = {1, 2, 3, 4};
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Median(), 2); // (2 + 3) / 2 = 2 (целочисленное деление)
}

TEST(test_median_empty) {
    std::vector<int> vec;
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Median(), 0);
}

// ============================================================================
// Тесты для Max() и Min()
// ============================================================================

TEST(test_max_basic) {
    std::vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Max(), 9);
}

TEST(test_min_basic) {
    std::vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Min(), 1);
}

TEST(test_max_empty) {
    std::vector<int> vec;
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Max(), 0);
}

TEST(test_min_empty) {
    std::vector<int> vec;
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Min(), 0);
}

TEST(test_max_with_comparator) {
    std::vector<Item> items = {Item(1, "a"), Item(5, "b"), Item(3, "c")};
    auto result = Query2::For(items);
    auto maxItem = result.Max([](const Item& a, const Item& b) {
        return a.value < b.value;
    });
    ASSERT_EQ(maxItem.value, 5);
}

TEST(test_spread) {
    std::vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Spread(), 8); // 9 - 1
}

TEST(test_spread_empty) {
    std::vector<int> vec;
    auto result = Query2::For(vec);
    ASSERT_EQ(result.Spread(), 0);
}

// ============================================================================
// Тесты для Any(), All(), None()
// ============================================================================

TEST(test_any_true) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec);
    ASSERT_TRUE(result.Any([](int x) { return x > 3; }));
}

TEST(test_any_false) {
    std::vector<int> vec = {1, 2, 3};
    auto result = Query2::For(vec);
    ASSERT_FALSE(result.Any([](int x) { return x > 10; }));
}

TEST(test_all_true) {
    std::vector<int> vec = {2, 4, 6, 8};
    auto result = Query2::For(vec);
    ASSERT_TRUE(result.All([](int x) { return x % 2 == 0; }));
}

TEST(test_all_false) {
    std::vector<int> vec = {1, 2, 3, 4};
    auto result = Query2::For(vec);
    ASSERT_FALSE(result.All([](int x) { return x % 2 == 0; }));
}

TEST(test_none_true) {
    std::vector<int> vec = {1, 3, 5};
    auto result = Query2::For(vec);
    ASSERT_TRUE(result.None([](int x) { return x % 2 == 0; }));
}

TEST(test_none_false) {
    std::vector<int> vec = {1, 2, 3};
    auto result = Query2::For(vec);
    ASSERT_FALSE(result.None([](int x) { return x % 2 == 0; }));
}

// ============================================================================
// Тесты для Any(), All(), None() без функтора
// ============================================================================

TEST(test_any_no_func_true) {
    std::vector<int> vec = {0, 0, 1, 0};
    auto result = Query2::For(vec);
    ASSERT_TRUE(result.Any());
}

TEST(test_any_no_func_false) {
    std::vector<int> vec = {0, 0, 0};
    auto result = Query2::For(vec);
    ASSERT_FALSE(result.Any());
}

TEST(test_all_no_func_true) {
    std::vector<int> vec = {1, 2, 3};
    auto result = Query2::For(vec);
    ASSERT_TRUE(result.All());
}

TEST(test_all_no_func_false) {
    std::vector<int> vec = {1, 0, 3};
    auto result = Query2::For(vec);
    ASSERT_FALSE(result.All());
}

TEST(test_none_no_func_true) {
    std::vector<int> vec = {0, 0, 0};
    auto result = Query2::For(vec);
    ASSERT_TRUE(result.None());
}

TEST(test_none_no_func_false) {
    std::vector<int> vec = {0, 1, 0};
    auto result = Query2::For(vec);
    ASSERT_FALSE(result.None());
}

TEST(test_any_with_shared_ptr) {
    std::vector<std::shared_ptr<int>> ptrs = {
        nullptr,
        std::make_shared<int>(42),
        nullptr
    };
    auto result = Query2::For(ptrs);
    ASSERT_TRUE(result.Any());
}

TEST(test_all_with_shared_ptr) {
    std::vector<std::shared_ptr<int>> ptrs = {
        std::make_shared<int>(1),
        std::make_shared<int>(2)
    };
    auto result = Query2::For(ptrs);
    ASSERT_TRUE(result.All());
}

TEST(test_none_with_shared_ptr) {
    std::vector<std::shared_ptr<int>> ptrs = {
        nullptr,
        nullptr,
        nullptr
    };
    auto result = Query2::For(ptrs);
    ASSERT_TRUE(result.None());
}

// ============================================================================
// Тесты для Do() и Call()
// ============================================================================

TEST(test_do_basic) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    int sum = 0;
    Query2::For(vec).Do([&sum](int x) { sum += x; });
    ASSERT_EQ(sum, 15);
}

TEST(test_call_with_member_function) {
    std::vector<TestClass> items = {TestClass(1), TestClass(2), TestClass(3)};
    int sum = 0;
    Query2::For(items).Call([&sum](const TestClass& obj) { sum += obj.getVal(); });
    
    ASSERT_EQ(sum, 6); // 1 + 2 + 3
}

TEST(test_call_with_member_function_const) {
    std::vector<TestClass> items = {TestClass(1), TestClass(2), TestClass(3)};
    int sum = 0;
    Query2::For(items).Call([&sum](const TestClass& obj) { sum += obj.getVal(); });
    
    ASSERT_EQ(sum, 6); // 1 + 2 + 3
}

// ============================================================================
// Тесты для Insert()
// ============================================================================

TEST(test_insert_basic) {
    std::vector<int> source = {1, 2, 3, 4, 5};
    std::vector<int> dest;
    
    Query2::For(source).Insert(dest);
    
    ASSERT_EQ(dest.size(), 5u);
    ASSERT_EQ(dest[0], 1);
    ASSERT_EQ(dest[4], 5);
}

TEST(test_insert_after_transform) {
    std::vector<int> source = {1, 2, 3};
    std::vector<int> dest;
    
    Query2::For(source)
        .Apply([](int x) { return x * 100; })
        .Insert(dest);
    
    ASSERT_EQ(dest[0], 100);
    ASSERT_EQ(dest[1], 200);
    ASSERT_EQ(dest[2], 300);
}

// ============================================================================
// Тесты для ExcludeLastElement()
// ============================================================================

TEST(test_exclude_last_element) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec).ExcludeLastElement();
    ASSERT_EQ(result.Count(), 4u);
    
    std::vector<int> output;
    result.Insert(output);
    ASSERT_EQ(output[0], 1);
    ASSERT_EQ(output[3], 4);
}

TEST(test_exclude_last_element_empty) {
    std::vector<int> vec;
    auto result = Query2::For(vec).ExcludeLastElement();
    ASSERT_EQ(result.Count(), 0u);
}

TEST(test_exclude_last_element_single) {
    std::vector<int> vec = {42};
    auto result = Query2::For(vec).ExcludeLastElement();
    ASSERT_EQ(result.Count(), 0u);
}

// ============================================================================
// Тесты для Less, Greater, Is, IsNot
// ============================================================================

TEST(test_less) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec).Where(Less(3));
    ASSERT_EQ(result.Count(), 2u); // 1, 2
}

TEST(test_greater) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec).Where(Greater(3));
    ASSERT_EQ(result.Count(), 2u); // 4, 5
}

TEST(test_is) {
    std::vector<int> vec = {1, 2, 3, 2, 1};
    auto result = Query2::For(vec).Where(Is(2));
    ASSERT_EQ(result.Count(), 2u);
}

TEST(test_is_not) {
    std::vector<int> vec = {1, 2, 3, 2, 1};
    auto result = Query2::For(vec).Where(IsNot(2));
    ASSERT_EQ(result.Count(), 3u); // 1, 3, 1
}

TEST(test_less_equal) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec).Where(LessEqual(3));
    ASSERT_EQ(result.Count(), 3u); // 1, 2, 3
}

TEST(test_greater_equal) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec).Where(GreaterEqual(3));
    ASSERT_EQ(result.Count(), 3u); // 3, 4, 5
}

// ============================================================================
// Тесты для Not()
// ============================================================================

TEST(test_not_predicate) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Query2::For(vec).Where(Not([](int x) { return x % 2 == 0; }));
    ASSERT_EQ(result.Count(), 3u); // 1, 3, 5
}

// ============================================================================
// Тесты для Less/Greater/Is/IsNot с оператором (OP)
// ============================================================================

TEST(test_less_with_op) {
    std::vector<Item> items = {Item(1, "a"), Item(5, "b"), Item(3, "c")};
    auto result = Query2::For(items).Where(Less(&Item::getValue, 4));
    ASSERT_EQ(result.Count(), 2u); // 1, 3
}

TEST(test_greater_with_op) {
    std::vector<Item> items = {Item(1, "a"), Item(5, "b"), Item(3, "c")};
    auto result = Query2::For(items).Where(Greater(&Item::getValue, 2));
    ASSERT_EQ(result.Count(), 2u); // 5, 3
}

TEST(test_is_with_op) {
    std::vector<Item> items = {Item(1, "a"), Item(5, "b"), Item(3, "c")};
    auto result = Query2::For(items).Where(Is(&Item::getValue, 5));
    ASSERT_EQ(result.Count(), 1u);
}

TEST(test_is_not_with_op) {
    std::vector<Item> items = {Item(1, "a"), Item(5, "b"), Item(3, "c")};
    auto result = Query2::For(items).Where(IsNot(&Item::getValue, 5));
    ASSERT_EQ(result.Count(), 2u);
}

// ============================================================================
// Комплексные тесты цепочек
// ============================================================================

TEST(test_complex_chain_1) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    auto result = Query2::For(vec)
        .Where([](int x) { return x % 2 == 0; })      // чётные: 2, 4, 6, 8, 10
        .Apply([](int x) { return x * x; })           // квадраты: 4, 16, 36, 64, 100
        .Where([](int x) { return x > 20; });         // > 20: 36, 64, 100
    
    ASSERT_EQ(result.Count(), 3u);
    ASSERT_EQ(result.Accumulate(), 200); // 36 + 64 + 100
}

TEST(test_complex_chain_2) {
    std::vector<Item> items = {
        Item(10, "apple"),
        Item(-5, "banana"),
        Item(20, "cherry"),
        Item(0, "date"),
        Item(15, "elderberry")
    };
    
    auto result = Query2::For(items)
        .Where(&Item::isActive)                              // активные: 10, 20, 15
        .Apply(&Item::getValue)                              // значения: 10, 20, 15
        .Where(Greater(10));                         // > 10: 20, 15
    
    ASSERT_EQ(result.Count(), 2u);
    ASSERT_NEAR(result.Average(), 17.5, 1e-6);
}

TEST(test_complex_chain_with_strings) {
    std::vector<std::string> strings = {"hello", "world", "foo", "bar", "baz"};
    
    auto result = Query2::For(strings)
        .Where([](const std::string& s) { return s.length() >= 4; })
        .Apply([](const std::string& s) { return s.length(); });
    
    ASSERT_EQ(result.Count(), 2u); // "hello", "world"
}

// ============================================================================
// Тесты для работы с shared_ptr
// ============================================================================

TEST(test_shared_ptr_with_method) {
    std::vector<std::shared_ptr<TestClass>> ptrs = {
        std::make_shared<TestClass>(5),
        std::make_shared<TestClass>(10),
        std::make_shared<TestClass>(15)
    };
    
    auto result = Query2::For(ptrs)
        .Where(NotNull{})
        .Apply(&TestClass::getVal);
    
    std::vector<int> output;
    result.Insert(output);
    
    ASSERT_EQ(output[0], 5);
    ASSERT_EQ(output[1], 10);
    ASSERT_EQ(output[2], 15);
}

// ============================================================================
// Главная функция запуска тестов
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Query2 Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // For()
    RUN_TEST(test_for_with_vector);
    RUN_TEST(test_for_with_begin_end);
    RUN_TEST(test_for_empty_container);
    
    // Where()
    RUN_TEST(test_where_basic);
    RUN_TEST(test_where_with_lambda_capture);
    RUN_TEST(test_where_chained);
    RUN_TEST(test_where_with_member_function_pointer);
    RUN_TEST(test_where_not_null);
    
    // Apply()
    RUN_TEST(test_apply_basic);
    RUN_TEST(test_apply_with_member_function);
    RUN_TEST(test_apply_chained_with_where);
    
    // Count()
    RUN_TEST(test_count_basic);
    RUN_TEST(test_count_if);
    RUN_TEST(test_count_empty);
    
    // Accumulate()
    RUN_TEST(test_accumulate_basic);
    RUN_TEST(test_accumulate_empty);
    RUN_TEST(test_accumulate_after_filter);
    
    // Average()
    RUN_TEST(test_average_basic);
    RUN_TEST(test_average_empty);
    RUN_TEST(test_average_after_transform);
    
    // Median()
    RUN_TEST(test_median_odd);
    RUN_TEST(test_median_even);
    RUN_TEST(test_median_empty);
    
    // Max/Min/Spread
    RUN_TEST(test_max_basic);
    RUN_TEST(test_min_basic);
    RUN_TEST(test_max_empty);
    RUN_TEST(test_min_empty);
    RUN_TEST(test_max_with_comparator);
    RUN_TEST(test_spread);
    RUN_TEST(test_spread_empty);
    
    // Any/All/None
    RUN_TEST(test_any_true);
    RUN_TEST(test_any_false);
    RUN_TEST(test_all_true);
    RUN_TEST(test_all_false);
    RUN_TEST(test_none_true);
    RUN_TEST(test_none_false);
    
    // Any/All/None без функтора
    RUN_TEST(test_any_no_func_true);
    RUN_TEST(test_any_no_func_false);
    RUN_TEST(test_all_no_func_true);
    RUN_TEST(test_all_no_func_false);
    RUN_TEST(test_none_no_func_true);
    RUN_TEST(test_none_no_func_false);
    RUN_TEST(test_any_with_shared_ptr);
    RUN_TEST(test_all_with_shared_ptr);
    RUN_TEST(test_none_with_shared_ptr);
    
    // Do/Call
    RUN_TEST(test_do_basic);
    RUN_TEST(test_call_with_member_function);
    RUN_TEST(test_call_with_member_function_const);
    
    // Insert
    RUN_TEST(test_insert_basic);
    RUN_TEST(test_insert_after_transform);
    
    // ExcludeLastElement
    RUN_TEST(test_exclude_last_element);
    RUN_TEST(test_exclude_last_element_empty);
    RUN_TEST(test_exclude_last_element_single);
    
    // Less/Greater/Is/IsNot
    RUN_TEST(test_less);
    RUN_TEST(test_greater);
    RUN_TEST(test_is);
    RUN_TEST(test_is_not);
    RUN_TEST(test_less_equal);
    RUN_TEST(test_greater_equal);
    
    // Not
    RUN_TEST(test_not_predicate);
    
    // Less/Greater/Is/IsNot with OP
    RUN_TEST(test_less_with_op);
    RUN_TEST(test_greater_with_op);
    RUN_TEST(test_is_with_op);
    RUN_TEST(test_is_not_with_op);
    
    // Complex chains
    RUN_TEST(test_complex_chain_1);
    RUN_TEST(test_complex_chain_2);
    RUN_TEST(test_complex_chain_with_strings);
    
    // Shared_ptr
    RUN_TEST(test_shared_ptr_with_method);
    
    std::cout << "========================================" << std::endl;
    std::cout << "Tests passed: " << testsPassed << std::endl;
    std::cout << "Tests failed: " << testsFailed << std::endl;
    std::cout << "========================================" << std::endl;
    
    return testsFailed == 0 ? 0 : 1;
}
