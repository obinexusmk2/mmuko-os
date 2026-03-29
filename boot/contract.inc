%ifndef NSIGII_CONTRACT_INC
%define NSIGII_CONTRACT_INC

CONTRACT_BASE           equ 0x0600
STATE_BLOCK_BASE        equ 0x0680
STAGE1_LOAD_ADDR        equ 0x7C00
STAGE2_LOAD_SEGMENT     equ 0x0000
STAGE2_LOAD_OFFSET      equ 0x8000
STAGE2_LOADER_SECTORS   equ 1
STAGE2_TOTAL_SECTORS    equ 8
KERNEL_LOAD_SEGMENT     equ 0x0000
KERNEL_LOAD_OFFSET      equ 0x8200
CONTRACT_SIGNATURE      equ 0x3247534E    ; 'NSG2'
STATE_SIGNATURE         equ 0x4947494E    ; 'NIGI'
CONTRACT_VERSION        equ 0x0001

T_NO                    equ 0x00
T_YES                   equ 0x01
T_MB                    equ 0xFF
T_MBN                   equ 0xFE
O_PASS                  equ 0xAA
O_HOLD                  equ 0xBB
O_ALRT                  equ 0xCC

CONTRACT_SIG_OFF        equ 0x00
CONTRACT_VER_OFF        equ 0x04
CONTRACT_BOOT_DRIVE_OFF equ 0x06
CONTRACT_STAGE2_SEG_OFF equ 0x08
CONTRACT_STAGE2_OFF_OFF equ 0x0A
CONTRACT_STAGE2_SEC_OFF equ 0x0C
CONTRACT_KERNEL_SEG_OFF equ 0x0E
CONTRACT_KERNEL_OFF_OFF equ 0x10
CONTRACT_FLAGS_OFF      equ 0x12
CONTRACT_STATE_PTR_OFF  equ 0x14
CONTRACT_MEM_KB_OFF     equ 0x16
CONTRACT_LAST_ERR_OFF   equ 0x18
CONTRACT_MEMBRANE_OFF   equ 0x19
CONTRACT_RING_LEVEL_OFF equ 0x0B    ; protection ring level (was reserved0)
RING_0_KERNEL           equ 0x00
RING_1_DRIVER           equ 0x01
RING_2_SERVICE          equ 0x02
RING_3_USER             equ 0x03
CONTRACT_TIER1_OFF      equ 0x1A
CONTRACT_TIER2_OFF      equ 0x1B
CONTRACT_MEMMAP_PTR_OFF equ 0x1C
CONTRACT_MEMMAP_CNT_OFF equ 0x1E

STATE_SIG_OFF           equ 0x00
STATE_VER_OFF           equ 0x04
STATE_MEMBRANE_OFF      equ 0x06
STATE_TIER1_OFF         equ 0x07
STATE_TIER2_OFF         equ 0x08
STATE_BOOT_DRIVE_OFF    equ 0x09
STATE_MEM_KB_OFF        equ 0x0A
STATE_FLAGS_OFF         equ 0x0C
STATE_MSG_OFF           equ 0x0E

MEMMAP_BASE             equ 0x06C0
MEMMAP_COUNT            equ 4
MEMMAP_ENTRY_SIZE       equ 8

%endif
