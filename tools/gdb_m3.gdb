set pagination off
set confirm off
file build/kernel.elf
target remote localhost:1234
hbreak kmain
hbreak kernel_panic_at
continue
