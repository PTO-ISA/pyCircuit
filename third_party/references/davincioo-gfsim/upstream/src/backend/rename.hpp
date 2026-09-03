#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <unordered_map>

#include "davincioo/model/config.hpp"
#include "davincioo/model/framework.hpp"
#include "davincioo/model/pto_inst.hpp"

namespace davincioo::backend {

class Rename : public Module<Rename, PTOInstRef> {
public:
  explicit Rename(RenameConfig config);

  void SetRenameWidth(std::size_t width);
  std::size_t RenameWidth() const noexcept { return rename_width_; }
  
protected:
  void BuildSelf() override;
  void ResetSelf() override;
  void WorkSelf() override;

private:
  struct TileStorageKey {
    PTOTileType tile_type = PTOTileType::Unknown;
    std::uint64_t address = 0;

    bool operator==(const TileStorageKey& other) const noexcept {
      return tile_type == other.tile_type && address == other.address;
    }
  };

  struct TileStorageKeyHash {
    std::size_t operator()(const TileStorageKey& key) const noexcept {
      return (static_cast<std::size_t>(key.address) << 4) ^ static_cast<std::size_t>(key.tile_type);
    }
  };

  bool IsManagedInput(const PTOInst& inst, std::size_t input_index) const;
  bool IsManagedOutput(const PTOInst& inst, std::size_t output_index) const;
  std::size_t RequiredOutputTags(const PTOInst& inst) const;
  bool HasEnoughFreeTags(const PTOInst& inst) const;
  void RecycleRetiredInstruction(const PTOInstRef& inst);
  void RenameInstruction(PTOInst& inst);
  std::uint64_t AllocateTileTag();
  static TileStorageKey MakeTileStorageKey(const PTOTileReg& reg);

  RenameConfig config_{};
  std::set<std::uint64_t> free_tile_tags_;
  std::unordered_map<TileStorageKey, std::uint64_t, TileStorageKeyHash> smap_;
  std::size_t rename_width_ = 1;
};

}  // namespace davincioo::backend
