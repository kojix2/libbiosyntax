# C API reference

This reference summarizes the public C API for `libbiosyntax`. Include
`biosyntax.h` and compile `src/biosyntax.c` with your program, or link with
`build/libbiosyntax.so` / `build/libbiosyntax.a`.

## Stateful API

FASTQ has a four-line record structure, so streaming code should use the
stateful API instead of relying only on `line_no % 4`. WIG also benefits from the
stateful API because `fixedStep` and `variableStep` data lines have different
column meanings.

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

If `biosyn_highlight_next_line()` needs more spans than `out_cap`, it returns
the required count and leaves `biosyn_state_t` unchanged. State advances only
when the returned count is less than or equal to `out_cap`, so callers can
allocate a larger span buffer and retry the same line.

## IO and threads

The C core does not open files, keep file handles, or own input buffers. Feed it
one already-read line at a time from your own IO layer.

Format IDs describe text syntax families. For example, BAM/CRAM should be
decoded to SAM text and BCF should be decoded to VCF text before highlighting.
The `bcat` example shows plain-text IO without extra dependencies; the `biocat`
example demonstrates the HTSlib boundary for compressed and binary inputs. The
library itself stays IO-free.

Stateless functions such as `biosyn_highlight_line()` and metadata lookups are
reentrant. A single `biosyn_state_t` is a mutable stream cursor; do not use the
same state object concurrently from multiple threads. Use one state per stream
or protect shared state with a lock.

## FFI notes

The public data path uses fixed-width counters so bindings do not need to map
`size_t`:

```c
uint64_t biosyn_highlight_line(
    biosyn_format_t format,
    const char *line,
    uint64_t len,
    uint64_t zero_based_line_no,
    biosyn_span_t *out,
    uint64_t out_cap
);
```

The ANSI helper follows the same convention:

```c
uint64_t biosyn_render_ansi_line(
    const char *line,
    uint64_t len,
    const biosyn_span_t *spans,
    uint64_t span_count,
    char *out,
    uint64_t out_cap
);
```

For theme switching, pass ANSI SGR overrides at render time:

```c
typedef struct biosyn_ansi_style {
    uint32_t class_id;
    uint32_t reserved;
    const char *ansi_sgr;
} biosyn_ansi_style_t;

uint64_t biosyn_render_ansi_line_with_styles(
    const char *line,
    uint64_t len,
    const biosyn_span_t *spans,
    uint64_t span_count,
    const biosyn_ansi_style_t *styles,
    uint64_t style_count,
    char *out,
    uint64_t out_cap
);
```

Theme files are intentionally outside the C core. Bindings or applications can
load any file format they like, map class names to class IDs with
`biosyn_class_info()`, and pass the selected styles to the renderer. This keeps
the core dependency-free while letting language bindings and editor integrations
use their stronger file-format tooling.

The public span ABI is:

```c
typedef struct biosyn_span {
    uint64_t start;
    uint64_t length;
    uint32_t class_id;
    uint32_t reserved;
} biosyn_span_t;
```

Strings returned through metadata APIs are static strings owned by the library.
Do not free them.

## Portability notes

The public ABI avoids plain `int` or `long` for data-bearing fields. Format IDs,
class IDs, spans, counters, and stream state use fixed-width integer types such
as `uint32_t` and `uint64_t`. Internally, the implementation still uses `size_t`
for local memory indexing and rejects public lengths or capacities that cannot
fit in the host process.

The structs are an in-process C ABI, not a file or network serialization format.
Endianness does not matter as long as bindings declare the fields with native C
layout and pass them to the same process. Do not persist raw struct bytes across
machines or architectures.

Span offsets and lengths are byte offsets into the input line. The tokenizer is
ASCII-oriented for syntax recognition; non-ASCII bytes are left as ordinary text
unless a format-specific rule classifies them otherwise.

## Introspection

Formats and token classes can be discovered at runtime:

```c
uint32_t biosyn_format_count(void);
int biosyn_format_info(biosyn_format_t format, biosyn_format_info_t *out);

uint32_t biosyn_class_count(void);
int biosyn_class_info(biosyn_class_t class_id, biosyn_class_info_t *out);
```

`biosyn_format_info_t.stateful` is non-zero for formats where streaming state
matters, currently FASTQ and WIG.

## Main C API

```c
uint32_t biosyn_abi_version(void);
const char *biosyn_version(void);

biosyn_format_t biosyn_format_from_name(const char *name);
biosyn_format_t biosyn_guess_format_from_path(const char *path_or_extension);
const char *biosyn_format_name(biosyn_format_t format);
uint32_t biosyn_format_count(void);
int biosyn_format_info(biosyn_format_t format, biosyn_format_info_t *out);

const char *biosyn_class_name(biosyn_class_t class_id);
const char *biosyn_class_scope(biosyn_class_t class_id);
const char *biosyn_class_ansi_sgr(biosyn_class_t class_id);
const char *biosyn_class_default_foreground(biosyn_class_t class_id);
const char *biosyn_class_default_background(biosyn_class_t class_id);
const char *biosyn_class_default_font_style(biosyn_class_t class_id);

uint32_t biosyn_class_count(void);
int biosyn_class_info(biosyn_class_t class_id, biosyn_class_info_t *out);

void biosyn_state_init(biosyn_state_t *state, biosyn_format_t format);
biosyn_state_t *biosyn_state_new(biosyn_format_t format);
void biosyn_state_free(biosyn_state_t *state);

uint64_t biosyn_highlight_line(
    biosyn_format_t format,
    const char *line,
    uint64_t len,
    uint64_t zero_based_line_no,
    biosyn_span_t *out,
    uint64_t out_cap
);

uint64_t biosyn_highlight_next_line(
    biosyn_state_t *state,
    const char *line,
    uint64_t len,
    biosyn_span_t *out,
    uint64_t out_cap
);

uint64_t biosyn_render_ansi_line(
    const char *line,
    uint64_t len,
    const biosyn_span_t *spans,
    uint64_t span_count,
    char *out,
    uint64_t out_cap
);

uint64_t biosyn_render_ansi_line_with_styles(
    const char *line,
    uint64_t len,
    const biosyn_span_t *spans,
    uint64_t span_count,
    const biosyn_ansi_style_t *styles,
    uint64_t style_count,
    char *out,
    uint64_t out_cap
);
```
