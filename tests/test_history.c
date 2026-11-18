/* test_history.c: rengaspuskurin testit (vaihe 5).
 *
 * Rajatapaukset: tyhjä, osittain täysi, tasan täysi, yksi yli (ensimmäinen ylikirjoitus)
 * ja monta kierrosta.
 */
#include <stddef.h>

#include "unity.h"
#include "history.h"

static struct history h;

void setUp(void)
{
    h = (struct history){ 0 };
}

void tearDown(void) {}

/* Lisää n riviä: rivin k lämpötila on 1000 + k, aika k * 1000 ms. */
static void add_n(size_t n)
{
    for (size_t k = 0; k < n; k++) {
        struct history_entry e = { .t_ms = (uint32_t)(k * 1000), .centi = (int32_t)(1000 + k),
                                   .kind = HIST_OK };
        history_add(&h, e);
    }
}

static void test_empty(void)
{
    TEST_ASSERT_EQUAL_size_t(0, history_count(&h));
}

static void test_partial_keeps_order(void)
{
    add_n(3);
    TEST_ASSERT_EQUAL_size_t(3, history_count(&h));
    TEST_ASSERT_EQUAL_INT32(1000, history_get(&h, 0).centi);   /* vanhin */
    TEST_ASSERT_EQUAL_INT32(1002, history_get(&h, 2).centi);   /* uusin */
}

static void test_exactly_full(void)
{
    add_n(HISTORY_LEN);
    TEST_ASSERT_EQUAL_size_t(HISTORY_LEN, history_count(&h));
    TEST_ASSERT_EQUAL_INT32(1000, history_get(&h, 0).centi);
    TEST_ASSERT_EQUAL_INT32(1000 + HISTORY_LEN - 1, history_get(&h, HISTORY_LEN - 1).centi);
}

static void test_one_over_drops_oldest(void)
{
    add_n(HISTORY_LEN + 1);
    TEST_ASSERT_EQUAL_size_t(HISTORY_LEN, history_count(&h));
    TEST_ASSERT_EQUAL_INT32(1001, history_get(&h, 0).centi);   /* 1000 putosi pois */
    TEST_ASSERT_EQUAL_INT32(1000 + HISTORY_LEN, history_get(&h, HISTORY_LEN - 1).centi);
}

static void test_many_rounds(void)
{
    add_n(100);
    TEST_ASSERT_EQUAL_size_t(HISTORY_LEN, history_count(&h));
    TEST_ASSERT_EQUAL_INT32(1000 + 100 - HISTORY_LEN, history_get(&h, 0).centi);
    TEST_ASSERT_EQUAL_INT32(1099, history_get(&h, HISTORY_LEN - 1).centi);
    /* Järjestys säilyy koko matkalta: jokainen rivi on edellistä yhtä suurempi. */
    for (size_t i = 1; i < HISTORY_LEN; i++) {
        TEST_ASSERT_EQUAL_INT32(history_get(&h, i - 1).centi + 1, history_get(&h, i).centi);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty);
    RUN_TEST(test_partial_keeps_order);
    RUN_TEST(test_exactly_full);
    RUN_TEST(test_one_over_drops_oldest);
    RUN_TEST(test_many_rounds);
    return UNITY_END();
}
