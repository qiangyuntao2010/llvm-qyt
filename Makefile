all: configure

configure :
	mkdir llvm-profiler-install; cd llvm-profiler-install; \
	../llvm-profiler/configure --with-llvmsrc=${LLVM_SRC_ROOT} -with-llvmobj=${LLVM_OBJ_DIR} --prefix=${PWD} --enable-optimized=no; \
