#include <stdint.h>
#include <mcsos/arch/cpu.h>
#include <mcsos/kernel/log.h>
#include <mcsos/kernel/panic.h>
#include <mcsos/kernel/version.h>
#include <mcsos/arch/idt.h>

extern char __kernel_start[];
extern char __kernel_end[];

static void m3_selftest(void) {
    KERNEL_ASSERT(__kernel_end > __kernel_start);
    KERNEL_ASSERT(sizeof(uintptr_t) == 8u);
    log_writeln("[M3] selftest: basic invariants passed");
}
static void m4_selftest(void) {
    KERNEL_ASSERT(x86_64_idt_limit_for_test() == (uint16_t)((256u * 16u) - 1u));
    KERNEL_ASSERT(x86_64_idt_base_for_test() != 0u);
    log_writeln("[M4] selftest: IDT installed with expected limit");
}

void kmain(void) {
    log_init();
    log_write(MCSOS_NAME);
    log_write(" ");
    log_write(MCSOS_VERSION);
    log_write(" ");
    log_write(MCSOS_MILESTONE);
    log_writeln(" kernel entered");
    log_key_value_hex64("kernel_start", (uint64_t)(uintptr_t)__kernel_start);
    log_key_value_hex64("kernel_end", (uint64_t)(uintptr_t)__kernel_end);
    log_key_value_hex64("rflags", cpu_read_rflags());
    m3_selftest();

    x86_64_idt_init();
    m4_selftest();

#ifdef MCSOS_M3_TRIGGER_PANIC
    KERNEL_PANIC("intentional M3 panic test", 0x4D43534F533033u);
#else
        log_writeln("[M3] panic path installed; intentional panic disabled");

#ifdef MCSOS_M4_TRIGGER_BREAKPOINT
        log_writeln("[M4] triggering int3 breakpoint test");
        x86_64_trigger_breakpoint_for_test();
        log_writeln("[M4] returned from breakpoint handler");
#endif

        log_writeln("[M4] IDT and exception dispatch path installed");
        log_writeln("[M3] ready for QEMU smoke test and GDB audit");
        cpu_halt_forever();
#endif
}
