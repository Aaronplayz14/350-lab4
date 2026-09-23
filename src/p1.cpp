#include <cstddef>  // size_t
#include <cstdint>  // uint64_t
#include <iostream>
#include <memory>  // make_unique
#include <random>  // mt19937_64

#include "timer.h"

constexpr std::size_t kSide = 4000;
constexpr std::size_t kElements = kSide * kSide;

// Fill row-major: the outer loop walks rows, so consecutive rng() calls land
// at consecutive addresses (sequential memory writes).
void fillRowMajor(uint64_t* data, std::mt19937_64& rng)
{
    for (std::size_t row = 0; row < kSide; ++row)
    {
        for (std::size_t column = 0; column < kSide; ++column)
        {
            data[row * kSide + column] = rng();
        }
    }
}

// Fill column-major: the outer loop walks columns, so consecutive rng() calls
// land 4000 * 8 = 32 KiB apart (strided memory writes).
void fillColumnMajor(uint64_t* data, std::mt19937_64& rng)
{
    for (std::size_t column = 0; column < kSide; ++column)
    {
        for (std::size_t row = 0; row < kSide; ++row)
        {
            data[row * kSide + column] = rng();
        }
    }
}

// Sequential memory walk: visits data[0], data[1], ..., data[kElements - 1].
// The hardware prefetcher keeps the rows of the array in flight.
uint64_t sumRowMajor(const uint64_t* data)
{
    uint64_t total = 0;
    for (std::size_t row = 0; row < kSide; ++row)
    {
        for (std::size_t column = 0; column < kSide; ++column)
        {
            total += data[row * kSide + column];
        }
    }
    return total;
}

// Strided memory walk: each access is 32 KiB apart, missing the cache nearly
// every time. Because the fill was column-major too, this still adds the very
// same rng() numbers in the very same order as sumRowMajor.
uint64_t sumColumnMajor(const uint64_t* data)
{
    uint64_t total = 0;
    for (std::size_t column = 0; column < kSide; ++column)
    {
        for (std::size_t row = 0; row < kSide; ++row)
        {
            total += data[row * kSide + column];
        }
    }
    return total;
}

int main()
{
    // Both arrays live on the heap and are freed automatically at scope exit.
    const std::unique_ptr<uint64_t[]> rowArray = std::make_unique<uint64_t[]>(kElements);
    const std::unique_ptr<uint64_t[]> columnArray = std::make_unique<uint64_t[]>(kElements);

    // Same seed means both fills produce exactly the same sequence of values.
    std::mt19937_64 rng(0);
    fillRowMajor(rowArray.get(), rng);
    rng.seed(0);
    fillColumnMajor(columnArray.get(), rng);

    Timer timer;

    timer.restart();
    const uint64_t rowSum = sumRowMajor(rowArray.get());
    const uint64_t rowTimeUs = timer.click<Timer::Micros>();

    timer.restart();
    const uint64_t columnSum = sumColumnMajor(columnArray.get());
    const uint64_t columnTimeUs = timer.click<Timer::Micros>();

    // The sums must agree; if not, the fills did not produce the same data.
    if (rowSum != columnSum)
    {
        std::cerr << "Sums differ: row-major " << rowSum << " vs column-major " << columnSum << '\n';
        return 1;
    }

    std::cout << rowTimeUs << ' ' << rowSum << '\n';
    std::cout << columnTimeUs << ' ' << columnSum << '\n';
    return 0;
}