
SECTIONS {
    .flash.text :
    {
        . = ALIGN(4);
        
        __cli_commands_start = ABSOLUTE(.);
        KEEP(*(.cli.commands))
        __cli_commands_end = ABSOLUTE(.);
    } >iram0_2_seg
}
