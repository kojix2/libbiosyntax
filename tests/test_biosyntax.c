/* SPDX-License-Identifier: GPL-3.0-only */
#include "biosyntax.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int has_class(const biosyn_span_t *s, size_t n, biosyn_class_t cls) {
    size_t i;
    for (i = 0; i < n; i++) if (s[i].class_id == cls) return 1;
    return 0;
}

static void test_guess(void) {
    assert(biosyn_guess_format_from_path("x.fa") == BIOSYN_FORMAT_FASTA);
    assert(biosyn_guess_format_from_path("x.fastq.gz") == BIOSYN_FORMAT_FASTQ);
    assert(biosyn_guess_format_from_path("x.bam") == BIOSYN_FORMAT_SAM);
    assert(biosyn_guess_format_from_path("x.cram") == BIOSYN_FORMAT_SAM);
    assert(biosyn_guess_format_from_path("x.bcf") == BIOSYN_FORMAT_VCF);
    assert(biosyn_guess_format_from_path("x.vcf.bgz") == BIOSYN_FORMAT_VCF);
    assert(biosyn_guess_format_from_path("x.flagstat") == BIOSYN_FORMAT_FLAGSTAT);
    assert(biosyn_format_from_name("fasta-nt") == BIOSYN_FORMAT_FASTA_NT);
    assert(biosyn_format_from_name("fasta-hc") == BIOSYN_FORMAT_FASTA_HC);
    assert(biosyn_format_from_name("fasta-zappo") == BIOSYN_FORMAT_FASTA_ZAPPO);
    assert(biosyn_format_from_name("fasta-orf") == BIOSYN_FORMAT_FASTA_ORF);
    assert(biosyn_guess_format_from_path("x.faa") == BIOSYN_FORMAT_FASTA_CLUSTAL);
    assert(biosyn_guess_format_from_path("x.fastaa") == BIOSYN_FORMAT_FASTA_CLUSTAL);
    assert(biosyn_guess_format_from_path("x.fas") == BIOSYN_FORMAT_FASTA_HC);
    assert(biosyn_guess_format_from_path("x.mfa") == BIOSYN_FORMAT_FASTA_HC);
    assert(biosyn_guess_format_from_path("x.fastahydro") == BIOSYN_FORMAT_FASTA_HYDRO);
    assert(biosyn_guess_format_from_path("x.fastaylor") == BIOSYN_FORMAT_FASTA_TAYLOR);
    assert(biosyn_guess_format_from_path("x.fastza") == BIOSYN_FORMAT_FASTA_ZAPPO);
    assert(strcmp(biosyn_format_name(BIOSYN_FORMAT_FASTA_ORF), "fasta-orf") == 0);
}

static void test_fasta(void) {
    biosyn_span_t spans[128];
    size_t n;
    n = biosyn_highlight_line(BIOSYN_FORMAT_FASTA, ">seq1\n", 6, 0, spans, 128);
    assert(n == 1 && spans[0].class_id == BIOSYN_CLASS_HEADER);
    { const char *seq = "ACGTURYSWKMBDHVNX-\n"; n = biosyn_highlight_line(BIOSYN_FORMAT_FASTA_NT, seq, strlen(seq), 1, spans, 128); }
    assert(has_class(spans, n, BIOSYN_CLASS_NT_A));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_C));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_G));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_T));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_U));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_R));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_Y));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_S));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_W));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_M));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_K));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_D));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_B));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_V));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_H));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_N));
    assert(has_class(spans, n, BIOSYN_CLASS_NT_X));
    assert(has_class(spans, n, BIOSYN_CLASS_GAP));

    { const char *seq = "ACGTN-\n"; n = biosyn_highlight_line(BIOSYN_FORMAT_FASTA_HC, seq, strlen(seq), 0, spans, 128); }
    assert(has_class(spans, n, BIOSYN_CLASS_HC_A));
    assert(has_class(spans, n, BIOSYN_CLASS_HC_GAP));

    { const char *seq = "ARNDCQEGHILKMFPSTWYVBXZ-\n"; n = biosyn_highlight_line(BIOSYN_FORMAT_FASTA_CLUSTAL, seq, strlen(seq), 0, spans, 128); }
    assert(has_class(spans, n, BIOSYN_CLASS_AA_A));
    assert(has_class(spans, n, BIOSYN_CLASS_AA_R));
    assert(has_class(spans, n, BIOSYN_CLASS_AA_Z));
    assert(has_class(spans, n, BIOSYN_CLASS_GAP));

    { const char *seq = "ARZ\n"; n = biosyn_highlight_line(BIOSYN_FORMAT_FASTA_ZAPPO, seq, strlen(seq), 0, spans, 128); }
    assert(has_class(spans, n, BIOSYN_CLASS_ZAPPO_A));
    assert(has_class(spans, n, BIOSYN_CLASS_ZAPPO_R));
    assert(has_class(spans, n, BIOSYN_CLASS_ZAPPO_Z));

    { const char *seq = "CCCATGAAATAGCCC\n"; n = biosyn_highlight_line(BIOSYN_FORMAT_FASTA_ORF, seq, strlen(seq), 0, spans, 128); }
    assert(has_class(spans, n, BIOSYN_CLASS_ORF_START));
    assert(has_class(spans, n, BIOSYN_CLASS_ORF_CODING));
    assert(has_class(spans, n, BIOSYN_CLASS_ORF_STOP));
}

static void test_fastq_state(void) {
    biosyn_state_t st;
    biosyn_span_t spans[32];
    biosyn_span_t one_span[1];
    size_t n;
    biosyn_state_init(&st, BIOSYN_FORMAT_FASTQ);
    n = biosyn_highlight_next_line(&st, "@r1\n", 4, spans, 32);
    assert(n == 1 && spans[0].class_id == BIOSYN_CLASS_HEADER);
    n = biosyn_highlight_next_line(&st, "ACGT\n", 5, spans, 32);
    assert(has_class(spans, n, BIOSYN_CLASS_NT_A));
    n = biosyn_highlight_next_line(&st, "+\n", 2, spans, 32);
    assert(n == 1 && spans[0].class_id == BIOSYN_CLASS_HEADER);
    n = biosyn_highlight_next_line(&st, "IIII\n", 5, spans, 32);
    assert(has_class(spans, n, BIOSYN_CLASS_QUAL_10I));

    biosyn_state_init(&st, BIOSYN_FORMAT_FASTQ);
    n = biosyn_highlight_next_line(&st, "@r2\n", 4, spans, 32);
    assert(n == 1 && st.line_no == 1);
    n = biosyn_highlight_next_line(&st, "ACGTN-\n", 7, one_span, 1);
    assert(n > 1);
    assert(st.line_no == 1);
    n = biosyn_highlight_next_line(&st, "ACGTN-\n", 7, spans, 32);
    assert(st.line_no == 2);
    assert(has_class(spans, n, BIOSYN_CLASS_NT_A));
}

static void test_sam(void) {
    biosyn_span_t spans[256];
    size_t n;
    const char *hdr = "@HD\tVN:1.6\tSO:coordinate\n";
    n = biosyn_highlight_line(BIOSYN_FORMAT_SAM, hdr, strlen(hdr), 0, spans, 256);
    assert(has_class(spans, n, BIOSYN_CLASS_HEADER));
    assert(has_class(spans, n, BIOSYN_CLASS_GOOD));
    assert(has_class(spans, n, BIOSYN_CLASS_NUMBER));

    hdr = "@SQ\tSN:chr1\tLN:248956422\tUR:https://example.invalid/ref.fa\n";
    n = biosyn_highlight_line(BIOSYN_FORMAT_SAM, hdr, strlen(hdr), 0, spans, 256);
    assert(has_class(spans, n, BIOSYN_CLASS_CHROM));
    assert(has_class(spans, n, BIOSYN_CLASS_URL));

    {
        const char *sam = "r001\t99\tchr1\t7\t30\t8M1I4M\t=\t37\t39\tACGTRYK\tIIIIIII\tNM:i:1\tMD:Z:8A4\n";
        n = biosyn_highlight_line(BIOSYN_FORMAT_SAM, sam, strlen(sam), 0, spans, 256);
        assert(has_class(spans, n, BIOSYN_CLASS_CHROM));
        assert(has_class(spans, n, BIOSYN_CLASS_CIGAR_INSERTION));
        assert(has_class(spans, n, BIOSYN_CLASS_KEYWORD3));
        assert(has_class(spans, n, BIOSYN_CLASS_NT_R));
        assert(has_class(spans, n, BIOSYN_CLASS_NT_Y));
        assert(has_class(spans, n, BIOSYN_CLASS_NT_K));
    }
}

static void test_vcf_gtf_bed(void) {
    biosyn_span_t spans[512];
    size_t n;
    const char *meta = "##INFO=<ID=DP,Number=1,Type=Integer,Description=\"Depth\">\n";
    n = biosyn_highlight_line(BIOSYN_FORMAT_VCF, meta, strlen(meta), 0, spans, 512);
    assert(has_class(spans, n, BIOSYN_CLASS_KEYWORD6));
    assert(has_class(spans, n, BIOSYN_CLASS_NUMBER));
    assert(has_class(spans, n, BIOSYN_CLASS_QUOTED_STRING));

    meta = "##contig=<ID=chr1,length=248956422,URL=https://example.invalid/chr1>\n";
    n = biosyn_highlight_line(BIOSYN_FORMAT_VCF, meta, strlen(meta), 0, spans, 512);
    assert(has_class(spans, n, BIOSYN_CLASS_CHROM));
    assert(has_class(spans, n, BIOSYN_CLASS_URL));

    {
        const char *vcf = "chr1\t42\trs1\tA\tR,<DEL>\t99\tPASS\tDP=10;AF=0.5\tGT:DP\t0/1:10\t./.:.\n";
        n = biosyn_highlight_line(BIOSYN_FORMAT_VCF, vcf, strlen(vcf), 0, spans, 512);
        assert(has_class(spans, n, BIOSYN_CLASS_GOOD));
        assert(has_class(spans, n, BIOSYN_CLASS_KEYWORD6));
        assert(has_class(spans, n, BIOSYN_CLASS_NT_R));
        assert(has_class(spans, n, BIOSYN_CLASS_KEYWORD3));
        assert(has_class(spans, n, BIOSYN_CLASS_NULL));
        assert(has_class(spans, n, BIOSYN_CLASS_NUMBER));
    }

    {
        const char *gtf = "chr1\tsrc\tstart_codon\t1\t9\t.\t+\t.\tgene_id \"g1\"; transcript_id \"t1\"; score \"1.5\";\n";
        n = biosyn_highlight_line(BIOSYN_FORMAT_GTF, gtf, strlen(gtf), 0, spans, 512);
        assert(has_class(spans, n, BIOSYN_CLASS_SOFTWARE));
        assert(has_class(spans, n, BIOSYN_CLASS_FEATURE_START_CODON));
        assert(has_class(spans, n, BIOSYN_CLASS_FEATURE_GENE));
        assert(has_class(spans, n, BIOSYN_CLASS_FEATURE_TRANSCRIPT));
        assert(has_class(spans, n, BIOSYN_CLASS_STRAND_PLUS));
    }

    {
        const char *bed = "chr1\t10\t20\tfeature\t750\t-\n";
        n = biosyn_highlight_line(BIOSYN_FORMAT_BED, bed, strlen(bed), 0, spans, 512);
        assert(has_class(spans, n, BIOSYN_CLASS_GRADBW_8));
        assert(has_class(spans, n, BIOSYN_CLASS_STRAND_MINUS));
    }
}

static void test_faidx_flagstat_wig_pdb(void) {
    biosyn_span_t spans[512];
    size_t n;
    biosyn_state_t st;
    const char *fai = "chr1\t248956422\t52\t60\t61\n";
    n = biosyn_highlight_line(BIOSYN_FORMAT_FAIDX, fai, strlen(fai), 0, spans, 512);
    assert(has_class(spans, n, BIOSYN_CLASS_SEQUENCE_LENGTH));
    assert(has_class(spans, n, BIOSYN_CLASS_FILE_OFFSET));
    assert(has_class(spans, n, BIOSYN_CLASS_LINE_BASES));
    assert(has_class(spans, n, BIOSYN_CLASS_LINE_WIDTH));

    {
        const char *flag = "100 + 2 in total (QC-passed reads + QC-failed reads)\n";
        n = biosyn_highlight_line(BIOSYN_FORMAT_FLAGSTAT, flag, strlen(flag), 0, spans, 512);
        assert(has_class(spans, n, BIOSYN_CLASS_QC_PASSED));
        assert(has_class(spans, n, BIOSYN_CLASS_QC_FAILED));
    }

    biosyn_state_init(&st, BIOSYN_FORMAT_WIG);
    n = biosyn_highlight_next_line(&st, "fixedStep chrom=chr1 start=1 step=10\n", 37, spans, 512);
    assert(has_class(spans, n, BIOSYN_CLASS_HEADER));
    assert(has_class(spans, n, BIOSYN_CLASS_CHROM));
    n = biosyn_highlight_next_line(&st, "123\n", 4, spans, 512);
    assert(!has_class(spans, n, BIOSYN_CLASS_POSITION));
    assert(has_class(spans, n, BIOSYN_CLASS_GRAD_2));

    biosyn_state_init(&st, BIOSYN_FORMAT_WIG);
    n = biosyn_highlight_next_line(&st, "variableStep chrom=chr1 span=5\n", 30, spans, 512);
    assert(has_class(spans, n, BIOSYN_CLASS_HEADER));
    n = biosyn_highlight_next_line(&st, "50 900\n", 7, spans, 512);
    assert(has_class(spans, n, BIOSYN_CLASS_POSITION));
    assert(has_class(spans, n, BIOSYN_CLASS_GRAD_10));

    {
        const char *pdb = "ATOM      1  N   MET A   1      11.104  13.207   9.112  1.00 20.00           N\n";
        n = biosyn_highlight_line(BIOSYN_FORMAT_PDB, pdb, strlen(pdb), 0, spans, 512);
        assert(has_class(spans, n, BIOSYN_CLASS_AMINO_HYDRO));
        assert(has_class(spans, n, BIOSYN_CLASS_POSITION));
        assert(has_class(spans, n, BIOSYN_CLASS_KEYWORD6));
    }
}

static void test_theme_api(void) {
    biosyn_class_info_t info;
    biosyn_format_info_t fmt_info;
    assert(strcmp(biosyn_version(), BIOSYN_VERSION_STRING) == 0);
    assert(biosyn_abi_version() == BIOSYN_ABI_VERSION);
    assert(strcmp(biosyn_class_name(BIOSYN_CLASS_NT_R), "nt_r") == 0);
    assert(strcmp(biosyn_class_name(BIOSYN_CLASS_ZAPPO_A), "zappo_a") == 0);
    assert(strcmp(biosyn_class_name(BIOSYN_CLASS_ORF_START), "orf_start") == 0);
    assert(biosyn_class_default_foreground(BIOSYN_CLASS_NT_A)[0] == '#');
    assert(biosyn_class_ansi_sgr(BIOSYN_CLASS_NT_A)[0] != '\0');
    assert(biosyn_class_count() == BIOSYN_CLASS__COUNT);
    assert(biosyn_class_info(BIOSYN_CLASS_NT_A, &info));
    assert(strcmp(info.name, "nt_a") == 0);
    assert(strcmp(info.foreground, biosyn_class_default_foreground(BIOSYN_CLASS_NT_A)) == 0);
    assert(!biosyn_class_info(BIOSYN_CLASS__COUNT, &info));

    assert(biosyn_format_count() == BIOSYN_FORMAT__COUNT);
    assert(biosyn_format_info(BIOSYN_FORMAT_VCF, &fmt_info));
    assert(strcmp(fmt_info.name, "vcf") == 0);
    assert(!fmt_info.stateful);
    assert(biosyn_format_info(BIOSYN_FORMAT_FASTQ, &fmt_info));
    assert(strcmp(fmt_info.name, "fastq") == 0);
    assert(fmt_info.stateful);
    assert(!biosyn_format_info(BIOSYN_FORMAT__COUNT, &fmt_info));
}

static void test_ansi_api(void) {
    const char *line = "chr1\t42\trs1\tA\tT\t99\tPASS\n";
    biosyn_span_t spans[64];
    biosyn_ansi_style_t styles[1];
    char out[256];
    uint64_t n, required;
    n = biosyn_highlight_line(BIOSYN_FORMAT_VCF, line, (uint64_t)strlen(line), 0, spans, 64);
    assert(n > 0);
    required = biosyn_render_ansi_line(line, (uint64_t)strlen(line), spans, n, out, sizeof(out));
    assert(required > strlen(line));
    assert(strstr(out, "\033[") != NULL);
    styles[0].class_id = BIOSYN_CLASS_GOOD;
    styles[0].reserved = 0;
    styles[0].ansi_sgr = "1;31";
    required = biosyn_render_ansi_line_with_styles(line, (uint64_t)strlen(line), spans, n, styles, 1, out, sizeof(out));
    assert(required > strlen(line));
    assert(strstr(out, "\033[1;31mPASS") != NULL);

    spans[0].start = UINT64_MAX;
    spans[0].length = UINT64_MAX;
    spans[0].class_id = BIOSYN_CLASS_GOOD;
    spans[0].reserved = 0;
    required = biosyn_render_ansi_line(line, (uint64_t)strlen(line), spans, 1, out, sizeof(out));
    assert(required == strlen(line));
    assert(strstr(out, "\033[") == NULL);
}

int main(void) {
    test_guess();
    test_fasta();
    test_fastq_state();
    test_sam();
    test_vcf_gtf_bed();
    test_faidx_flagstat_wig_pdb();
    test_theme_api();
    test_ansi_api();
    puts("ok");
    return 0;
}
