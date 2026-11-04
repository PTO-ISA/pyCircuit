#pragma once
#include <gfsim/wire.h>
#include <gfsim/dff.h>
#include <gfsim/sampled_clock.h>
#include <gfsim/sync_mem.h>
#include <gfsim/byte_mem.h>
#include <gfsim/fifo.h>
#include <gfsim/SimModule.h>
#include <gfsim/SimSystem.h>
#include <gfsim/collection.h>
#include <gfsim/WorkExecutor.h>
#include <memory>
#include <string>
#include <string_view>
#include <array>
#include <algorithm>
#include <bit>
#include <type_traits>
#include <utility>
#include <cstddef>
#include <cstdint>
constexpr std::int64_t pyc_floordiv(std::int64_t a, std::int64_t b) { return a/b - ((a%b != 0 && (a<0)!=(b<0)) ? 1 : 0); }
constexpr std::int64_t pyc_mod(std::int64_t a, std::int64_t b) { auto r=a%b; return r != 0 && (r<0)!=(b<0) ? r+b : r; }
template <std::size_t N> consteval std::uint64_t pyc_mod_words(
  std::array<std::uint64_t,N> words, bool negative, std::uint64_t modulus) {
  std::uint64_t remainder=0;
  for (std::size_t i=N; i-- > 0;) for (unsigned bit=64; bit-- > 0;)
    remainder=(remainder*2+((words[i]>>bit)&1))%modulus;
  return negative && remainder ? modulus-remainder : remainder;
}
template <unsigned W> gfsim::Bits<W> pyc_bits(std::int64_t value) { return gfsim::Bits<W>{static_cast<std::uint64_t>(value)}; }

namespace rv32i_core { namespace types { struct Fetch {
  gfsim::Bits<1> valid{};
  gfsim::Bits<32> pc{};
  gfsim::Bits<32> instruction{};
  gfsim::Bits<1> fault{};
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::types::Fetch> : gfsim::hardware_struct_traits<::rv32i_core::types::Fetch, &::rv32i_core::types::Fetch::valid, &::rv32i_core::types::Fetch::pc, &::rv32i_core::types::Fetch::instruction, &::rv32i_core::types::Fetch::fault> {};
namespace rv32i_core { namespace types { struct Decoded {
  gfsim::Bits<5> rs1{};
  gfsim::Bits<5> rs2{};
  gfsim::Bits<5> rd{};
  gfsim::Bits<1> use1{};
  gfsim::Bits<1> use2{};
  gfsim::Bits<1> writes{};
  gfsim::Bits<4> operation{};
  gfsim::Bits<32> immediate{};
  gfsim::Bits<1> immediate_rhs{};
  gfsim::Bits<1> load{};
  gfsim::Bits<1> store{};
  gfsim::Bits<2> size{};
  gfsim::Bits<1> signed_load{};
  gfsim::Bits<1> branch{};
  gfsim::Bits<3> condition{};
  gfsim::Bits<2> jump{};
  gfsim::Bits<1> trap{};
  gfsim::Bits<4> cause{};
  gfsim::Bits<32> tval{};
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::types::Decoded> : gfsim::hardware_struct_traits<::rv32i_core::types::Decoded, &::rv32i_core::types::Decoded::rs1, &::rv32i_core::types::Decoded::rs2, &::rv32i_core::types::Decoded::rd, &::rv32i_core::types::Decoded::use1, &::rv32i_core::types::Decoded::use2, &::rv32i_core::types::Decoded::writes, &::rv32i_core::types::Decoded::operation, &::rv32i_core::types::Decoded::immediate, &::rv32i_core::types::Decoded::immediate_rhs, &::rv32i_core::types::Decoded::load, &::rv32i_core::types::Decoded::store, &::rv32i_core::types::Decoded::size, &::rv32i_core::types::Decoded::signed_load, &::rv32i_core::types::Decoded::branch, &::rv32i_core::types::Decoded::condition, &::rv32i_core::types::Decoded::jump, &::rv32i_core::types::Decoded::trap, &::rv32i_core::types::Decoded::cause, &::rv32i_core::types::Decoded::tval> {};
namespace rv32i_core { namespace types { struct Execute {
  gfsim::Bits<1> valid{};
  gfsim::Bits<32> pc{};
  gfsim::Bits<32> instruction{};
  ::rv32i_core::types::Decoded control;
  gfsim::Bits<32> first{};
  gfsim::Bits<32> second{};
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::types::Execute> : gfsim::hardware_struct_traits<::rv32i_core::types::Execute, &::rv32i_core::types::Execute::valid, &::rv32i_core::types::Execute::pc, &::rv32i_core::types::Execute::instruction, &::rv32i_core::types::Execute::control, &::rv32i_core::types::Execute::first, &::rv32i_core::types::Execute::second> {};
namespace rv32i_core { namespace types { struct Packet {
  gfsim::Bits<1> valid{};
  gfsim::Bits<32> pc{};
  gfsim::Bits<32> instruction{};
  gfsim::Bits<32> next_pc{};
  gfsim::Bits<5> rd{};
  gfsim::Bits<1> writes{};
  gfsim::Bits<32> result{};
  gfsim::Bits<1> load{};
  gfsim::Bits<1> store{};
  gfsim::Bits<32> address{};
  gfsim::Bits<32> store_data{};
  gfsim::Bits<32> load_data{};
  gfsim::Bits<2> size{};
  gfsim::Bits<1> signed_load{};
  gfsim::Bits<1> done{};
  gfsim::Bits<1> redirect{};
  gfsim::Bits<1> trap{};
  gfsim::Bits<4> cause{};
  gfsim::Bits<32> tval{};
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::types::Packet> : gfsim::hardware_struct_traits<::rv32i_core::types::Packet, &::rv32i_core::types::Packet::valid, &::rv32i_core::types::Packet::pc, &::rv32i_core::types::Packet::instruction, &::rv32i_core::types::Packet::next_pc, &::rv32i_core::types::Packet::rd, &::rv32i_core::types::Packet::writes, &::rv32i_core::types::Packet::result, &::rv32i_core::types::Packet::load, &::rv32i_core::types::Packet::store, &::rv32i_core::types::Packet::address, &::rv32i_core::types::Packet::store_data, &::rv32i_core::types::Packet::load_data, &::rv32i_core::types::Packet::size, &::rv32i_core::types::Packet::signed_load, &::rv32i_core::types::Packet::done, &::rv32i_core::types::Packet::redirect, &::rv32i_core::types::Packet::trap, &::rv32i_core::types::Packet::cause, &::rv32i_core::types::Packet::tval> {};
namespace rv32i_core { namespace types { struct Machine {
  gfsim::table<gfsim::Bits<32>, 32> registers{};
  gfsim::Bits<32> fetch_pc{};
  gfsim::Bits<32> arch_pc{};
  gfsim::Bits<1> fetch_busy{};
  gfsim::Bits<32> fetch_request_pc{};
  gfsim::Bits<1> fetch_killed{};
  ::rv32i_core::types::Fetch fetch_buffer;
  ::rv32i_core::types::Fetch decode_stage;
  ::rv32i_core::types::Execute execute_stage;
  ::rv32i_core::types::Packet memory_stage;
  ::rv32i_core::types::Packet writeback_stage;
  gfsim::Bits<1> stopping{};
  gfsim::Bits<1> halted{};
  gfsim::Bits<64> order{};
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::types::Machine> : gfsim::hardware_struct_traits<::rv32i_core::types::Machine, &::rv32i_core::types::Machine::registers, &::rv32i_core::types::Machine::fetch_pc, &::rv32i_core::types::Machine::arch_pc, &::rv32i_core::types::Machine::fetch_busy, &::rv32i_core::types::Machine::fetch_request_pc, &::rv32i_core::types::Machine::fetch_killed, &::rv32i_core::types::Machine::fetch_buffer, &::rv32i_core::types::Machine::decode_stage, &::rv32i_core::types::Machine::execute_stage, &::rv32i_core::types::Machine::memory_stage, &::rv32i_core::types::Machine::writeback_stage, &::rv32i_core::types::Machine::stopping, &::rv32i_core::types::Machine::halted, &::rv32i_core::types::Machine::order> {};
namespace rv32i_core { namespace types { struct Pins {
  gfsim::Bits<1> imem_valid{};
  gfsim::Bits<32> imem_address{};
  gfsim::Bits<1> dmem_valid{};
  gfsim::Bits<1> dmem_write{};
  gfsim::Bits<32> dmem_address{};
  gfsim::Bits<32> dmem_data{};
  gfsim::Bits<4> dmem_mask{};
  gfsim::Bits<1> retire_valid{};
  gfsim::Bits<64> retire_order{};
  gfsim::Bits<32> retire_pc{};
  gfsim::Bits<32> retire_instruction{};
  gfsim::Bits<32> retire_next_pc{};
  gfsim::Bits<1> retire_rd_write{};
  gfsim::Bits<5> retire_rd{};
  gfsim::Bits<32> retire_value{};
  gfsim::Bits<1> retire_load{};
  gfsim::Bits<1> retire_store{};
  gfsim::Bits<32> retire_address{};
  gfsim::Bits<4> retire_mask{};
  gfsim::Bits<32> retire_mem_data{};
  gfsim::Bits<1> retire_trap{};
  gfsim::Bits<4> retire_cause{};
  gfsim::Bits<32> retire_tval{};
  gfsim::Bits<1> stage_if{};
  gfsim::Bits<1> stage_id{};
  gfsim::Bits<1> stage_ex{};
  gfsim::Bits<1> stage_mem{};
  gfsim::Bits<1> stage_wb{};
  gfsim::Bits<1> load_use{};
  gfsim::Bits<1> memory_wait{};
  gfsim::Bits<1> redirected{};
  gfsim::Bits<1> halted{};
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::types::Pins> : gfsim::hardware_struct_traits<::rv32i_core::types::Pins, &::rv32i_core::types::Pins::imem_valid, &::rv32i_core::types::Pins::imem_address, &::rv32i_core::types::Pins::dmem_valid, &::rv32i_core::types::Pins::dmem_write, &::rv32i_core::types::Pins::dmem_address, &::rv32i_core::types::Pins::dmem_data, &::rv32i_core::types::Pins::dmem_mask, &::rv32i_core::types::Pins::retire_valid, &::rv32i_core::types::Pins::retire_order, &::rv32i_core::types::Pins::retire_pc, &::rv32i_core::types::Pins::retire_instruction, &::rv32i_core::types::Pins::retire_next_pc, &::rv32i_core::types::Pins::retire_rd_write, &::rv32i_core::types::Pins::retire_rd, &::rv32i_core::types::Pins::retire_value, &::rv32i_core::types::Pins::retire_load, &::rv32i_core::types::Pins::retire_store, &::rv32i_core::types::Pins::retire_address, &::rv32i_core::types::Pins::retire_mask, &::rv32i_core::types::Pins::retire_mem_data, &::rv32i_core::types::Pins::retire_trap, &::rv32i_core::types::Pins::retire_cause, &::rv32i_core::types::Pins::retire_tval, &::rv32i_core::types::Pins::stage_if, &::rv32i_core::types::Pins::stage_id, &::rv32i_core::types::Pins::stage_ex, &::rv32i_core::types::Pins::stage_mem, &::rv32i_core::types::Pins::stage_wb, &::rv32i_core::types::Pins::load_use, &::rv32i_core::types::Pins::memory_wait, &::rv32i_core::types::Pins::redirected, &::rv32i_core::types::Pins::halted> {};
namespace rv32i_core { namespace types { struct ExecuteControl {
  gfsim::Bits<1> moved{};
  gfsim::Bits<1> kill{};
  gfsim::Bits<1> redirected{};
  gfsim::Bits<1> memory_fault{};
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::types::ExecuteControl> : gfsim::hardware_struct_traits<::rv32i_core::types::ExecuteControl, &::rv32i_core::types::ExecuteControl::moved, &::rv32i_core::types::ExecuteControl::kill, &::rv32i_core::types::ExecuteControl::redirected, &::rv32i_core::types::ExecuteControl::memory_fault> {};
namespace rv32i_core { namespace execute { struct ResultExecuteLogic {
  ::rv32i_core::types::Machine state;
  ::rv32i_core::types::Pins io;
  ::rv32i_core::types::ExecuteControl control;
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::execute::ResultExecuteLogic> : gfsim::hardware_struct_traits<::rv32i_core::execute::ResultExecuteLogic, &::rv32i_core::execute::ResultExecuteLogic::state, &::rv32i_core::execute::ResultExecuteLogic::io, &::rv32i_core::execute::ResultExecuteLogic::control> {};
namespace rv32i_core { namespace frontend { struct ResultFrontend {
  ::rv32i_core::types::Machine state;
  ::rv32i_core::types::Pins io;
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::frontend::ResultFrontend> : gfsim::hardware_struct_traits<::rv32i_core::frontend::ResultFrontend, &::rv32i_core::frontend::ResultFrontend::state, &::rv32i_core::frontend::ResultFrontend::io> {};
namespace rv32i_core { namespace types { struct MemoryControl {
  gfsim::Bits<1> fault{};
  gfsim::Bits<1> space{};
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::types::MemoryControl> : gfsim::hardware_struct_traits<::rv32i_core::types::MemoryControl, &::rv32i_core::types::MemoryControl::fault, &::rv32i_core::types::MemoryControl::space> {};
namespace rv32i_core { namespace memory { struct ResultMemory {
  ::rv32i_core::types::Machine state;
  ::rv32i_core::types::Pins io;
  ::rv32i_core::types::Packet memory;
  ::rv32i_core::types::MemoryControl control;
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::memory::ResultMemory> : gfsim::hardware_struct_traits<::rv32i_core::memory::ResultMemory, &::rv32i_core::memory::ResultMemory::state, &::rv32i_core::memory::ResultMemory::io, &::rv32i_core::memory::ResultMemory::memory, &::rv32i_core::memory::ResultMemory::control> {};
namespace rv32i_core { namespace types { struct WritebackControl {
  gfsim::Bits<1> store{};
  gfsim::Bits<1> fault{};
  gfsim::Bits<1> done{};
  gfsim::Bits<1> space{};
  gfsim::Bits<1> terminal{};
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::types::WritebackControl> : gfsim::hardware_struct_traits<::rv32i_core::types::WritebackControl, &::rv32i_core::types::WritebackControl::store, &::rv32i_core::types::WritebackControl::fault, &::rv32i_core::types::WritebackControl::done, &::rv32i_core::types::WritebackControl::space, &::rv32i_core::types::WritebackControl::terminal> {};
namespace rv32i_core { namespace types { struct BusInputs {
  gfsim::Bits<1> imem_ready{};
  gfsim::Bits<32> imem_data{};
  gfsim::Bits<1> imem_fault{};
  gfsim::Bits<1> dmem_ready{};
  gfsim::Bits<32> dmem_data{};
  gfsim::Bits<1> dmem_fault{};
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::types::BusInputs> : gfsim::hardware_struct_traits<::rv32i_core::types::BusInputs, &::rv32i_core::types::BusInputs::imem_ready, &::rv32i_core::types::BusInputs::imem_data, &::rv32i_core::types::BusInputs::imem_fault, &::rv32i_core::types::BusInputs::dmem_ready, &::rv32i_core::types::BusInputs::dmem_data, &::rv32i_core::types::BusInputs::dmem_fault> {};
namespace rv32i_core { namespace writeback { struct ResultWriteback {
  ::rv32i_core::types::Machine state;
  ::rv32i_core::types::Pins io;
  ::rv32i_core::types::WritebackControl control;
};
}
}
template <> struct gfsim::hardware_traits<::rv32i_core::writeback::ResultWriteback> : gfsim::hardware_struct_traits<::rv32i_core::writeback::ResultWriteback, &::rv32i_core::writeback::ResultWriteback::state, &::rv32i_core::writeback::ResultWriteback::io, &::rv32i_core::writeback::ResultWriteback::control> {};
