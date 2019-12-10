llvm_path = /home/yasas/Fall2019/Compiler/llvm-7.0.0.src/
run:
	$(llvm_path)/build/bin/opt -load $(llvm_path)/build/lib/LLVMCompProject.so -project < $(file) > /dev/null

bitcode:
	clang-7 -c -emit-llvm main.cpp
	llvm-dis-7 main.bc	
