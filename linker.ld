/* 链接脚本，定义程序从0x40080000开始加载 */
ENTRY(_boot)

SECTIONS
{
    . = 0x40080000;

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }
}
