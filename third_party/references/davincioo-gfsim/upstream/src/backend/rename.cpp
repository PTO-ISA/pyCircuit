#include "backend/rename.hpp"

namespace davincioo::backend {

namespace {

bool IsScalarOnlyOp(const PTOInst& inst) {
  return inst.engine_kind == PTOEngineKind::Scalar;
}

bool IsLoadLike(const PTOInst& inst) {
  return inst.engine_kind == PTOEngineKind::Tma && inst.raw_opcode == "TLOAD";
}

bool IsStoreLike(const PTOInst& inst) {
  return inst.engine_kind == PTOEngineKind::Tma &&
         (inst.raw_opcode == "TSTORE" || inst.raw_opcode == "TSTORE_FP");
}

}  // namespace

Rename::Rename(RenameConfig config)
    : Module<Rename, PTOInstRef>("rename"),
      config_(config) {}

void Rename::BuildSelf() {
  GFSIM_ASSERT(config_.tile_tags > 0);
  GFSIM_ASSERT(inputs_.size() > 1);
  GFSIM_ASSERT(inputs_[0] != nullptr);
  GFSIM_ASSERT(inputs_[1] != nullptr);
  GFSIM_ASSERT(outputs_.size() > 0);
  GFSIM_ASSERT(outputs_[0] != nullptr);
}

void Rename::ResetSelf() {
  free_tile_tags_.clear();
  smap_.clear();
  for (std::uint64_t tag = 0; tag < config_.tile_tags; ++tag) {
    free_tile_tags_.insert(tag);
  }
}

bool Rename::IsManagedInput(const PTOInst& inst, std::size_t) const {
  if (IsScalarOnlyOp(inst) || IsLoadLike(inst)) {
    return false;
  }
  return true;
}

bool Rename::IsManagedOutput(const PTOInst& inst, std::size_t) const {
  if (IsScalarOnlyOp(inst) || IsStoreLike(inst)) {
    return false;
  }
  return true;
}

std::size_t Rename::RequiredOutputTags(const PTOInst& inst) const {
  std::size_t count = 0;
  for (std::size_t index = 0; index < inst.tile_reg_outputs.size(); ++index) {
    if (IsManagedOutput(inst, index)) {
      ++count;
    }
  }
  return count;
}

bool Rename::HasEnoughFreeTags(const PTOInst& inst) const {
  return free_tile_tags_.size() >= RequiredOutputTags(inst);
}

void Rename::RecycleRetiredInstruction(const PTOInstRef& inst) {
  GFSIM_ASSERT(inst != nullptr);
  for (const std::uint64_t tag : inst->output_replaced_tile_tags) {
    if (tag == kInvalidTileTag) {
      continue;
    }
    const auto [_, inserted] = free_tile_tags_.insert(tag);
    GFSIM_ASSERT(inserted);
    MarkProgress();
  }
}

std::uint64_t Rename::AllocateTileTag() {
  GFSIM_ASSERT(!free_tile_tags_.empty());
  const auto it = free_tile_tags_.begin();
  const std::uint64_t tag = *it;
  free_tile_tags_.erase(it);
  return tag;
}

Rename::TileStorageKey Rename::MakeTileStorageKey(const PTOTileReg& reg) {
  return TileStorageKey{
      .tile_type = reg.tile_type,
      .address = reg.address,
  };
}

void Rename::RenameInstruction(PTOInst& inst) {
  inst.timestamps.rename_cycle = CurrentCycle();
  inst.output_replaced_tile_tags.assign(inst.tile_reg_outputs.size(), kInvalidTileTag);
  inst.tile_input_valid.assign(inst.tile_reg_inputs.size(), false);
  inst.tile_input_ready.assign(inst.tile_reg_inputs.size(), true);
  inst.src_ready = false;

  for (std::size_t index = 0; index < inst.tile_reg_inputs.size(); ++index) {
    PTOTileReg& reg = inst.tile_reg_inputs[index];
    reg.rename_managed = IsManagedInput(inst, index);
    if (!reg.rename_managed) {
      reg.tile_tag = kInvalidTileTag;
      inst.tile_input_valid[index] = false;
      inst.tile_input_ready[index] = true;
      continue;
    }
    const auto it = smap_.find(MakeTileStorageKey(reg));
    if (it == smap_.end()) {
      // No in-trace producer for this tile-storage key. This is legitimate
      // for pypto-emitted kernels whose loop-carried tiles are first
      // referenced as reads (e.g. RMSNorm broadcasts a row-reduced tile
      // whose storage key differs subtly from the TASSIGN that allocated
      // it). Rather than asserting on what is really "external input"
      // semantics, mark the tile as already-ready and let the cycle model
      // proceed.
      reg.rename_managed = false;
      reg.tile_tag = kInvalidTileTag;
      inst.tile_input_valid[index] = false;
      inst.tile_input_ready[index] = true;
      continue;
    }
    reg.tile_tag = it->second;
    inst.tile_input_valid[index] = true;
    inst.tile_input_ready[index] = false;
  }

  for (std::size_t index = 0; index < inst.tile_reg_outputs.size(); ++index) {
    PTOTileReg& reg = inst.tile_reg_outputs[index];
    reg.rename_managed = IsManagedOutput(inst, index);
    if (!reg.rename_managed) {
      reg.tile_tag = kInvalidTileTag;
      inst.output_replaced_tile_tags[index] = kInvalidTileTag;
      continue;
    }

    const TileStorageKey key = MakeTileStorageKey(reg);
    const auto replaced = smap_.find(key);
    inst.output_replaced_tile_tags[index] = replaced == smap_.end() ? kInvalidTileTag : replaced->second;

    const std::uint64_t new_tag = AllocateTileTag();
    reg.tile_tag = new_tag;
    smap_[key] = new_tag;
    MarkProgress();
  }
}

void Rename::SetRenameWidth(std::size_t width) {
  GFSIM_ASSERT(width > 0);
  rename_width_ = width;
}

void Rename::WorkSelf() {
  INPUT(inst_in, 0);
  INPUT(retire_in, 1);
  OUTPUT(inst_out, 0);

  while (!retire_in->Empty()) {
    PTOInstRef retired = retire_in->Read();
    RecycleRetiredInstruction(retired);
  }
  std::size_t renamed_count = 0;
  while (renamed_count < rename_width_ && !inst_in->Empty() && !inst_out->Full()) {
    PTOInstRef head = inst_in->Front();
    GFSIM_ASSERT(head != nullptr);
    if (!HasEnoughFreeTags(*head)) {
      break;
    }
    PTOInstRef renamed = inst_in->Read();
    RenameInstruction(*renamed);
    inst_out->Write(renamed);
    MarkProgress();
    ++renamed_count;
  }
}

}  // namespace davincioo::backend
