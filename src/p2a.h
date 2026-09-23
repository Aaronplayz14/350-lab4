#ifndef P2A_H
#define P2A_H

#include <cassert>
#include <cstdint>

// Takes the binary picture of input and spreads each set bit out so that bit i
// lands at bit i * scale. Anything pushed past bit 63 is truncated away.
// Same implementation as the prep's expand, marked inline so that including
// this header from several .cpp files does not violate the one-definition rule.
inline uint64_t expand(uint64_t input, uint32_t scale)
{
    assert(scale >= 1);

    uint64_t output = 0;
    for (uint32_t bit = 0; bit < 64; ++bit)
    {
        if ((input >> bit) & uint64_t{1})
        {
            const uint64_t outBit = static_cast<uint64_t>(bit) * scale;
            if (outBit < 64)
            {
                output |= uint64_t{1} << outBit;
            }
        }
    }
    return output;
}

// Scale-3 expansion: bit i of input becomes bit 3i.
inline uint64_t expand3(uint64_t input)
{
    return expand(input, 3);
}

// Morton code for d = 3, k = 64. The three expansions use bit offsets that
// never overlap (x: 0, 3, 6, ..., y: 1, 4, 7, ..., z: 2, 5, 8, ...), so the
// result is simply the bitwise-or of the three scaled coordinates.
inline uint64_t morton3d(uint64_t x, uint64_t y, uint64_t z)
{
    return expand3(x) | (expand3(y) << 1) | (expand3(z) << 2);
}

#endif  // P2A_H