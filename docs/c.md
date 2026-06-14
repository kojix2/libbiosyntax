# C guide

The core library is `include/biosyntax.h` plus `src/biosyntax.c`. You can either
copy those two files into another project, compile them directly, or build the
static/shared libraries for linking and FFI.

## Direct use

Compile the implementation with your program:

```sh
cc -Iinclude your_program.c src/biosyntax.c -o your_program
```

No runtime data files are required.

`libbiosyntax` does not perform IO. Read files, sockets, gzip streams, or HTSlib
objects in your own code and pass one line at a time to the highlighter. BAM and
CRAM should be decoded to SAM text; BCF should be decoded to VCF text.

## Makefile builds

Using the Makefile:

```sh
make
make test
```

Build outputs:

```text
build/libbiosyntax.so
build/libbiosyntax.a
build/test_biosyntax
build/bcat
```

`make examples` builds `build/bcat`, a small POSIX C plain-text ANSI-rendering
example. The HTSlib-backed `biocat` example is built separately with
`make biocat`.

Install the header and libraries:

```sh
make install PREFIX=/usr/local
```

This installs:

```text
include/biosyntax.h
lib/libbiosyntax.so
lib/libbiosyntax.a
```

## Theme boundary

`libbiosyntax` exposes built-in class metadata and lets callers pass
`biosyn_ansi_style_t` overrides to the ANSI renderer. It deliberately does not
load external theme files.

Theme file loading belongs in the application or language binding layer, where
JSON, XML, TOML, YAML, editor-specific formats, paths, encodings, and detailed
errors are much easier to handle. A binding can load any theme format it likes,
map class names with `biosyn_class_info()`, and pass selected ANSI styles to
`biosyn_render_ansi_line_with_styles()`.

## Minimal example

```c
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
        64
    );

    for (uint64_t i = 0; i < n && i < 64; i++) {
        printf("%llu %llu %s\n",
               (unsigned long long)spans[i].start,
               (unsigned long long)spans[i].length,
               biosyn_class_name(spans[i].class_id));
    }
    return 0;
}
```

A complete sample is available at `examples/highlight_vcf.c`:

```sh
cc -Iinclude examples/highlight_vcf.c src/biosyntax.c -o highlight_vcf
./highlight_vcf
```

## Stateful highlighting

Use the stateful API for FASTQ and WIG streams:

```c
biosyn_state_t st;
biosyn_span_t spans[128];

biosyn_state_init(&st, BIOSYN_FORMAT_FASTQ);

uint64_t n = biosyn_highlight_next_line(&st, line, len, spans, 128);
```

For FFI use, heap-managed state is also available:

```c
biosyn_state_t *st = biosyn_state_new(BIOSYN_FORMAT_FASTQ);
/* ... */
biosyn_state_free(st);
```

If the span buffer is too small, `biosyn_highlight_next_line()` returns the
required count without advancing the state, so you can allocate a larger buffer
and retry the same line. State advances only when the returned count is less
than or equal to the span capacity you provided.

Stateless calls are reentrant. A `biosyn_state_t` is mutable and should be used
by one thread at a time.

## Reflection for bindings

Bindings can discover the built-in format and token-class tables at runtime:

```c
biosyn_format_info_t fmt;
if (biosyn_format_info(BIOSYN_FORMAT_FASTQ, &fmt) && fmt.stateful) {
    /* use biosyn_state_new() for streaming */
}

biosyn_class_info_t cls;
if (biosyn_class_info(BIOSYN_CLASS_NT_A, &cls)) {
    puts(cls.name);
}
```

The returned strings are owned by `libbiosyntax` and remain valid for the
process lifetime.

See [api.md](api.md) for the full public API.
