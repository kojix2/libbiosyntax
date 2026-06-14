# User guide

This guide covers supported formats, the highlighting model, class metadata, and
example CLIs.

## Supported formats

- FASTA
- FASTA nucleotide scheme: `fasta-nt`
- FASTA high-contrast nucleotide scheme: `fasta-hc`
- FASTA Clustal amino-acid scheme: `fasta-clustal`
- FASTA hydrophobicity scheme: `fasta-hydro` / `fasta-hydrophobicity`
- FASTA Taylor amino-acid scheme: `fasta-taylor`
- FASTA Zappo amino-acid scheme: `fasta-zappo`
- FASTA ORF mode: `fasta-orf`
- FASTQ
- SAM
- VCF
- BED
- GTF
- GFF
- PDB
- CLUSTAL alignment
- FASTA index / `.fai`
- SAM flagstat
- WIG / bedGraph-like lines

`biosyn_guess_format_from_path()` recognizes BioSyntax-style FASTA scheme
extensions such as `.faa`/`.fastaa`, `.fas`/`.mfa`, `.fastahydro`,
`.fastaylor`, and `.fastza`.

## Highlighting details

`libbiosyntax` returns semantic token classes rather than final colors only.
Important token families include:

- individual nucleotide and IUPAC ambiguity classes: `nt_a`, `nt_c`, `nt_g`,
  `nt_t`, `nt_u`, `nt_n`, `nt_r`, `nt_y`, `nt_s`, `nt_w`, `nt_m`, `nt_k`,
  `nt_d`, `nt_b`, `nt_v`, `nt_h`, and `nt_x`;
- high-contrast nucleotide classes for `fasta-hc`;
- amino-acid classes for Clustal, Zappo, Taylor, and hydrophobicity schemes;
- ORF start, coding-region, and stop-codon classes;
- SAM CIGAR and quality classes;
- SAM header tags for `@HD`, `@SQ`, `@RG`, `@PG`, and comments;
- VCF meta-line attributes, INFO attributes, FORMAT fields, sample delimiters,
  missing genotype values, numbers, quoted strings, URLs, and symbolic alleles;
- BED/GTF/GFF feature, strand, score-gradient, and attribute classes;
- PDB record, residue, chain, coordinate, occupancy, temperature-factor, and
  element classes;
- FASTA index column classes;
- SAM flagstat passed/failed counts and percentages;
- WIG fixedStep and variableStep state-aware value classes.

## Class metadata and default colors

The C library includes built-in BioSyntax-inspired class metadata. No external
theme file is required at runtime. Applications can retrieve class names,
scopes, ANSI SGR fragments, foreground colors, background colors, and font style
hints:

```c
const char *biosyn_class_name(biosyn_class_t class_id);
const char *biosyn_class_scope(biosyn_class_t class_id);
const char *biosyn_class_ansi_sgr(biosyn_class_t class_id);
const char *biosyn_class_default_foreground(biosyn_class_t class_id);
const char *biosyn_class_default_background(biosyn_class_t class_id);
const char *biosyn_class_default_font_style(biosyn_class_t class_id);

biosyn_class_info_t info;
int ok = biosyn_class_info(BIOSYN_CLASS_NT_A, &info);
```

Use `biosyn_class_count()` to iterate over all token classes. The metadata API is
the supported way to build editor themes, GUI styles, or binding-level lookup
tables.

## CLI examples

`bcat` is the simplest ANSI-rendering example. It is a small POSIX C program
for plain text input:

```sh
make examples
build/bcat -f vcf sample.vcf
build/bcat -f fasta-zappo proteins.fa
build/bcat sample.fa
```

`biocat` is the HTSlib-backed example for compressed and binary bioinformatics
files:

```sh
make biocat
build/biocat sample.vcf.gz
build/biocat sample.bam
```

`biocat` reads input through HTSlib, including `.gz`/`.bgz` text, BAM, CRAM, and
BCF. Binary alignment and variant files are converted to SAM or VCF text in the
CLI before highlighting.

Build both examples with:

```sh
make examples
make biocat
```

Only `biocat` is linked with HTSlib. Install HTSlib development headers and
library before running `make biocat`. The core library itself has no HTSlib
dependency.
