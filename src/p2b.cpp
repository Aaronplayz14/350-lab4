#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#include "p2a.h"
#include "timer.h"

constexpr std::size_t X = 256;
constexpr std::size_t Y = 256;
constexpr std::size_t Z = 256;

constexpr std::size_t KX = 4;
constexpr std::size_t KY = 4;
constexpr std::size_t KZ = 4;
constexpr std::size_t Stride = 4;

constexpr std::size_t outX = (X - KX) / Stride + 1;
constexpr std::size_t outY = (Y - KY) / Stride + 1;
constexpr std::size_t outZ = (Z - KZ) / Stride + 1;

constexpr std::size_t Input = X * Y * Z;
constexpr std::size_t Kernel = KX * KY * KZ;
constexpr std::size_t Output = outX * outY * outZ;

// Row-major index helpers. The three arrays involved have different shapes
// (input, kernel, output), so each needs its own formula.
std::size_t rowMajorIndexIn(std::size_t x, std::size_t y, std::size_t z)
{
    return x + X * (y + Y * z);
}

std::size_t rowMajorIndexK(std::size_t x, std::size_t y, std::size_t z)
{
    return x + KX * (y + KY * z);
}

std::size_t rowMajorIndexOut(std::size_t x, std::size_t y, std::size_t z)
{
    return x + outX * (y + outY * z);
}

int main()
{

    std::vector<uint64_t> A(Input);
    std::vector<uint64_t> B(Input);

    std::mt19937_64 rng(0);

    // Initialize A in memory order, exactly as it is laid out.
    for (std::size_t i = 0; i < Input; ++i)
    {
        A[i] = rng();
    }

    // Precompute the Morton code of each position inside a 4 x 4 x 4 block.
    // A block's coordinates fit in two bits each, so all these values are < 64.
    std::array<std::size_t, Kernel> localMorton{};
    for (std::size_t z = 0; z < KZ; ++z)
    {
        for (std::size_t y = 0; y < KY; ++y)
        {
            for (std::size_t x = 0; x < KX; ++x)
            {
                localMorton[rowMajorIndexK(x, y, z)] =
                    static_cast<std::size_t>(morton3d(x, y, z));
            }
        }
    }

    for (std::size_t bz = 0; bz < outZ; ++bz)
    {
        for (std::size_t by = 0; by < outY; ++by)
        {
            for (std::size_t bx = 0; bx < outX; ++bx)
            {
                // Block (bx, by, bz) starts at morton3d(bx, by, bz) * Kernel.
                // Every 4 x 4 x 4 block occupies one contiguous 64-entry chunk,
                // so together these writes cover B exactly once, in Morton
                // order: B[morton3d(x, y, z)] = A[rowMajorIndexIn(x, y, z)].
                const std::size_t block = static_cast<std::size_t>(morton3d(bx, by, bz));
                const std::size_t blockStart = block * Kernel;

                for (std::size_t z = 0; z < KZ; ++z)
                {
                    for (std::size_t y = 0; y < KY; ++y)
                    {
                        for (std::size_t x = 0; x < KX; ++x)
                        {
                            const std::size_t local = rowMajorIndexK(x, y, z);
                            const std::size_t globalX = bx * Stride + x;
                            const std::size_t globalY = by * Stride + y;
                            const std::size_t globalZ = bz * Stride + z;

                            B[blockStart + localMorton[local]] = A[rowMajorIndexIn(globalX, globalY, globalZ)];
                        }
                    }
                }
            }
        }
    }

    // Kernel values K(x, y, z) = x + y + z.
    std::array<uint64_t, Kernel> Ka{};

    for (std::size_t z = 0; z < KZ; ++z)
    {
        for (std::size_t y = 0; y < KY; ++y)
        {
            for (std::size_t x = 0; x < KX; ++x)
            {
                Ka[rowMajorIndexK(x, y, z)] = x + y + z;
            }
        }
    }

    // Same kernel, but stored in Morton order so it matches B's layout.
    std::array<uint64_t, Kernel> Kb{};

    for (std::size_t z = 0; z < KZ; ++z)
    {
        for (std::size_t y = 0; y < KY; ++y)
        {
            for (std::size_t x = 0; x < KX; ++x)
            {
                Kb[morton3d(x, y, z)] = x + y + z;
            }
        }
    }

    std::vector<uint64_t> rowMajorOutput(Output);
    std::vector<uint64_t> mortonOutput(Output);

    Timer timer;

    // Row-major convolution: a six-level nested loop, as the hint suggests.
    timer.restart();

    for (std::size_t oz = 0; oz < outZ; ++oz)
    {
        for (std::size_t oy = 0; oy < outY; ++oy)
        {
            for (std::size_t ox = 0; ox < outX; ++ox)
            {
                uint64_t sum = 0;

                for (std::size_t kz = 0; kz < KZ; ++kz)
                {
                    for (std::size_t ky = 0; ky < KY; ++ky)
                    {
                        for (std::size_t kx = 0; kx < KX; ++kx)
                        {
                            const std::size_t x = ox * Stride + kx;
                            const std::size_t y = oy * Stride + ky;
                            const std::size_t z = oz * Stride + kz;

                            sum += A[rowMajorIndexIn(x, y, z)] *
                                   Ka[rowMajorIndexK(kx, ky, kz)];
                        }
                    }
                }

                rowMajorOutput[rowMajorIndexOut(ox, oy, oz)] = sum;
            }
        }
    }

    const uint64_t rowMajorTime = timer.click<Timer::Micros>();

    // Morton convolution: only a double-nested loop. Each 4 x 4 x 4 block of B
    // is contiguous (see the fill above) and Kb uses the same Morton order, so
    // the dot product is a straight sequential pass. Output block i corresponds
    // to morton3d(bx, by, bz) == i.
    timer.restart();

    for (std::size_t block = 0; block < Output; ++block)
    {
        uint64_t sum = 0;
        const std::size_t inputBlockStart = block * Kernel;

        for (std::size_t k = 0; k < Kernel; ++k)
        {
            sum += B[inputBlockStart + k] * Kb[k];
        }

        mortonOutput[block] = sum;
    }

    const uint64_t mortonTime = timer.click<Timer::Micros>();

    // The two layouts must produce identical results; verify every pair of
    // output entries. (Assertions only run in debug builds; NDEBUG disables
    // them in release.)
    for (std::size_t z = 0; z < outZ; ++z)
    {
        for (std::size_t y = 0; y < outY; ++y)
        {
            for (std::size_t x = 0; x < outX; ++x)
            {
                assert(rowMajorOutput[rowMajorIndexOut(x, y, z)] == mortonOutput[morton3d(x, y, z)]);
            }
        }
    }

    std::cout << rowMajorTime << '\n';
    std::cout << mortonTime << '\n';

    // In release builds the assertions above are compiled out, which would let
    // the compiler discard the (otherwise unused) convolution results and make
    // the measured times meaningless. Fold both outputs into a checksum stored
    // to a volatile so the timed work remains observable, without printing it.
    uint64_t checksum = 0;
    for (std::size_t i = 0; i < Output; ++i)
    {
        checksum += rowMajorOutput[i] + mortonOutput[i];
    }
    volatile uint64_t sink = checksum;
    (void)sink;

    return 0;
}