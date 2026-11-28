#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace parallel_gfsim {

class SimMemory {
public:
    struct ElfImage {
        std::uint64_t entry = 0;
        std::size_t loadSegments = 0;
    };

    void Map(std::uint64_t address, std::uint64_t size);
    bool IsMappedRange(std::uint64_t address, std::uint64_t size) const;

    std::uint64_t Load(std::uint64_t address, unsigned width,
                       bool signedLoad = false) const;
    bool TryLoad(std::uint64_t address, unsigned width, bool signedLoad,
                 std::uint64_t& value) const;
    void Store(std::uint64_t address, std::uint64_t value, unsigned width);

    std::vector<std::uint8_t> ReadBytes(std::uint64_t address,
                                        std::size_t size) const;
    void WriteBytes(std::uint64_t address,
                    const std::vector<std::uint8_t>& bytes);

    ElfImage LoadElf(const std::string& path);
    void Reset();

private:
    std::uint8_t ReadByte(std::uint64_t address) const;
    void WriteByte(std::uint64_t address, std::uint8_t value);

    // Half-open mapped intervals [first, second), merged on overlap/touch.
    std::map<std::uint64_t, std::uint64_t> regions_;
    static const std::uint64_t kPageSize = 4096;
    using Page = std::array<std::uint8_t, kPageSize>;
    // Missing pages within a mapped interval read as zero.
    std::unordered_map<std::uint64_t, std::unique_ptr<Page>> pages_;
};

} // namespace parallel_gfsim
