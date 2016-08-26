TheTelephone - Developer
===

Code formation
---

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
*.{c,h}
```

Code documentation
---

via [doxygen](www.doxygen.org)

```bash
cmake . && make doc
```