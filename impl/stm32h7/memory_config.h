#pragma once

#include <cassert>
#include <span>

#include <stm32h7xx_hal.h>

#include <constexpr_tools/memory_size.h>
#include <constexpr_tools/static_mapping.h>

namespace stm32h7 {

using namespace ct::literals;

struct MemoryRegion {
  uint32_t    base_addr;
  std::size_t size;
};

#if defined(STM32H755)

inline constexpr MemoryRegion InstructionTightlyCoupledMemory = {0x0000'0000,
                                                                 64_KiB};
inline constexpr MemoryRegion DataTightlyCoupledMemory = {0x2000'0000, 128_KiB};

inline constexpr MemoryRegion Flash = {0x0800'0000, 1_MiB};

inline constexpr MemoryRegion AxiSram = {0x2400'0000, 512_KiB};
inline constexpr MemoryRegion Sram1   = {0x3000'0000, 128_KiB};
inline constexpr MemoryRegion Sram2   = {0x3002'0000, 128_KiB};
inline constexpr MemoryRegion Sram3   = {0x3004'0000, 32_KiB};
inline constexpr MemoryRegion Sram4   = {0x3800'0000, 64_KiB};

#endif

enum class MpuRegionSize : uint8_t {
  B32    = MPU_REGION_SIZE_32B,
  B64    = MPU_REGION_SIZE_64B,
  B128   = MPU_REGION_SIZE_128B,
  B256   = MPU_REGION_SIZE_256B,
  B512   = MPU_REGION_SIZE_512B,
  KiB1   = MPU_REGION_SIZE_1KB,
  KiB2   = MPU_REGION_SIZE_2KB,
  KiB4   = MPU_REGION_SIZE_4KB,
  KiB8   = MPU_REGION_SIZE_8KB,
  KiB16  = MPU_REGION_SIZE_16KB,
  KiB32  = MPU_REGION_SIZE_32KB,
  KiB64  = MPU_REGION_SIZE_64KB,
  KiB128 = MPU_REGION_SIZE_128KB,
  KiB256 = MPU_REGION_SIZE_256KB,
  KiB512 = MPU_REGION_SIZE_512KB,
  MiB1   = MPU_REGION_SIZE_1MB,
  MiB2   = MPU_REGION_SIZE_2MB,
  MiB4   = MPU_REGION_SIZE_4MB,
  MiB8   = MPU_REGION_SIZE_8MB,
  MiB16  = MPU_REGION_SIZE_16MB,
  MiB32  = MPU_REGION_SIZE_32MB,
  MiB64  = MPU_REGION_SIZE_64MB,
  MiB128 = MPU_REGION_SIZE_128MB,
  MiB256 = MPU_REGION_SIZE_256MB,
  MiB512 = MPU_REGION_SIZE_512MB,
  GiB1   = MPU_REGION_SIZE_1GB,
  GiB2   = MPU_REGION_SIZE_2GB,
  GiB4   = MPU_REGION_SIZE_4GB,
};

constexpr std::size_t GetRegionSize(MpuRegionSize size) {
  using namespace ct::literals;

  switch (size) {
  case MpuRegionSize::B32: return 32_B;
  case MpuRegionSize::B64: return 64_B;
  case MpuRegionSize::B128: return 128_B;
  case MpuRegionSize::B256: return 256_B;
  case MpuRegionSize::B512: return 512_B;
  case MpuRegionSize::KiB1: return 1_KiB;
  case MpuRegionSize::KiB2: return 2_KiB;
  case MpuRegionSize::KiB4: return 4_KiB;
  case MpuRegionSize::KiB8: return 8_KiB;
  case MpuRegionSize::KiB16: return 16_KiB;
  case MpuRegionSize::KiB32: return 32_KiB;
  case MpuRegionSize::KiB64: return 64_KiB;
  case MpuRegionSize::KiB128: return 128_KiB;
  case MpuRegionSize::KiB256: return 256_KiB;
  case MpuRegionSize::KiB512: return 512_KiB;
  case MpuRegionSize::MiB1: return 1_MiB;
  case MpuRegionSize::MiB2: return 2_MiB;
  case MpuRegionSize::MiB4: return 4_MiB;
  case MpuRegionSize::MiB8: return 8_MiB;
  case MpuRegionSize::MiB16: return 16_MiB;
  case MpuRegionSize::MiB32: return 32_MiB;
  case MpuRegionSize::MiB64: return 64_MiB;
  case MpuRegionSize::MiB128: return 128_MiB;
  case MpuRegionSize::MiB256: return 256_MiB;
  case MpuRegionSize::MiB512: return 512_MiB;
  case MpuRegionSize::GiB1: return 1_GiB;
  case MpuRegionSize::GiB2: return 2_GiB;
  }

  std::unreachable();
}

consteval MpuRegionSize RegionSizeFromNumBytes(std::size_t num_bytes) {
  return ct::StaticMap<std::size_t, MpuRegionSize, 27>(
      num_bytes,
      {{
          {32_B, MpuRegionSize::B32},       {64_B, MpuRegionSize::B64},
          {128_B, MpuRegionSize::B128},     {256_B, MpuRegionSize::B256},
          {512_B, MpuRegionSize::B512},     {1_KiB, MpuRegionSize::KiB1},
          {2_KiB, MpuRegionSize::KiB2},     {4_KiB, MpuRegionSize::KiB4},
          {8_KiB, MpuRegionSize::KiB8},     {16_KiB, MpuRegionSize::KiB16},
          {32_KiB, MpuRegionSize::KiB32},   {64_KiB, MpuRegionSize::KiB64},
          {128_KiB, MpuRegionSize::KiB128}, {256_KiB, MpuRegionSize::KiB256},
          {512_KiB, MpuRegionSize::KiB512}, {1_MiB, MpuRegionSize::MiB1},
          {2_MiB, MpuRegionSize::MiB2},     {4_MiB, MpuRegionSize::MiB4},
          {8_MiB, MpuRegionSize::MiB8},     {16_MiB, MpuRegionSize::MiB16},
          {32_MiB, MpuRegionSize::MiB32},   {64_MiB, MpuRegionSize::MiB64},
          {128_MiB, MpuRegionSize::MiB128}, {256_MiB, MpuRegionSize::MiB256},
          {512_MiB, MpuRegionSize::MiB512}, {1_GiB, MpuRegionSize::GiB1},
          {2_GiB, MpuRegionSize::GiB2},
      }});
}

enum class MpuRegionNumber : uint8_t {
  Region0 = MPU_REGION_NUMBER0,
  Region1 = MPU_REGION_NUMBER1,
  Region2 = MPU_REGION_NUMBER2,
  Region3 = MPU_REGION_NUMBER3,
  Region4 = MPU_REGION_NUMBER4,
  Region5 = MPU_REGION_NUMBER5,
  Region6 = MPU_REGION_NUMBER6,
  Region7 = MPU_REGION_NUMBER7,
#if defined(CORE_CM7)
  Region8  = MPU_REGION_NUMBER8,
  Region9  = MPU_REGION_NUMBER9,
  Region10 = MPU_REGION_NUMBER10,
  Region11 = MPU_REGION_NUMBER11,
  Region12 = MPU_REGION_NUMBER12,
  Region13 = MPU_REGION_NUMBER13,
  Region14 = MPU_REGION_NUMBER14,
  Region15 = MPU_REGION_NUMBER15,
#endif
};

enum class MpuTexLevel : uint8_t {
  Level0 = MPU_TEX_LEVEL0,
  Level1 = MPU_TEX_LEVEL1,
  Level2 = MPU_TEX_LEVEL2,
};

enum class MpuAccess : uint8_t {
  NoAccess                                = MPU_REGION_NO_ACCESS,
  PriviligedReadWrite                     = MPU_REGION_PRIV_RW,
  PriviligedReadWriteUnpriviligedReadOnly = MPU_REGION_PRIV_RW_URO,
  FullAccess                              = MPU_REGION_FULL_ACCESS,
  PriviligedReadOnly                      = MPU_REGION_PRIV_RO,
  PriviligedReadOnlyUnpriviligedReadOnly  = MPU_REGION_PRIV_RO_URO
};

struct MpuRegionConfig {
  MpuRegionNumber num;
  uint32_t        base_addr;
  MpuRegionSize   size;
  MpuTexLevel     tex;
  MpuAccess       access;
  bool            executable;
  bool            shareable;
  bool            cacheable;
  bool            bufferable;

  consteval bool Validate() const noexcept {
    // Base address must be aligned to the region size
    assert(("Base address must be aligned to region size",
            base_addr % GetRegionSize(size) == 0));

    // Validate TEX field + C / B bit combinations
    switch (tex) {
    case MpuTexLevel::Level0:
      // If C == 0
      if (!cacheable) {
        assert(("Region with TEX 0 and Cacheable = false is always shareable",
                shareable));
      }
      break;
    case MpuTexLevel::Level1:
      if (!cacheable && bufferable) {
        assert(
            ("Region with TEX 1 may not have Cacheable=false, Bufferable=true",
             false));
      }
      break;
    case MpuTexLevel::Level2:
      assert(("Region with TEX 2 may not have Cacheable=true", !cacheable));
      break;
    }

    return true;
  }
};

enum class MpuControlMode {
  BackgroundNoAccess                                   = MPU_HFNMI_PRIVDEF_NONE,
  BackgroundNoAccessMpuEnabledDuringNmiHardFault       = MPU_HARDFAULT_NMI,
  BackgroundPrivilegedOnly                             = MPU_PRIVILEGED_DEFAULT,
  BackgroundPrivilegedOnlyMpuEnabledDuringNmiHardFault = MPU_HFNMI_PRIVDEF,
};

namespace detail {

void ConfigureMpuImpl(MpuControlMode                   control_mode,
                      std::span<const MpuRegionConfig> regions) noexcept;

}

template <MpuRegionConfig... Rs>
void ConfigureMpu(MpuControlMode control_mode =
                      MpuControlMode::BackgroundPrivilegedOnly) noexcept {
  static_assert((... && Rs.Validate()));

  constexpr std::array<MpuRegionConfig, sizeof...(Rs)> Regions{Rs...};
  detail::ConfigureMpuImpl(control_mode, Regions);
}

enum class CachePolicy {
  WriteThrough,
  WriteBackNoWriteAllocate,
  WriteBackWriteReadAllocate
};

struct MpuCachePolicySettings {
  MpuTexLevel tex;
  bool        cacheable;
  bool        bufferable;
};

consteval MpuCachePolicySettings GetSettingsForCachePolicy(CachePolicy policy) {
  switch (policy) {
  case CachePolicy::WriteThrough:
    return {.tex = MpuTexLevel::Level0, .cacheable = true, .bufferable = false};
  case CachePolicy::WriteBackNoWriteAllocate:
    return {.tex = MpuTexLevel::Level0, .cacheable = true, .bufferable = true};
  case CachePolicy::WriteBackWriteReadAllocate:
    return {.tex = MpuTexLevel::Level1, .cacheable = true, .bufferable = true};
  }
}

inline constexpr auto CacheWt =
    GetSettingsForCachePolicy(CachePolicy::WriteThrough);
inline constexpr auto CacheWbnwa =
    GetSettingsForCachePolicy(CachePolicy::WriteBackNoWriteAllocate);
inline constexpr auto CacheWbwa =
    GetSettingsForCachePolicy(CachePolicy::WriteBackWriteReadAllocate);

template <MpuRegionNumber RN>
inline constexpr MpuRegionConfig MpuDefaultFlashConfig{
    .num        = RN,
    .base_addr  = 0x0000'0000,
    .size       = MpuRegionSize::MiB512,
    .tex        = CacheWt.tex,
    .access     = MpuAccess::FullAccess,
    .executable = true,
    .shareable  = false,
    .cacheable  = CacheWt.cacheable,
    .bufferable = CacheWt.bufferable,
};

template <MpuRegionNumber RN>
inline constexpr MpuRegionConfig MpuDefaultSramConfig{
    .num        = RN,
    .base_addr  = 0x2000'0000,
    .size       = MpuRegionSize::MiB512,
    .tex        = CacheWbwa.tex,
    .access     = MpuAccess::FullAccess,
    .executable = true,
    .shareable  = false,
    .cacheable  = CacheWbwa.cacheable,
    .bufferable = CacheWbwa.bufferable,
};

#if defined(CORE_CM7)

inline void SetInstructionCacheEnabled(bool enable) noexcept {
  if (enable) {
    SCB_EnableICache();
  } else {
    SCB_DisableICache();
  }
}

inline void SetDataCacheEnabled(bool enable) noexcept {
  if (enable) {
    SCB_EnableDCache();
  } else {
    SCB_DisableDCache();
  }
}

#endif

}   // namespace stm32h7