# gcmalloc
## introduction
- C++11 based
- Use two buffer to speed up
- main_arena and non_main_arena support
- thread safe 
- root tracing to mark garbage
- mark & sweep to recycle garbage

## install
- first: download this rep
```asm
git clone https://github.com/Runner-2019/gcmalloc.git
```

- 2nd: enter command as follows
```asm
cd gcmalloc
mkdir build
cd build
cmake ..
make gcmalloc
```

## performance
