Information for Developers
===

## Patch submission procedure

Patches are submitted as pull requests on the [Github repository](https://github.com/TheTelephone/TheTelephone).
Please stick to the following rules for patch submissions and thus being respectful with other people's time.

If you need help, please submit an initial version and say that you need help as well as what kind of help.

Recommendations for satisfying patch submissions:

1. make sure that the implemented functionality compiles and works for your case,
2. be sure that the implemented functionality is useful for others (and thus important to include),
3. verify that your coding style (e.g., variable names, comments, empty lines) are in line with the code base,
4. fix all compiler warnings,
5. make sure that the documentation is included or updated (e.g., pd-help, demos)
6. apply _automatic_ code formating (see below),
7. test that TheTelephone still builds on all supported platforms (see [README.md](README.md)) or state which platforms were not tested, and
8. preferably write patches against the most recent commit of the master branch.

## Code formatting
The code is formatted using the [GNU Indent](https://www.gnu.org/software/indent/).

Please apply the following [GNU Indent](https://www.gnu.org/software/indent/) rule before submitting patches.
```bash
indent \
--braces-after-if-line \
--braces-after-func-def-line \
--braces-on-if-line \
--braces-on-func-def-line \
--braces-on-struct-decl-line \
--break-after-boolean-operator \
--break-before-boolean-operator \
--dont-break-function-decl-args \
--dont-break-function-decl-args-end \
--dont-break-procedure-type \
--dont-star-comments \
--space-after-cast \
--no-tabs \
--cuddle-else \
-l999 \
*.{c,h,cpp}
```

## Programming concepts

### Externals do not depend on each other

All externals are implemented in such a way that they do not depend on each other (i.e., _self-contained_).
Although this leads in some cases to code duplication, it ensures that the externals remain readable.

If a sufficient number of externals use the same functionality repeatedly, consider moving it carefully to a header-file.
An example for this is `generic_codec.h`, which provides basic data structures and functions for chunk-based one channel audio processing (e.g., buffers and resampling).

## Testing for _undefined symbols_
PureData's externals are technically speaking shared libraries.
These contain undefined symbols which are provided by PureData, when an external is loaded (see `m_pd.h`).
Therefore, the compiler/linker is configured, so undefined symbols are not reported as errors.
However, many externals implemented in TheTelephone link against other shared libraries (e.g., libopus) that are not provided by PureData.
If these symbols are not resolved while linking, these externals cannot be loaded due to the undefined symbols that are not provided by PureData.

The executable `test_undefined_symbols` tests if an external is properly linked, i.e., only PureData's symbols are undefined.  
By calling `make test` all build externals are ćhécked.  
Use `make test-verbose` to get verbose output.
