#include <stdint.h>
#include <mcsos/arch/idt.h>
#include <mcsos/arch/pic.h>
#include <mcsos/arch/pit.h>
#include <mcsos/kernel/log.h>
#include <mcsos/kernel/panic.h>

static const char *x86_64_exception_name(uint64_t vector) {
    switch (vector) {
        case 0u:  return "#DE Divide Error";
        case 1u:  return "#DB Debug";
        case 2u:  return "#NMI Interrupt";
        case 3u:  return "#BP Breakpoint";
        case 4u:  return "#OF Overflow";
        case 5u:  return "#BR BOUND Range Exceeded";
        case 6u:  return "#UD Invalid Opcode";
        case 7u:  return "#NM Device Not Available";
        case 8u:  return "#DF Double Fault";
        case 10u: return "#TS Invalid TSS";
        case 11u: return "#NP Segment Not Present";
        case 12u: return "#SS Stack-Segment Fault";
        case 13u: return "#GP General Protection";
        case 14u: return "#PF Page Fault";
        case 16u: return "#MF x87 FP Exception";
        case 17u: return "#AC Alignment Check";
        case 18u: return "#MC Machine Check";
        case 19u: return "#XM SIMD FP Exception";
        case 21u: return "#CP Control Protection";
        case 29u: return "#VC VMM Communication";
        case 30u: return "#SX Security Exception";
        default:  return "Reserved/Unknown";
    }
}

void x86_64_trap_dispatch(x86_64_trap_frame_t *frame) {
    if (frame->vector >= PIC_MASTER_OFFSET &&
        frame->vector < (PIC_SLAVE_OFFSET + 8u)) {
        uint8_t irq = (uint8_t)(frame->vector - PIC_MASTER_OFFSET);
        if (irq == 0u) {
            timer_on_irq0();
        } else {
            log_writeln("[M5] unexpected IRQ");
        }
        pic_send_eoi(irq);
        return;
    }

    if (frame->vector == 3u) {
        log_writeln("[M4] breakpoint handled, returning from handler");
        return;
    }

    log_writeln("[M4] exception entered");
    log_key_value_hex64("trap_vector",     frame->vector);
    log_key_value_hex64("trap_error_code", frame->error_code);
    log_key_value_hex64("trap_rip",        frame->rip);
    log_key_value_hex64("trap_cs",         frame->cs);
    log_key_value_hex64("trap_rflags",     frame->rflags);
    log_writeln(x86_64_exception_name(frame->vector));
    KERNEL_PANIC("unhandled CPU exception, halting (fail-closed policy)", 0xC0DEFA17u);
}
