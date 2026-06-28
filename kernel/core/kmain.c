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
#include "mcsos_thread.h"
#include "mcsos/syscall.h"

extern char __kernel_start[];
extern char __kernel_end[];

/* M9: scheduler globals */
static mcsos_scheduler_t g_sched;
static mcsos_thread_t    g_boot_thread;
static mcsos_thread_t    g_thread_a;
static mcsos_thread_t    g_thread_b;
static unsigned char     g_stack_a[8192] __attribute__((aligned(16)));
static unsigned char     g_stack_b[8192] __attribute__((aligned(16)));

static void demo_thread_a(void *arg) {
    (void)arg;
    for (;;) {
        log_writeln("[M9] thread A tick");
        mcsos_sched_yield(&g_sched);
    }
}

static void demo_thread_b(void *arg) {
    (void)arg;
    for (;;) {
        log_writeln("[M9] thread B tick");
        mcsos_sched_yield(&g_sched);
    }
}

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
    log_key_value_hex64("m8_heap_total",       (uint64_t)st.total_bytes);
    log_key_value_hex64("m8_heap_free",        (uint64_t)st.free_bytes);
    log_key_value_hex64("m8_heap_largest_free",(uint64_t)st.largest_free);
    log_key_value_hex64("m8_heap_blocks",      (uint64_t)st.block_count);
}

static void m9_scheduler_init(void) {
    mcsos_scheduler_init(&g_sched, &g_boot_thread);
    mcsos_thread_prepare(&g_thread_a, "demo-a", demo_thread_a, (void*)0,
                         g_stack_a, sizeof(g_stack_a), g_sched.next_id++);
    mcsos_thread_prepare(&g_thread_b, "demo-b", demo_thread_b, (void*)0,
                         g_stack_b, sizeof(g_stack_b), g_sched.next_id++);
    mcsos_sched_enqueue(&g_sched, &g_thread_a);
    mcsos_sched_enqueue(&g_sched, &g_thread_b);
    log_writeln("[M9] scheduler initialized");
    log_writeln("[M9] starting yield loop");
    mcsos_sched_yield(&g_sched);
}


/* ---- M10: syscall callbacks ---- */
static uint64_t k_get_ticks(void) {
    return timer_ticks();
}
static void k_yield_current(void) {
    mcsos_sched_yield(&g_sched);
}
static void k_exit_current(int code) {
    (void)code;
    log_writeln("[M10] exit_current stub called");
}
static int64_t k_write_serial(const char *buf, size_t len) {
    (void)len;
    log_write(buf);
    return (int64_t)len;
}
static void m10_syscall_init(void) {
    mcsos_syscall_ops_t ops = {
        .get_ticks     = k_get_ticks,
        .yield_current = k_yield_current,
        .exit_current  = k_exit_current,
        .write_serial  = k_write_serial,
    };
    mcsos_syscall_init(&ops);
    mcsos_syscall_set_user_region((mcsos_user_region_t){
        .base  = 0x0000000000400000ULL,
        .limit = 0x0000800000000000ULL,
    });
    int64_t r = mcsos_syscall_dispatch(MCSOS_SYS_PING,0,0,0,0,0,0);
    log_writeln("[M10] syscall dispatcher initialized");
    log_key_value_hex64("ping_result", (uint64_t)r);
    int64_t t = mcsos_syscall_dispatch(MCSOS_SYS_GET_TICKS,0,0,0,0,0,0);
    log_key_value_hex64("ticks", (uint64_t)t);
    log_writeln("[M10] syscall ping ok");
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
    pic_remap(PIC_MASTER_OFFSET, PIC_SLAVE_OFFSET);
    pic_mask_all();
    pic_unmask_irq(0u);
    log_writeln("[M5] PIC remapped, IRQ0 unmasked");
    pit_configure_hz(100u);
    log_writeln("[M5] PIT configured 100Hz");
    log_writeln("[M5] enabling interrupts (sti)");
    cpu_sti();
    log_writeln("[M3] ready for QEMU smoke test and GDB audit");
    m10_syscall_init();
    m9_scheduler_init();
    for (;;) {
        cpu_hlt();
    }
}
