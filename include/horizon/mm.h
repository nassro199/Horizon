/**
 * mm.h - Memory management definitions
 *
 * This file contains definitions for the memory management subsystem.
 */

#ifndef _HORIZON_MM_H
#define _HORIZON_MM_H

#include <horizon/types.h>

/* Page size */
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE - 1))

/* Memory allocation flags */
#define MEM_KERNEL     0x01    /* Kernel memory */
#define MEM_USER       0x02    /* User memory */
#define MEM_DMA        0x04    /* DMA-capable memory */
#define MEM_ZERO       0x08    /* Zero memory */

/* Memory protection flags */
#define MEM_PROT_READ  0x01    /* Readable */
#define MEM_PROT_WRITE 0x02    /* Writable */
#define MEM_PROT_EXEC  0x04    /* Executable */

/* Memory map structure */
typedef struct mm_map {
    void *start;              /* Start address */
    void *end;                /* End address */
    u32 flags;                /* Flags */
    struct mm_map *next;      /* Next map in list */
} mm_map_t;

/* Memory management functions */
void mm_init(void);
void *mm_alloc_pages(u32 count, u32 flags);
void mm_free_pages(void *addr, u32 count);
void *kmalloc(size_t size, u32 flags);
void kfree(void *addr);
void *vmalloc(size_t size);
void vfree(void *addr);

/* Page fault functions */
void page_fault_init(void);
void page_fault_handler(struct interrupt_frame *frame);
void page_fault_print_stats(void);

/* Swap functions */
void swap_init(void);
int swap_add(const char *path, u32 size);
int swap_remove(const char *path);
void swap_print_stats(void);

/* Swap policy functions */
void swap_policy_init(void);
int swap_policy_set(int policy);
int swap_policy_get(void);
int swap_policy_scan(struct task_struct *task, u32 count);
int swap_policy_prefetch(struct task_struct *task, u32 addr, u32 count);
void swap_policy_print_stats(void);

/* Swap compression functions */
void swap_compress_init(void);
int swap_compress_set_algo(int algo);
int swap_compress_get_algo(void);
ssize_t swap_compress_page(void *in, void *out, size_t in_size, size_t out_size);
ssize_t swap_decompress_page(void *in, void *out, size_t in_size, size_t out_size);
void swap_compress_print_stats(void);

/* Swap prioritization functions */
void swap_priority_init(void);
int swap_priority_set_algo(int algo);
int swap_priority_get_algo(void);
int swap_priority_get(struct task_struct *task, u32 addr);
int swap_priority_scan_high(struct task_struct *task, u32 count);
int swap_priority_scan_medium(struct task_struct *task, u32 count);
int swap_priority_scan_low(struct task_struct *task, u32 count);
void swap_priority_print_stats(void);

/* Swap monitoring functions */
void swap_monitor_init(void);
int swap_monitor_start(void);
int swap_monitor_stop(void);
int swap_monitor_set_interval(u64 interval);
int swap_monitor_set_threshold(u64 threshold);
int swap_monitor_set_auto_adjust(int enable);
void swap_monitor_update(u64 swap_in, u64 swap_out);
void swap_monitor_print_stats(void);

/* TLB functions */
void tlb_init(void);
void tlb_flush_single(u32 addr);
void tlb_flush_all(void);
void tlb_flush_range(u32 start, u32 end);
void tlb_flush_task(struct task_struct *task);
void tlb_flush_mm(struct mm_struct *mm);
void tlb_flush_vma(struct mm_struct *mm, struct vm_area_struct *vma);
void tlb_print_stats(void);

/* Cache functions */
void cache_init(void);
void cache_flush_data(void);
void cache_flush_instruction(void);
void cache_flush_all(void);
void cache_invalidate_data(void);
void cache_invalidate_instruction(void);
void cache_invalidate_all(void);
void cache_flush_range(void *addr, size_t size);
void cache_prefetch_data(void *addr);
void cache_prefetch_instruction(void *addr);
u32 cache_get_line_size(void);
void cache_print_stats(void);

/* Cache coherency functions */
void cache_coherency_init(void);
int cache_coherency_set_protocol(int protocol);
int cache_coherency_get_protocol(void);
int cache_coherency_invalidate(u64 address);
int cache_coherency_flush(u64 address);
int cache_coherency_broadcast(u64 address);
int cache_coherency_snoop(u64 address, int cpu);
int cache_coherency_read(u64 address, int cpu);
int cache_coherency_write(u64 address, int cpu);
void cache_coherency_print_stats(void);

/* NUMA functions */
void numa_init(void);
void numa_detect_nodes(void);
int numa_get_node_count(void);
struct numa_node *numa_get_node(int id);
struct numa_node *numa_get_local_node(void);
struct numa_node *numa_get_node_for_addr(u64 phys_addr);
int numa_set_policy(int policy, int preferred_node);
int numa_get_policy(void);
void *numa_alloc_pages(int node_id, u32 count, u32 flags);
void numa_free_pages(void *addr, u32 count);
void *numa_policy_alloc_pages(u32 count, u32 flags);
int numa_migrate_page(void *addr, int target_node);
void numa_print_stats(void);

/* Memory migration functions */
void memory_migration_init(void);
int memory_migration_enable(int enable);
int memory_migration_set_interval(u64 interval);
int memory_migration_set_threshold(u64 threshold);
int memory_migration_needed(void);
int memory_migration_check_imbalance(void);
int memory_migration_run(void);
int memory_migration_range(void *addr, size_t size, int target_node);
int memory_migration_task(struct task_struct *task, int target_node);
void memory_migration_print_stats(void);

#endif /* _HORIZON_MM_H */
