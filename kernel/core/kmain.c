#include <stdint.h>
#include <mcsos/arch/cpu.h>
#include <mcsos/arch/idt.h>
#include <mcsos/arch/io.h>
#include <mcsos/arch/pic.h>
#include <mcsos/arch/pit.h>
#include <mcsos/kernel/log.h>
#include <mcsos/kernel/panic.h>
#include <mcsos/kernel/version.h>
#include <mcsos/kernel/kmem.h>

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

static unsigned char m8_boot_heap[64u * 1024u] __attribute__((aligned(4096)));

static void m8_heap_bootstrap(void) {
    int rc = kmem_init(m8_boot_heap, sizeof(m8_boot_heap));
    if (rc != 0) {
        KERNEL_PANIC("M8 kmem_init failed", (uint64_t)rc);
    }

    void *probe = kmem_alloc(128);
    if (probe == (void *)0) {
        KERNEL_PANIC("M8 kmem_alloc probe failed", 0u);
    }

    if (kmem_free_checked(probe) != 0) {
        KERNEL_PANIC("M8 kmem_free_checked probe failed", 0u);
    }

    kmem_stats_t st;
    kmem_get_stats(&st);
    log_writeln("[M8] kmem heap initialized");
    log_key_value_hex64("m8_heap_total", (uint64_t)st.total_bytes);
    log_key_value_hex64("m8_heap_free", (uint64_t)st.free_bytes);
    log_key_value_hex64("m8_heap_largest_free", (uint64_t)st.largest_free);
    log_key_value_hex64("m8_heap_blocks", (uint64_t)st.block_count);
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
    log_key_value_hex64("kernel_end",   (uint64_t)(uintptr_t)__kernel_end);
    log_key_value_hex64("rflags", cpu_read_rflags());
    m3_selftest();

    x86_64_idt_init();
    m4_selftest();
    m8_heap_bootstrap();

#ifdef MCSOS_M3_TRIGGER_PANIC
    KERNEL_PANIC("intentional M3 panic test", 0x4D43534F533033u);
#else
    log_writeln("[M3] panic path installed; intentional panic disabled");
#endif

#ifdef MCSOS_M4_TRIGGER_BREAKPOINT
    log_writeln("[M4] triggering int3 breakpoint test");
    x86_64_trigger_breakpoint_for_test();
    log_writeln("[M4] returned from breakpoint handler");
#endif

    log_writeln("[M4] IDT and exception dispatch path installed");

    /* M5: PIC remap, PIT, enable interrupts */
    pic_remap(PIC_MASTER_OFFSET, PIC_SLAVE_OFFSET);
    pic_mask_all();
    pic_unmask_irq(0u);
    log_writeln("[M5] PIC remapped, IRQ0 unmasked");

    pit_configure_hz(100u);
    log_writeln("[M5] PIT configured 100Hz");

    log_writeln("[M5] enabling interrupts (sti)");
    cpu_sti();

    log_writeln("[M3] ready for QEMU smoke test and GDB audit");
    uint64_t last_reported = 0;
    for (;;) {
        cpu_hlt();
        uint64_t now = timer_ticks();
        if (now - last_reported >= 100u) {
            log_key_value_hex64("ticks", now);
            last_reported = now;
        }
    }
}
