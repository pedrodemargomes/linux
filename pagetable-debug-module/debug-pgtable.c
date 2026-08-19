#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/pid.h>
#include <linux/hugetlb.h>
#include <linux/pgtable.h>
#include <linux/pagemap.h>

static int pid = 1;
module_param(pid, int, 0444);
MODULE_PARM_DESC(pid, "PID to inspect");


static void dump_folio(struct page *page)
{
	struct folio *folio;

	if (!page) {
		pr_info("    page = NULL\n");
		return;
	}

	folio = page_folio(page);

	pr_info("    page       = %px\n", page);
	pr_info("    folio      = %px\n", folio);
	pr_info("    PFN        = %#lx\n", page_to_pfn(page));
	pr_info("    PA         = %#llx\n",
		(unsigned long long)page_to_phys(page));

	pr_info("    order      = %u\n", folio_order(folio));
	pr_info("    nr_pages   = %lu\n", folio_nr_pages(folio));
	pr_info("    refcount   = %d\n", folio_ref_count(folio));
	pr_info("    mapcount   = %d\n", folio_mapcount(folio));

	pr_info("    anonymous  = %s\n",
		folio_test_anon(folio) ? "yes" : "no");

	pr_info("    hugetlb    = %s\n",
		folio_test_hugetlb(folio) ? "yes" : "no");

}


static void dump_pmd(struct mm_struct *mm,
		     struct vm_area_struct *vma,
		     unsigned long addr,
		     pmd_t *pmd)
{
	pmd_t val;
	unsigned long pfn;
	struct page *page;

	/*
	 * The PMD can change concurrently with page-table operations.
	 *
	 * For a debugging walker we take the PMD page-table lock while
	 * inspecting the entry.
	 */
	spinlock_t *ptl;

	ptl = pmd_lock(mm, pmd);

	val = READ_ONCE(*pmd);

	if (pmd_none(val)) {
		spin_unlock(ptl);
		return;
	}

	pr_info("\n");
	pr_info("========================================\n");
	pr_info("PMD mapping\n");
	pr_info("========================================\n");

	pr_info("  VA          = %#lx\n", addr);
	pr_info("  PMD         = %#lx\n",
		(unsigned long)pmd_val(val));

	pr_info("  VMA         = [%#lx-%#lx]\n",
		vma->vm_start,
		vma->vm_end);

	pr_info("  VMA flags   = %#lx\n",
		vma->vm_flags);

	pr_info("  present     = %s\n",
		pmd_present(val) ? "yes" : "no");

	pr_info("  leaf        = %s\n",
		pmd_leaf(val) ? "yes" : "no");

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	pr_info("  trans_huge  = %s\n",
		pmd_trans_huge(val) ? "yes" : "no");
#endif

#ifdef CONFIG_PMEM
	pr_info("  devmap      = %s\n",
		pmd_devmap(val) ? "yes" : "no");
#endif

	pr_info("  young       = %s\n",
		pmd_young(val) ? "yes" : "no");

	pr_info("  dirty       = %s\n",
		pmd_dirty(val) ? "yes" : "no");

	pr_info("  write       = %s\n",
		pmd_write(val) ? "yes" : "no");

	/*
	 * A non-leaf PMD points to a PTE table.
	 *
	 * There is no directly mapped physical page to report here.
	 */
	if (!pmd_leaf(val)) {
		pr_info("  type        = PTE table\n");

		spin_unlock(ptl);
		return;
	}

	pr_info("  type        = leaf mapping\n");

	if (!pmd_present(val)) {
		pr_info("  physical    = not present\n");

		spin_unlock(ptl);
		return;
	}

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	if (pmd_trans_huge(val))
		pr_info("  *** THP PMD ***\n");
#endif



	/*
	 * Obtain the PFN encoded in the PMD.
	 */
	pfn = pmd_pfn(val);

	pr_info("  PFN         = %#lx\n", pfn);
	pr_info("  PA          = %#llx\n",
		(unsigned long long)PFN_PHYS(pfn));

	/*
	 * pfn_valid() is important before converting the PFN into
	 * struct page.
	 */
	if (!pfn_valid(pfn)) {
		pr_info("  struct page = invalid PFN\n");

		spin_unlock(ptl);
		return;
	}

	page = pfn_to_page(pfn);

	/*
	 * Keep the folio alive while we inspect it.
	 *
	 * This is primarily useful for a debugging module because a
	 * folio may otherwise disappear after the page-table lock is
	 * released.
	 */
	if (!folio_try_get(page_folio(page))) {
		pr_info("  folio       = could not acquire reference\n");

		spin_unlock(ptl);
		return;
	}

	spin_unlock(ptl);

	dump_folio(page);

	folio_put(page_folio(page));
}


static void walk_pmds(struct mm_struct *mm)
{
	struct vm_area_struct *vma;
	VMA_ITERATOR(vmi, mm, 0);

	/*
	 * Stabilizes the VMA tree and the VMAs while we walk them.
	 */
	mmap_read_lock(mm);

	for_each_vma(vmi, vma) {
		unsigned long addr;
		unsigned long end;

		pr_info("\n");
		pr_info("VMA [%#lx-%#lx] flags=%#lx\n",
			vma->vm_start,
			vma->vm_end,
			vma->vm_flags);

		if (is_vm_hugetlb_page(vma))
			pr_info("  VMA type: hugetlb\n");

		addr = vma->vm_start;
		end = vma->vm_end;

		while (addr < end) {
			pgd_t *pgd;
			p4d_t *p4d;
			pud_t *pud;
			pmd_t *pmd;
			unsigned long next;

			pgd = pgd_offset(mm, addr);

			if (pgd_none(*pgd) || pgd_bad(*pgd)) {
				addr = pgd_addr_end(addr, end);
				continue;
			}

			p4d = p4d_offset(pgd, addr);

			if (p4d_none(*p4d) || p4d_bad(*p4d)) {
				addr = p4d_addr_end(addr, end);
				continue;
			}

			pud = pud_offset(p4d, addr);

			if (pud_none(*pud) || pud_bad(*pud)) {
				addr = pud_addr_end(addr, end);
				continue;
			}

			/*
			 * A PUD leaf is a larger mapping (e.g. 1 GiB
			 * on x86-64). There is no PMD below it.
			 */
			if (pud_leaf(*pud)) {
				addr = pud_addr_end(addr, end);
				continue;
			}

			pmd = pmd_offset(pud, addr);

			next = pmd_addr_end(addr, end);

			if (!pmd_none(READ_ONCE(*pmd)))
				dump_pmd(mm, vma, addr, pmd);

			addr = next;
		}
	}

	mmap_read_unlock(mm);
}


static int __init pmd_dump_init(void)
{
	struct task_struct *task;
	struct mm_struct *mm;

	pr_info("pmd_dump: loading\n");
	pr_info("pmd_dump: PID=%d\n", pid);

	task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);

	if (!task) {
		pr_err("pmd_dump: PID %d not found\n", pid);
		return -ESRCH;
	}

	mm = get_task_mm(task);

	put_task_struct(task);

	if (!mm) {
		pr_err("pmd_dump: PID %d has no userspace mm\n", pid);
		return -EINVAL;
	}

	walk_pmds(mm);

	mmput(mm);

	pr_info("pmd_dump: finished\n");

	return 0;
}


static void __exit pmd_dump_exit(void)
{
	pr_info("pmd_dump: unloaded\n");
}


module_init(pmd_dump_init);
module_exit(pmd_dump_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pedro");
MODULE_DESCRIPTION("Dump PMD mappings of a process");

