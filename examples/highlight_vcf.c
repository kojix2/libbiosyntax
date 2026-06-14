/* SPDX-License-Identifier: GPL-3.0-only */
#include "biosyntax.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    const char *line = "chr1\t42\trs1\tA\tT\t99\tPASS\tDP=10;AF=0.5\n";
    biosyn_span_t spans[64];
    uint64_t n = biosyn_highlight_line(
        BIOSYN_FORMAT_VCF,
        line,
        strlen(line),
        0,
        spans,
        sizeof(spans) / sizeof(spans[0])
    );

    for (uint64_t i = 0; i < n && i < sizeof(spans) / sizeof(spans[0]); i++) {
        printf("%llu\t%llu\t%s\n",
               (unsigned long long)spans[i].start,
               (unsigned long long)spans[i].length,
               biosyn_class_name(spans[i].class_id));
    }
    return 0;
}
