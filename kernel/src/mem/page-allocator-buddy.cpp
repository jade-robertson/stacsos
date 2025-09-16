/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <global.h>
#include <stacsos/kernel/debug.h>
#include <stacsos/kernel/mem/page-allocator-buddy.h>
#include <stacsos/kernel/mem/page.h>
#include <stacsos/memops.h>

using namespace stacsos;
using namespace stacsos::kernel;
using namespace stacsos::kernel::mem;

void page_allocator_buddy::dump() const
{
	dprintf("*** buddy page allocator - free list ***\n");

	for (int i = 0; i <= LastOrder; i++) {
		dprintf("[%02u] ", i);

		page *c = free_list_[i];
		while (c) {
			dprintf("%lx--%lx ", c->base_address(), (c->base_address() + ((1 << i) << PAGE_BITS)) - 1);
			c = c->next_free_;
		}

		dprintf("\n");
	}
}

/// @brief Insert a range of pages into the allocator.
/// @param range_start The start of the range
/// @param page_count The number of pages to insert
void page_allocator_buddy::insert_pages(page &range_start, u64 page_count)
{
	int offset = 0;
	u64 range_start_pfn = range_start.pfn();
	u64 range_end_pfn = range_start.pfn() + page_count;
	// Since the range of pages may not be aligned to anything, its necessary to have an algorithm to split the range into blocks
	// This finds some split point in the middle of the range, which splits such that there is at most one block of each size below and above it(excluding the
	// largest size) Then on both sides of the split point insert the blocks necessary.

	// split point is the largest number with one set bit that fits between range start and range end, or if no such number exists, the start of the range
	u64 split_point = 0;
	for (u64 i = LastOrder + 1; i >= 0; i--) {

		split_point = 1 << i;
		if (split_point >= range_start_pfn && split_point <= range_end_pfn)
			break;
		if (i == 0) {
			split_point = range_start_pfn;
			break;
		} // if no numbers fit in between, then split point is range start
	}

	u64 above_split = range_end_pfn - split_point;
	u64 below_split = split_point > range_start_pfn ? split_point - range_start_pfn : 0;

	// insert all the pages below the split point
	// for some count of pages, to insert n pages is equivalent to inserting power of two blocks for each high bit
	for (int i = 0; i <= LastOrder; i++) {
		if (below_split & 1 << i) {
			insert_free_block(i, page::get_from_pfn(range_start_pfn + offset));
			offset += (1 << i);
		}
	}
	// If there are too many pages to be inserted, it can be necessary to insert an arbitrary number of pages of high order
	while (range_end_pfn - (range_start_pfn + offset) > pages_per_block(LastOrder)) {
		insert_free_block(LastOrder, page::get_from_pfn(range_start_pfn + offset));
		offset += (1 << LastOrder);
	}

	// insert all the pages above the split point

	for (int i = LastOrder - 1; i >= 0; i--) {
		if (above_split & 1 << i) {

			insert_free_block(i, page::get_from_pfn(range_start_pfn + offset));
			offset += (1 << i);
		}
	}
}

/// @brief Removes a range of pages from the allocator
/// If they are not contained, they are skipped
/// @param range_start The start of the range to remove
/// @param page_count The number of pages to remove
void page_allocator_buddy::remove_pages(page &range_start, u64 page_count)
{
	// Since the pages to remove could be in any(potentially even multiple) orders, we need to scan though each order to find blocks that overlap with the range
	// to remove

	u64 range_end = range_start.pfn() + page_count;
	u64 removed = 0;

	// go through each order, largest first, and try to find the page range
	// If a block only partially overlaps the range, split it
	// Since we're going through the orders large -> small, we'll get the pages on the next pass
	for (int o = LastOrder; o >= 0; o--) {
		if (removed >= page_count)
			return; // If we've removed enough pages, we dont need to go through the rest of the orders
		if (free_list_[o] == nullptr)
			continue; // if this order is empty, skip to the next one
		page *cur_slot = free_list_[o];

		// Go through each page in order
		while (cur_slot) {
			// If the block is entirely within the range to remove, it can straightforwardly be removed
			if (cur_slot->pfn() >= range_start.pfn() && cur_slot->pfn() + pages_per_block(o) <= range_end) {
				page *n = cur_slot->next_free_;
				remove_free_block(o, *cur_slot);
				removed += pages_per_block(o);
				cur_slot = n;

			} else if (cur_slot->pfn() >= range_end) {
				// If the current block's pfn is after the end of the range to remove, the rest of the order can be skipped, because the orders are kept sorted
				break;
			} else if (cur_slot->pfn() + pages_per_block(o) <= range_start.pfn()) {
				cur_slot = cur_slot->next_free_; // non overlapping
			} else {
				page *n = cur_slot->next_free_; // If there's only a partial overlap, then split the block

				split_block(o, *cur_slot);
				cur_slot = n;
			}
		}
	}
}

/// @brief Insert a block into the free list
/// This keeps the list sorted, so takes O(n) wrt to the length of the free list
/// @param order The order to insert into
/// @param block_start The block to insert. Should be block aligned
void page_allocator_buddy::insert_free_block(int order, page &block_start)
{
	// Insets a block into an order, making sure to merge if it's buddy is in the list
	// Since the list is kept sorted, the buddy will either be directly before it or directly after
	assert(order >= 0 && order <= LastOrder);

	assert(block_aligned(order, block_start.pfn()));

	u64 buddy_addr = block_start.pfn() ^ pages_per_block(order);
	bool buddy_found = false;

	page *target = &block_start;
	page **slot = &free_list_[order];
	if ((*slot)->pfn() == buddy_addr)
		buddy_found = true;

	while (*slot && *slot < target) {
		slot = &((*slot)->next_free_);
		if ((*slot)->pfn() == buddy_addr)
			buddy_found = true;
	}

	assert(*slot != target);

	target->next_free_ = *slot;
	*slot = target;

	// if next blocks address is the buddies, or if the buddy was previously found, merge buddies
	if ((target->pfn() == buddy_addr || buddy_found) && order != LastOrder) {
		merge_buddies(order, block_start);
	}
}

/// @brief Remove a block from the free list
// This is modified from the starter code
/// @param order The order to remove from
/// @param block_start The page to start from
void page_allocator_buddy::remove_free_block(int order, page &block_start)
{
	// Get the pointer from the previous block to remove it
	page **candidate_slot = find_page_slot(order, block_start);

	*candidate_slot = block_start.next_free_;
	block_start.next_free_ = nullptr;
}

/// @brief Remove a block from an order, and insert it's two halves into the order below
/// No error checking, so make sure the block is definitely in the list
/// @param order The order to remove from
/// @param block_start The block to remove
void page_allocator_buddy::split_block(int order, page &block_start)
{
	// remove the block
	remove_free_block(order, block_start);
	// Get the two half blocks,
	page *half_1 = &block_start;
	page *half_2 = &page::get_from_pfn((&block_start)->pfn() + pages_per_block(order - 1));
	// Only insert one block, then manually set the pointers of the second half
	// This is fine, since the buddies will by definition be in the correct order
	// And prevents insert_free_block from merging the two blocks
	insert_free_block(order - 1, *half_1);
	half_2->next_free_ = half_1->next_free_;
	half_1->next_free_ = half_2;
}

/// @brief Merges a block with it's buddy
/// Note that both buddies must be in the list
/// @param order The order of the blocks
/// @param buddy One of the buddies
void page_allocator_buddy::merge_buddies(int order, page &buddy)
{
	// Get the address of both buddies, and the lowest one
	u64 buddy2_adr = buddy.pfn() ^ pages_per_block(order);
	u64 buddy1_adr = buddy.pfn();
	u64 lowest_adr = buddy1_adr < buddy2_adr ? buddy1_adr : buddy2_adr;

	// Remove the two buddies, by manually manipulating pointers
	// Since the list is sorted, the buddies are continuous, so the pointer from the block before the low buddy can be changed to the block after the high
	// buddies to remove both buddies at once
	page **lowest_buddy = find_page_slot(order, page::get_from_pfn(lowest_adr));
	*lowest_buddy = (*lowest_buddy)->next_free_->next_free_;
	// Insert the newly formed block into the high order
	insert_free_block(order + 1, page::get_from_pfn(lowest_adr));
}

/// @brief Finds the slot for a page, ie the pointer from a previous block that points to this page
/// This is used as a helper function
/// @param order The order to search in
/// @param target_page The page to find
/// @return The pointer to the pointer to the target page, from the page before it
page **page_allocator_buddy::find_page_slot(int order, page &target_page)
{
	// This was modified from the provided page
	//  assert order in range
	assert(order >= 0 && order <= LastOrder);

	// assert block_start aligned to order
	assert(block_aligned(order, target_page.pfn()));

	page *target = &target_page;
	page **candidate_slot = &free_list_[order];
	while (*candidate_slot && *candidate_slot != target) {
		candidate_slot = &((*candidate_slot)->next_free_);
	}

	// assert candidate block exists
	assert(*candidate_slot == target);
	return candidate_slot;
}

/// @brief Allocate a block of pages of an particular order
/// @param order The order to allocate from
/// @param flags idk???
/// @return the page at the start of the block that was allocated
page *page_allocator_buddy::allocate_pages(int order, page_allocation_flags flags)
{
	// This is incase we need to split a higher block to get the correct pages
	// Increment cur_order until we get to an order with pages in it
	int cur_order = order;
	while (cur_order < LastOrder + 1) {
		if (free_list_[cur_order]) {
			break;
		} else {
			cur_order++;
		}
	}
	if (!free_list_[cur_order])
		return nullptr;
	// split the block down
	while (cur_order > order) {
		split_block(cur_order, *free_list_[cur_order]);
		cur_order--;
	}

	if (free_list_[order]) {
		page *p = free_list_[order];
		free_list_[order] = p->next_free_;
		return p;
	}
	return nullptr;
}

/// @brief Free a block, ie insert it back into the list
/// @param block_start The page at the start of the block
/// @param order the order to free into
void page_allocator_buddy::free_pages(page &block_start, int order) { insert_free_block(order, block_start); }
