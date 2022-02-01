#include "os.h"

#define VALID 1
#define INVALID 0

void page_table_update(uint64_t, uint64_t, uint64_t);
uint64_t page_table_query(uint64_t, uint64_t);
int get_current_pte(uint64_t, int);

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn) {
    int curr_level = 0; //5 levels [0,1,2,3,4] of pt exists in this architecture.
    uint64_t* curr_level_pt; //Page Table in each level.
    int curr_pte; //relevant Page Table Entry in each Page Table.

    curr_level_pt = (uint64_t*)phys_to_virt(pt<<12); //gets Page Table root (level 0) by adding a 12-zero-bit offest (keeping 4K aligned attribute, as defined by a true physical frame's size).
    
    //this code suffers from code duplication, but in doing so it gets optimal. :)

    if (ppn == NO_MAPPING) { //vpn’s mapping (if it exists) should be destroyed.
        while (curr_level < 4) { //walking trough the trie (Multi-level Page Table), returning as soon as an invalid bit is found.
            curr_pte = get_current_pte(vpn,curr_level);
            if ((curr_level_pt[curr_pte]&1) == INVALID) { //checks if a new allocation is needed.
                return;
            }
            curr_level_pt = (uint64_t*)phys_to_virt(curr_level_pt[curr_pte] - VALID); //getting the pointer to next level Page Table.
            curr_level++;
        }
        curr_pte = get_current_pte(vpn,curr_level); //current level is 4 (last one).
        curr_level_pt[curr_pte] = ppn<<12; //valid bit is 0 in the final PTE, destoring the mapping.
    }
    else { //ppn specifies the physical page number that vpn should be mapped to.
        while (curr_level < 4) { //walking trough the trie (Multi-level Page Table), creating new allocations if needed.
            curr_pte = get_current_pte(vpn,curr_level);
            if ((curr_level_pt[curr_pte]&1) == INVALID) { //checks if a new allocation is needed.
                curr_level_pt[curr_pte] = (alloc_page_frame()<<12) + VALID; //adding a 12-zero-bit offest and a positive valid bit.
            }
            curr_level_pt = (uint64_t*)phys_to_virt(curr_level_pt[curr_pte] - VALID); //getting the pointer to next level Page Table.
            curr_level++;
        }
        curr_pte = get_current_pte(vpn,curr_level); //current level is 4 (last one).
        curr_level_pt[curr_pte] = (ppn<<12) + VALID; //updates last PTE with the wanted PPN and marks it VALID.
    }
    return;
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn) {
    int curr_level = 0; //5 levels [0,1,2,3,4] of pt exists in this architecture.
    uint64_t* curr_level_pt; //Page Table in each level.
    int curr_pte; //relevant Page Table Entry in each Page Table.
    uint64_t ppn; //the Physical Page Number that vpn is mapped to.

    curr_level_pt = (uint64_t*)phys_to_virt(pt<<12); //gets Page Table root (level 0) by adding a 12-zero-bit offest (keeping 4K aligned attribute, as defined by a true physical frame's size).
    while (curr_level < 4) { //walking trough the trie (Multi-level Page Table), without allocating.
        curr_pte = get_current_pte(vpn,curr_level);
        if ((curr_level_pt[curr_pte]&1) == INVALID) {
            return NO_MAPPING;
        }
        curr_level_pt = (uint64_t*)phys_to_virt(curr_level_pt[curr_pte] - VALID); //getting the pointer to next level Page Table.
        curr_level++;
    }

    curr_pte = get_current_pte(vpn,curr_level); //current level is 4 (last one).
    if ((curr_level_pt[curr_pte]&1) == INVALID) {return NO_MAPPING;}
    else {
        ppn = curr_level_pt[curr_pte];
        return ppn>>12;
    }
}

int get_current_pte(uint64_t vpn, int curr_level) {
    //symbol in the trie takes 9 bits as we got 4096 bytes(Page size)/8 byts(PTE size is 64 bits) = 512 children for each node in the trie.
    //VPN "true" size without Offset (bits #0-12) and Sign Extend (bits #57-63) is 45 bits (64-12-7).
    //at each level we would like to take the higher bits of the "true" VPN, thus we would strat at 36 and go down in multiples of 9.
    //using the bitwise operator '&' with the number 0x1ff=111111111, we'll get the first 9 bits that are the symbol are looking for in the trie (the relevant PTE in the PT).
    return (vpn>>(36-curr_level*9)) & 0x1ff;
}