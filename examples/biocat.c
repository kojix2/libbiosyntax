/* SPDX-License-Identifier: GPL-3.0-only */
#include "biosyntax.h"

#include <htslib/hts.h>
#include <htslib/hts_log.h>
#include <htslib/kstring.h>
#include <htslib/sam.h>
#include <htslib/vcf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum input_kind {
    INPUT_TEXT,
    INPUT_SAM,
    INPUT_VCF
} input_kind_t;

typedef struct input_stream {
    input_kind_t kind;
    htsFile *fp;
    sam_hdr_t *sam_header;
    bam1_t *bam;
    bcf_hdr_t *bcf_header;
    bcf1_t *bcf;
    kstring_t line;
    const char *header_text;
    char *owned_header_text;
    size_t header_len;
    size_t header_pos;
} input_stream_t;

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s [-f format] [file]\n", argv0);
    fprintf(stderr, "formats: fasta fasta-nt fasta-hc fasta-clustal fasta-hydro fasta-taylor fasta-zappo fasta-orf fastq sam vcf bed gtf gff pdb clustal faidx flagstat wig\n");
    fprintf(stderr, "input is read with HTSlib, including gzip/bgzip text, BAM, CRAM, and BCF\n");
}

static int ends_ci(const char *s, const char *suffix) {
    size_t n, m, i;
    if (!s || !suffix) return 0;
    n = strlen(s);
    m = strlen(suffix);
    if (n < m) return 0;
    s += n - m;
    for (i = 0; i < m; i++) {
        unsigned char a = (unsigned char)s[i], b = (unsigned char)suffix[i];
        if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + 32);
        if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + 32);
        if (a != b) return 0;
    }
    return 1;
}

static int append_newline(kstring_t *line) {
    return line->l == 0 || line->s[line->l - 1] != '\n' ? kputc('\n', line) : 0;
}

static int next_header_line(input_stream_t *in, const char **line, size_t *len) {
    size_t start, end;
    if (!in->header_text || in->header_pos >= in->header_len) return 0;
    start = in->header_pos;
    end = start;
    while (end < in->header_len && in->header_text[end] != '\n') end++;
    ks_clear(&in->line);
    if (kputsn(in->header_text + start, end - start, &in->line) < 0) return -1;
    if (append_newline(&in->line) < 0) return -1;
    in->header_pos = end < in->header_len ? end + 1u : end;
    *line = in->line.s;
    *len = in->line.l;
    return 1;
}

static int open_input(const char *path, biosyn_format_t fmt, input_stream_t *in) {
    const char *filename = path ? path : "-";
    char *header_copy;
    memset(in, 0, sizeof(*in));
    in->kind = INPUT_TEXT;

    if (fmt == BIOSYN_FORMAT_SAM) {
        in->kind = INPUT_SAM;
        in->fp = sam_open(filename, "r");
        if (!in->fp) return 0;
        in->sam_header = sam_hdr_read(in->fp);
        in->bam = bam_init1();
        if (!in->sam_header || !in->bam) return 0;
        in->header_text = sam_hdr_str(in->sam_header);
        in->header_len = sam_hdr_length(in->sam_header);
        return 1;
    }

    if (fmt == BIOSYN_FORMAT_VCF) {
        in->kind = INPUT_VCF;
        in->fp = bcf_open(filename, "r");
        if (!in->fp) return 0;
        in->bcf_header = bcf_hdr_read(in->fp);
        in->bcf = bcf_init();
        if (!in->bcf_header || !in->bcf) return 0;
        if (bcf_hdr_format(in->bcf_header, 0, &in->line) < 0) return 0;
        header_copy = (char *)malloc(in->line.l + 1u);
        if (!header_copy) return 0;
        memcpy(header_copy, in->line.s, in->line.l + 1u);
        in->owned_header_text = header_copy;
        in->header_text = in->owned_header_text;
        in->header_len = in->line.l;
        in->header_pos = 0;
        ks_clear(&in->line);
        return 1;
    }

    in->fp = hts_open(filename, "r");
    return in->fp != NULL;
}

static int next_input_line(input_stream_t *in, const char **line, size_t *len) {
    int ret;
    ret = next_header_line(in, line, len);
    if (ret != 0) return ret;

    if (in->kind == INPUT_SAM) {
        ret = sam_read1(in->fp, in->sam_header, in->bam);
        if (ret < 0) return 0;
        ks_clear(&in->line);
        if (sam_format1(in->sam_header, in->bam, &in->line) < 0) return -1;
        if (append_newline(&in->line) < 0) return -1;
        *line = in->line.s;
        *len = in->line.l;
        return 1;
    }

    if (in->kind == INPUT_VCF) {
        ret = bcf_read(in->fp, in->bcf_header, in->bcf);
        if (ret < 0) return 0;
        ks_clear(&in->line);
        if (vcf_format(in->bcf_header, in->bcf, &in->line) < 0) return -1;
        if (append_newline(&in->line) < 0) return -1;
        *line = in->line.s;
        *len = in->line.l;
        return 1;
    }

    ret = hts_getline(in->fp, '\n', &in->line);
    if (ret < 0) return 0;
    if (append_newline(&in->line) < 0) return -1;
    *line = in->line.s;
    *len = in->line.l;
    return 1;
}

static int close_input(input_stream_t *in) {
    int rc = 0;
    if (!in) return 0;
    if (in->bam) bam_destroy1(in->bam);
    if (in->sam_header) sam_hdr_destroy(in->sam_header);
    if (in->bcf) bcf_destroy(in->bcf);
    if (in->bcf_header) bcf_hdr_destroy(in->bcf_header);
    if (in->fp) rc = hts_close(in->fp);
    free(in->owned_header_text);
    free(in->line.s);
    memset(in, 0, sizeof(*in));
    return rc;
}

int main(int argc, char **argv) {
    const char *format_name = NULL;
    const char *path = NULL;
    input_stream_t input;
    biosyn_format_t fmt;
    biosyn_state_t state;
    int i;

    hts_set_log_level(HTS_LOG_ERROR);

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

    fmt = format_name ? biosyn_format_from_name(format_name) : biosyn_guess_format_from_path(path ? path : "");
    if (fmt == BIOSYN_FORMAT_UNKNOWN && path && (ends_ci(path, ".bam") || ends_ci(path, ".cram"))) fmt = BIOSYN_FORMAT_SAM;
    if (fmt == BIOSYN_FORMAT_UNKNOWN && path && ends_ci(path, ".bcf")) fmt = BIOSYN_FORMAT_VCF;
    if (fmt == BIOSYN_FORMAT_UNKNOWN) {
        fprintf(stderr, "biocat: unknown format; pass -f FORMAT\n");
        return 2;
    }

    if (!open_input(path, fmt, &input)) {
        perror(path);
        return 2;
    }

    biosyn_state_init(&state, fmt);
    for (;;) {
        const char *line = NULL;
        size_t len = 0;
        biosyn_span_t stack_spans[256];
        biosyn_span_t *spans = stack_spans;
        uint64_t need;
        char *rendered;
        uint64_t rendered_need;
        int line_status;

        line_status = next_input_line(&input, &line, &len);
        if (line_status == 0) break;
        if (line_status < 0) { close_input(&input); return 1; }

        need = biosyn_highlight_next_line(&state, line, len, spans, 256);
        if (need > 256) {
            if (need > (uint64_t)SIZE_MAX / sizeof(*spans)) { close_input(&input); return 1; }
            spans = (biosyn_span_t *)malloc((size_t)need * sizeof(*spans));
            if (!spans) { close_input(&input); return 1; }
            need = biosyn_highlight_next_line(&state, line, len, spans, need);
        }
        rendered_need = biosyn_render_ansi_line(line, len, spans, need, NULL, 0);
        if (rendered_need > (uint64_t)SIZE_MAX - 1u) { if (spans != stack_spans) free(spans); close_input(&input); return 1; }
        rendered = (char *)malloc((size_t)rendered_need + 1u);
        if (!rendered) { if (spans != stack_spans) free(spans); close_input(&input); return 1; }
        biosyn_render_ansi_line(line, len, spans, need, rendered, rendered_need + 1);
        fputs(rendered, stdout);
        free(rendered);
        if (spans != stack_spans) free(spans);
    }

    if (close_input(&input) != 0) {
        fprintf(stderr, "biocat: input read failed\n");
        return 2;
    }
    return 0;
}
