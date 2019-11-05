# C/C++ Compiler Wrapper

This is a wrapper application that force some flags for your compiler.  
You need to replace your original compiler with the wrapper application.  
A common use pattern would be setting `CC/CXX` to our wrapper applicating  
during configuration or compiling time.    

```
CC=compiler-wrapper /path/to/src/configure
CXX=compiler-wrapper++ /path/to/src/configure
```

To specify the true compiler, and the flags you want to force. You can   
either edit the `config` file in this direcetory, or use environment  
variable `WRAP_*`. They include `WRAP_CC`, `WRAP_CXX`, `WRAP_CFLAGS`,  
`WRAP_CXXFLAGS` and `WRAP_LDFLAGS`.

```
WRAP_CC=clang WRAP_CFLAGS=-fsanitize WRAP_LDFLAGS=-fsanitize make -j
```
The above command will invoke clang with -fsanitize flag both compiling  
and linking time during the whole `make -j`.
