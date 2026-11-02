OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 . = 0x80000000;
 .text : { *(.text.init) *(.text*) }
 . = 0x80008000;
 .data : { *(.rodata*) *(.sdata*) *(.data*) }
 .bss : { *(.sbss*) *(.bss*) *(COMMON) }
 __stack_top = 0x80010000;
}
