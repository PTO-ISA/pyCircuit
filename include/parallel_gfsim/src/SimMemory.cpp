#include "parallel_gfsim/SimMemory.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>

namespace parallel_gfsim {

namespace {

bool ValidWidth(unsigned width)
{
    return width == 1 || width == 2 || width == 4 || width == 8;
}

std::uint64_t ReadUnsigned(const std::vector<std::uint8_t>& image,
                           std::uint64_t offset, unsigned width)
{
    if (offset > image.size() || width > image.size() - offset) {
        throw std::invalid_argument("truncated ELF field");
    }
    std::uint64_t value = 0;
    for (unsigned index = 0; index < width; ++index) {
        value |= static_cast<std::uint64_t>(image[static_cast<std::size_t>(offset + index)])
                 << (8 * index);
    }
    return value;
}

std::vector<std::uint8_t> ReadFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("cannot open ELF file: " + path);
    }
    const std::streamoff length = file.tellg();
    if (length < 0 || static_cast<std::uint64_t>(length) >
                          std::numeric_limits<std::size_t>::max()) {
        throw std::runtime_error("ELF file is too large");
    }
    std::vector<std::uint8_t> image(static_cast<std::size_t>(length));
    file.seekg(0);
    if (!image.empty()) {
        file.read(reinterpret_cast<char*>(image.data()), length);
        if (!file) {
            throw std::runtime_error("cannot read complete ELF file: " + path);
        }
    }
    return image;
}

} // namespace

void SimMemory::Map(std::uint64_t address, std::uint64_t size)
{
    if (size == 0) {
        return;
    }
    if (size > std::numeric_limits<std::uint64_t>::max() - address) {
        throw std::invalid_argument("mapped range overflows address space");
    }
    std::uint64_t end = address + size;
    auto it = regions_.lower_bound(address);
    if (it != regions_.begin()) {
        auto prior = std::prev(it);
        if (prior->second >= address) {
            it = prior;
        }
    }
    while (it != regions_.end() && it->first <= end) {
        address = std::min(address, it->first);
        end = std::max(end, it->second);
        it = regions_.erase(it);
    }
    regions_.emplace(address, end);
}

bool SimMemory::IsMappedRange(std::uint64_t address, std::uint64_t size) const
{
    if (size == 0) {
        return true;
    }
    if (size > std::numeric_limits<std::uint64_t>::max() - address) {
        return false;
    }
    auto it = regions_.upper_bound(address);
    if (it == regions_.begin()) {
        return false;
    }
    --it;
    return it->second >= address + size;
}

std::uint8_t SimMemory::ReadByte(std::uint64_t address) const
{
    const auto found = pages_.find(address / kPageSize);
    return found == pages_.end() ? 0 : (*found->second)[address % kPageSize];
}

void SimMemory::WriteByte(std::uint64_t address, std::uint8_t value)
{
    const std::uint64_t pageIndex = address / kPageSize;
    auto found = pages_.find(pageIndex);
    if (found == pages_.end()) {
        if (value == 0) {
            return;
        }
        found = pages_.emplace(pageIndex, std::unique_ptr<Page>(new Page{})).first;
    }
    (*found->second)[address % kPageSize] = value;
}

std::uint64_t SimMemory::Load(std::uint64_t address, unsigned width,
                              bool signedLoad) const
{
    if (!ValidWidth(width)) {
        throw std::invalid_argument("memory load width must be 1, 2, 4, or 8");
    }
    if (!IsMappedRange(address, width)) {
        throw std::out_of_range("memory load touches an unmapped address");
    }
    std::uint64_t value = 0;
    for (unsigned index = 0; index < width; ++index) {
        value |= static_cast<std::uint64_t>(ReadByte(address + index)) << (8 * index);
    }
    if (signedLoad && width < 8 && (value & (std::uint64_t(1) << (width * 8 - 1)))) {
        value |= std::numeric_limits<std::uint64_t>::max() << (width * 8);
    }
    return value;
}

bool SimMemory::TryLoad(std::uint64_t address, unsigned width, bool signedLoad,
                        std::uint64_t& value) const
{
    if (!ValidWidth(width) || !IsMappedRange(address, width)) {
        return false;
    }
    value = Load(address, width, signedLoad);
    return true;
}

void SimMemory::Store(std::uint64_t address, std::uint64_t value, unsigned width)
{
    if (!ValidWidth(width)) {
        throw std::invalid_argument("memory store width must be 1, 2, 4, or 8");
    }
    if (!IsMappedRange(address, width)) {
        throw std::out_of_range("memory store touches an unmapped address");
    }
    for (unsigned index = 0; index < width; ++index) {
        WriteByte(address + index, static_cast<std::uint8_t>(value >> (8 * index)));
    }
}

std::vector<std::uint8_t> SimMemory::ReadBytes(std::uint64_t address,
                                               std::size_t size) const
{
    if (!IsMappedRange(address, size)) {
        throw std::out_of_range("memory read touches an unmapped address");
    }
    std::vector<std::uint8_t> result(size);
    for (std::size_t index = 0; index < size; ++index) {
        result[index] = ReadByte(address + index);
    }
    return result;
}

void SimMemory::WriteBytes(std::uint64_t address,
                           const std::vector<std::uint8_t>& bytes)
{
    if (!IsMappedRange(address, bytes.size())) {
        throw std::out_of_range("memory write touches an unmapped address");
    }
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        WriteByte(address + index, bytes[index]);
    }
}

SimMemory::ElfImage SimMemory::LoadElf(const std::string& path)
{
    const std::vector<std::uint8_t> image = ReadFile(path);
    if (image.size() < 16 || image[0] != 0x7f || image[1] != 'E' ||
        image[2] != 'L' || image[3] != 'F' ||
        (image[4] != 1 && image[4] != 2) || image[5] != 1 || image[6] != 1) {
        throw std::invalid_argument("unsupported ELF identification");
    }

    const bool is64 = image[4] == 2;
    const std::uint64_t headerSize = is64 ? 64 : 52;
    const std::uint64_t entrySize = is64 ? 56 : 32;
    if (image.size() < headerSize ||
        ReadUnsigned(image, is64 ? 52 : 40, 2) < headerSize) {
        throw std::invalid_argument("truncated ELF header");
    }
    const std::uint64_t programOffset = ReadUnsigned(image, is64 ? 32 : 28,
                                                       is64 ? 8 : 4);
    const std::uint64_t programEntrySize = ReadUnsigned(image, is64 ? 54 : 42, 2);
    const std::uint64_t programCount = ReadUnsigned(image, is64 ? 56 : 44, 2);
    if (programCount == 0 || programEntrySize < entrySize ||
        programOffset > image.size() ||
        programCount > (image.size() - programOffset) / programEntrySize) {
        throw std::invalid_argument("invalid ELF program header table");
    }

    ElfImage result;
    result.entry = ReadUnsigned(image, 24, is64 ? 8 : 4);
    SimMemory loaded;
    for (std::uint64_t index = 0; index < programCount; ++index) {
        const std::uint64_t header = programOffset + index * programEntrySize;
        if (ReadUnsigned(image, header, 4) != 1) { // PT_LOAD
            continue;
        }
        const std::uint64_t fileOffset = ReadUnsigned(image, header + (is64 ? 8 : 4),
                                                      is64 ? 8 : 4);
        const std::uint64_t address = ReadUnsigned(image, header + (is64 ? 16 : 8),
                                                   is64 ? 8 : 4);
        const std::uint64_t fileSize = ReadUnsigned(image, header + (is64 ? 32 : 16),
                                                    is64 ? 8 : 4);
        const std::uint64_t memorySize = ReadUnsigned(image, header + (is64 ? 40 : 20),
                                                      is64 ? 8 : 4);
        if (memorySize < fileSize || fileOffset > image.size() ||
            fileSize > image.size() - fileOffset) {
            throw std::invalid_argument("invalid ELF load segment");
        }
        loaded.Map(address, memorySize);
        for (std::uint64_t byte = 0; byte < fileSize; ++byte) {
            loaded.WriteByte(address + byte,
                             image[static_cast<std::size_t>(fileOffset + byte)]);
        }
        ++result.loadSegments;
    }
    if (result.loadSegments == 0) {
        throw std::invalid_argument("ELF has no loadable segments");
    }
    regions_.swap(loaded.regions_);
    pages_.swap(loaded.pages_);
    return result;
}

void SimMemory::Reset()
{
    regions_.clear();
    pages_.clear();
}

} // namespace parallel_gfsim
