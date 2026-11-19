#include <assert.h>
#include <stdio.h>
#include "paper/paper.h"

int main(void) {
    paper_dimensions_t d;

    assert(paper_get_standard_size(PAPER_SIZE_A4, &d) == PAPER_OK);
    assert(d.width_mm == 210.0 && d.height_mm == 297.0);

    assert(paper_get_standard_size(PAPER_SIZE_A0, &d) == PAPER_OK);
    assert(d.width_mm == 841.0 && d.height_mm == 1189.0);

    assert(paper_get_standard_size(PAPER_SIZE_US_LETTER, &d) == PAPER_OK);
    assert(d.width_mm == 215.9 && d.height_mm == 279.4);

    assert(paper_get_standard_size(PAPER_SIZE_US_LEGAL, &d) == PAPER_OK);
    assert(d.width_mm == 215.9 && d.height_mm == 355.6);

    assert(paper_get_standard_size(PAPER_SIZE_US_EXECUTIVE, &d) == PAPER_OK);
    assert(d.width_mm == 184.15 && d.height_mm == 266.7);

    /* ISO B5 != JIS B5 (deliberately distinct). */
    paper_dimensions_t b5_iso, b5_jis;
    assert(paper_get_standard_size(PAPER_SIZE_B5, &b5_iso) == PAPER_OK);
    assert(paper_get_standard_size(PAPER_SIZE_JIS_B5, &b5_jis) == PAPER_OK);
    assert(b5_iso.width_mm == 176.0 && b5_iso.height_mm == 250.0);
    assert(b5_jis.width_mm == 182.0 && b5_jis.height_mm == 257.0);
    assert(b5_iso.width_mm != b5_jis.width_mm);

    assert(paper_get_standard_size(PAPER_SIZE_CUSTOM, &d) == PAPER_ERR_INVALID_ARGUMENT);
    assert(paper_get_standard_size((paper_size_id_t)PAPER_SIZE_COUNT, &d) == PAPER_ERR_INVALID_ARGUMENT);

    assert(paper_get_size_name(PAPER_SIZE_A4) != NULL);
    assert(paper_get_size_name(PAPER_SIZE_COUNT) == NULL);

    printf("paper_size OK\n");
    return 0;
}
