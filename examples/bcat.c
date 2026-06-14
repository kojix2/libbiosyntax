/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L

#include "biosyntax.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s [-f format] [file]\n", argv0);
    fprintf(stderr, "reads plain text only; use biocat for gzip/bgzip, BAM, CRAM, and BCF\n");
}

static int render_line(biosyn_state_t *state, const char *line, uint64_t len) {
    biosyn_span_t stack_spans[256];
    biosyn_span_t *spans = stack_spans;
    uint64_t span_count;
    uint64_t out_len;
    char *out;

    span_count = biosyn_highlight_next_line(state, line, len, spans, 256);
    if (span_count > 256) {
        if (span_count > (uint64_t)SIZE_MAX / sizeof(*spans)) return 0;
        spans = (biosyn_span_t *)malloc((size_t)span_count * sizeof(*spans));
        if (!spans) return 0;
        span_count = biosyn_highlight_next_line(state, line, len, spans, span_count);
    }

    out_len = biosyn_render_ansi_line(line, len, spans, span_count, NULL, 0);
    if (out_len > (uint64_t)SIZE_MAX - 1u) {
        if (spans != stack_spans) free(spans);
        return 0;
    }
    out = (char *)malloc((size_t)out_len + 1u);
    if (!out) {
        if (spans != stack_spans) free(spans);
        return 0;
    }
    biosyn_render_ansi_line(line, len, spans, span_count, out, out_len + 1u);
    fputs(out, stdout);

    free(out);
    if (spans != stack_spans) free(spans);
    return 1;
}

int main(int argc, char **argv) {
    const char *format_name = NULL;
    const char *path = NULL;
    biosyn_format_t format;
    biosyn_state_t state;
    FILE *fp = stdin;
    char *line = NULL;
    size_t cap = 0;
    int i, rc = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            format_name = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            path = argv[i];
        }
    }

    format = format_name ? biosyn_format_from_name(format_name)
                         : biosyn_guess_format_from_path(path ? path : "");
    if (format == BIOSYN_FORMAT_UNKNOWN) {
        fprintf(stderr, "bcat: unknown format; pass -f FORMAT\n");
        return 2;
    }

    if (path) {
        fp = fopen(path, "rb");
        if (!fp) {
            perror(path);
            return 2;
        }
    }

    biosyn_state_init(&state, format);
    for (;;) {
        ssize_t len = getline(&line, &cap, fp);
        if (len < 0) break;
        if (!render_line(&state, line, (uint64_t)len)) {
            fprintf(stderr, "bcat: failed to render input\n");
            rc = 1;
            break;
        }
    }
    if (ferror(fp)) {
        fprintf(stderr, "bcat: failed to read input\n");
        rc = 1;
    }

    free(line);
    if (fp != stdin && fclose(fp) != 0) {
        perror(path);
        rc = 2;
    }
    return rc;
}
