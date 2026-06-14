# libbiosyntax

`libbiosyntax` is a small C99 tokenizer/highlighter core for biological text
formats. It reads one text line at a time and returns byte spans with semantic
token-class IDs.

It does not perform IO. Compressed or binary inputs such as BAM, CRAM, and BCF
should be decoded by the caller before passing text lines to the core.

The core is intentionally copy-friendly:

```text
include/biosyntax.h
src/biosyntax.c
```

No runtime data files are required. The Makefile builds FFI-friendly shared and
static libraries by default. The `bcat` and `biocat` commands under `examples/`
are only example CLIs.

## Quick start

```sh
make
make test
make examples
```

For direct embedding:

```sh
cc -Iinclude your_program.c src/biosyntax.c -o your_program
```

## Documentation

- [User guide](docs/user-guide.md): supported formats, highlighting model, class
  metadata, and example CLIs.
- [C guide](docs/c.md): C integration, library builds, installation, and API
  examples.
- [C API reference](docs/api.md): full public C API summary.

## License

This implementation is inspired by BioSyntax and uses the same OSS license as
the uploaded BioSyntax source tree: GNU General Public License version 3 only.
See [LICENSE.md](LICENSE.md).

Original BioSyntax project: <https://github.com/bioSyntax/bioSyntax>
