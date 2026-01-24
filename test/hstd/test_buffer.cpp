#include <gtest/gtest.h>

import hstd;

TEST(WriteOnlyCircularBuffer, Head) {
  hstd::WriteOnlyCircularBuffer<int, 10> buf{};

  // When empty
  ASSERT_EQ(buf.head(), nullptr);

  // Insert first element
  buf.Push(1);
  ASSERT_NE(buf.head(), nullptr);
  ASSERT_EQ(*buf.head(), 1);

  // Insert more elements
  buf.Push(2);
  buf.Push(3);
  ASSERT_NE(buf.head(), nullptr);
  ASSERT_EQ(*buf.head(), 3);
}

TEST(WriteOnlyCircularBuffer, IterateEmpty) {
  hstd::WriteOnlyCircularBuffer<int, 10> buf{};

  int counter = 0;
  for (const auto v : buf) {
    ++counter;
  }

  ASSERT_EQ(counter, 0);
}

TEST(WriteOnlyCircularBuffer, IterateNotFull) {
  hstd::WriteOnlyCircularBuffer<int, 10> buf{};

  // Add 5 elements - 0, 1, 2, 3, 4
  for (std::size_t i = 0; i < 5; ++i) {
    buf.Push(i);
  }

  // Validate elements are iterated in insertion order
  int counter = 0;
  for (const auto v : buf) {
    ASSERT_EQ(v, counter);
    counter++;
  }

  // Validate we saw 5 elements
  ASSERT_EQ(counter, 5);
}

TEST(WriteOnlyCircularBuffer, IterateJustFull) {
  hstd::WriteOnlyCircularBuffer<int, 5> buf{};

  // Add 5 elements - 0, 1, 2, 3, 4
  for (std::size_t i = 0; i < 5; ++i) {
    buf.Push(i);
  }

  // Validate elements are iterated in insertion order
  int counter = 0;
  for (const auto v : buf) {
    ASSERT_EQ(v, counter);
    counter++;
  }

  // Validate we saw 5 elements
  ASSERT_EQ(counter, 5);
}

TEST(WriteOnlyCircularBuffer, IterateFull) {
  hstd::WriteOnlyCircularBuffer<int, 5> buf{};

  // Add 8 elements. Elements in the buffer will be 3, 4, 5, 6, 7
  for (std::size_t i = 0; i < 8; ++i) {
    buf.Push(i);
  }

  // Validate elements are iterated in insertion order
  int counter = 0;
  for (const auto v : buf) {
    ASSERT_EQ(v, counter + 3);
    counter++;
  }

  // Validate we saw 5 elements
  ASSERT_EQ(counter, 5);
}

TEST(WriteOnlyCircularBuffer, IterateMultipleTimesFull) {
  hstd::WriteOnlyCircularBuffer<int, 5> buf{};

  // Add 8 elements. Elements in the buffer will be 15, 16, 17, 18, 19
  for (std::size_t i = 0; i < 20; ++i) {
    buf.Push(i);
  }

  // Validate elements are iterated in insertion order
  int counter = 0;
  for (const auto v : buf) {
    ASSERT_EQ(v, counter + 15);
    counter++;
  }

  // Validate we saw 5 elements
  ASSERT_EQ(counter, 5);
}

TEST(WriteOnlyCircularBuffer, IndexingUninitializedBuffer) {
  hstd::WriteOnlyCircularBuffer<int, 5> buf{};

  // Add 8 elements. Elements in the buffer will be 0, 1, 2, *, *
  for (std::size_t i = 0; i < 3; ++i) {
    buf.Push(i);
  }

  ASSERT_EQ(buf[0], 0);
  ASSERT_EQ(buf[1], 1);
  ASSERT_EQ(buf[2], 2);
}

TEST(WriteOnlyCircularBuffer, IndexingInitializedBuffer) {
  hstd::WriteOnlyCircularBuffer<int, 5> buf{};

  // Add 8 elements. Elements in the buffer will be 3, 4, 5, 6, 7
  for (std::size_t i = 0; i < 8; ++i) {
    buf.Push(i);
  }

  ASSERT_EQ(buf[0], 3);
  ASSERT_EQ(buf[3], 6);
  ASSERT_EQ(buf[4], 7);
}