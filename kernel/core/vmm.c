#include <mcsos/kernel/vmm.h>

static void vmm_zero_page(uint64_t *page) {
    for (size_t i = 0; i < VMM_ENTRIES_PER_TABLE; i++) page[i] = 0;
}

bool vmm_is_aligned_4k(uint64_t value) {
    return (value & (VMM_PAGE_SIZE - 1ULL)) == 0;
}

bool vmm_is_canonical(uint64_t vaddr) {
    uint64_t sign  = (vaddr >> 47) & 1ULL;
    uint64_t upper = vaddr >> 48;
    return sign ? (upper == 0xFFFFULL) : (upper == 0ULL);
}

static unsigned idx_pml4(uint64_t v) { return (unsigned)((v >> 39) & 0x1FFULL); }
static unsigned idx_pdpt(uint64_t v) { return (unsigned)((v >> 30) & 0x1FFULL); }
static unsigned idx_pd  (uint64_t v) { return (unsigned)((v >> 21) & 0x1FFULL); }
static unsigned idx_pt  (uint64_t v) { return (unsigned)((v >> 12) & 0x1FFULL); }

static uint64_t *table_from_phys(struct vmm_space *space, uint64_t paddr) {
    if (!space || !space->phys_to_virt || !vmm_is_aligned_4k(paddr)) return 0;
    return (uint64_t *)space->phys_to_virt(space->ctx, paddr);
}

static int get_or_alloc_next_table(struct vmm_space *space, uint64_t *table,
                                   unsigned index, uint64_t **out) {
    uint64_t entry = table[index];
    if (entry & VMM_PTE_PRESENT) {
        if (entry & VMM_PTE_HUGE) return VMM_ERR_EXISTS;
        uint64_t *next = table_from_phys(space, entry & VMM_PTE_ADDR_MASK);
        if (!next) return VMM_ERR_INVAL;
        *out = next;
        return VMM_MAP_OK;
    }
    if (!space->alloc_frame) return VMM_ERR_NOMEM;
    uint64_t new_paddr = space->alloc_frame(space->ctx);
    if (new_paddr == VMM_INVALID_PHYS || !vmm_is_aligned_4k(new_paddr)) return VMM_ERR_NOMEM;
    uint64_t *new_table = table_from_phys(space, new_paddr);
    if (!new_table) {
        if (space->free_frame) space->free_frame(space->ctx, new_paddr);
        return VMM_ERR_INVAL;
    }
    vmm_zero_page(new_table);
    table[index] = (new_paddr & VMM_PTE_ADDR_MASK) | VMM_PTE_PRESENT | VMM_PTE_WRITABLE;
    *out = new_table;
    return VMM_MAP_OK;
}

int vmm_space_init(struct vmm_space *space, uint64_t root_paddr, void *ctx,
                   vmm_alloc_frame_fn alloc_frame, vmm_free_frame_fn free_frame,
                   vmm_phys_to_virt_fn phys_to_virt) {
    if (!space || !phys_to_virt || !vmm_is_aligned_4k(root_paddr)) return VMM_ERR_INVAL;
    space->root_paddr   = root_paddr;
    space->ctx          = ctx;
    space->alloc_frame  = alloc_frame;
    space->free_frame   = free_frame;
    space->phys_to_virt = phys_to_virt;
    return VMM_MAP_OK;
}

int vmm_map_page(struct vmm_space *space, uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    if (!space || !vmm_is_canonical(vaddr) || !vmm_is_aligned_4k(vaddr) ||
        !vmm_is_aligned_4k(paddr)) return VMM_ERR_INVAL;
    uint64_t *pml4 = table_from_phys(space, space->root_paddr);
    if (!pml4) return VMM_ERR_INVAL;
    uint64_t *pdpt = 0, *pd = 0, *pt = 0;
    int rc;
    rc = get_or_alloc_next_table(space, pml4, idx_pml4(vaddr), &pdpt); if (rc) return rc;
    rc = get_or_alloc_next_table(space, pdpt, idx_pdpt(vaddr), &pd);   if (rc) return rc;
    rc = get_or_alloc_next_table(space, pd,   idx_pd(vaddr),   &pt);   if (rc) return rc;
    unsigned pti = idx_pt(vaddr);
    if (pt[pti] & VMM_PTE_PRESENT) return VMM_ERR_EXISTS;
    uint64_t allowed = VMM_PTE_WRITABLE | VMM_PTE_USER | VMM_PTE_WRITE_THROUGH |
                       VMM_PTE_CACHE_DISABLE | VMM_PTE_GLOBAL | VMM_PTE_NO_EXECUTE;
    pt[pti] = (paddr & VMM_PTE_ADDR_MASK) | VMM_PTE_PRESENT | (flags & allowed);
    return VMM_MAP_OK;
}

int vmm_query_page(struct vmm_space *space, uint64_t vaddr, struct vmm_mapping *out) {
    if (!space || !out || !vmm_is_canonical(vaddr) || !vmm_is_aligned_4k(vaddr))
        return VMM_ERR_INVAL;
    uint64_t *pml4 = table_from_phys(space, space->root_paddr);
    if (!pml4) return VMM_ERR_INVAL;

    uint64_t e1 = pml4[idx_pml4(vaddr)];
    if (!(e1 & VMM_PTE_PRESENT) || (e1 & VMM_PTE_HUGE)) return VMM_ERR_NOT_FOUND;
    uint64_t *pdpt = table_from_phys(space, e1 & VMM_PTE_ADDR_MASK);
    if (!pdpt) return VMM_ERR_INVAL;

    uint64_t e2 = pdpt[idx_pdpt(vaddr)];
    if (!(e2 & VMM_PTE_PRESENT) || (e2 & VMM_PTE_HUGE)) return VMM_ERR_NOT_FOUND;
    uint64_t *pd = table_from_phys(space, e2 & VMM_PTE_ADDR_MASK);
    if (!pd) return VMM_ERR_INVAL;

    uint64_t e3 = pd[idx_pd(vaddr)];
    if (!(e3 & VMM_PTE_PRESENT) || (e3 & VMM_PTE_HUGE)) return VMM_ERR_NOT_FOUND;
    uint64_t *pt = table_from_phys(space, e3 & VMM_PTE_ADDR_MASK);
    if (!pt) return VMM_ERR_INVAL;

    uint64_t e4 = pt[idx_pt(vaddr)];
    if (!(e4 & VMM_PTE_PRESENT)) return VMM_ERR_NOT_FOUND;
    out->vaddr = vaddr;
    out->paddr = e4 & VMM_PTE_ADDR_MASK;
    out->flags = e4 & ~VMM_PTE_ADDR_MASK;
    return VMM_MAP_OK;
}

int vmm_unmap_page(struct vmm_space *space, uint64_t vaddr) {
    if (!space || !vmm_is_canonical(vaddr) || !vmm_is_aligned_4k(vaddr)) return VMM_ERR_INVAL;
    uint64_t *pml4 = table_from_phys(space, space->root_paddr);
    if (!pml4) return VMM_ERR_INVAL;

    uint64_t e1 = pml4[idx_pml4(vaddr)];
    if (!(e1 & VMM_PTE_PRESENT) || (e1 & VMM_PTE_HUGE)) return VMM_ERR_NOT_FOUND;
    uint64_t *pdpt = table_from_phys(space, e1 & VMM_PTE_ADDR_MASK);
    if (!pdpt) return VMM_ERR_INVAL;

    uint64_t e2 = pdpt[idx_pdpt(vaddr)];
    if (!(e2 & VMM_PTE_PRESENT) || (e2 & VMM_PTE_HUGE)) return VMM_ERR_NOT_FOUND;
    uint64_t *pd = table_from_phys(space, e2 & VMM_PTE_ADDR_MASK);
    if (!pd) return VMM_ERR_INVAL;

    uint64_t e3 = pd[idx_pd(vaddr)];
    if (!(e3 & VMM_PTE_PRESENT) || (e3 & VMM_PTE_HUGE)) return VMM_ERR_NOT_FOUND;
    uint64_t *pt = table_from_phys(space, e3 & VMM_PTE_ADDR_MASK);
    if (!pt) return VMM_ERR_INVAL;

    unsigned pti = idx_pt(vaddr);
    if (!(pt[pti] & VMM_PTE_PRESENT)) return VMM_ERR_NOT_FOUND;
    pt[pti] = 0;
    vmm_invalidate_page(vaddr);
    return VMM_MAP_OK;
}

#if defined(__x86_64__) && !defined(MCSOS_HOST_TEST)
void vmm_invalidate_page(uint64_t vaddr) {
    __asm__ volatile("invlpg (%0)" :: "r"((void *)vaddr) : "memory");
}
uint64_t vmm_read_cr3(void) {
    uint64_t v; __asm__ volatile("mov %%cr3, %0" : "=r"(v) :: "memory"); return v;
}
void vmm_write_cr3(uint64_t value) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(value) : "memory");
}
uint64_t vmm_read_cr2(void) {
    uint64_t v; __asm__ volatile("mov %%cr2, %0" : "=r"(v) :: "memory"); return v;
}
#else
void vmm_invalidate_page(uint64_t vaddr) { (void)vaddr; }
uint64_t vmm_read_cr3(void) { return 0; }
void vmm_write_cr3(uint64_t value) { (void)value; }
uint64_t vmm_read_cr2(void) { return 0; }
#endif
