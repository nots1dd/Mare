# Here are some known issues with this compiler:

1. Binary ops does not understand strings:

```text 
Strings are handled in this compiler in such a way that you DO NOT have to call a print function! 
The codegen for StringExpr automatically links the external library `printf` that does the job.

I did it in this way coz its pretty cool, and does not break or interfere with externs and spares the job of adding another token.
```
