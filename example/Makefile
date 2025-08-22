MCHK_SRC = ./my_test.c
MCHK_BIN = ./my_test
MCHK_MOD = ./some_module.c
C_STD = c89
CXX_STD = c++98
ARGS = # -Wall -Wextra -pedantic

# CC = gcc
# CXX = g++
CC = clang
CXX = clang++
# CC = clang --target=x86_64-w64-windows-gnu
# CXX = clang++ --target=x86_64-w64-windows-gnu

.PHONY: default
.PHONY: example test_c test_c_ignore test_cpp test_cpp_ignore test_all
.PHONY: run clean build_c build_c_ignore build_cpp build_cpp_ignore

# 
default:
	@echo "Make-target one of:  example(runs the test_c)  test_c  test_c_ignore  test_cpp  test_cpp_ignore"

# Compile and run example as C
example: test_c

# Or try with any of these combinations (yes, these are lousy excuses for "tests")
test_c          : clean build_c run_c
test_c_ignore   : clean build_c_ignore run_c_ignore
test_cpp        : clean build_cpp run_cpp
test_cpp_ignore : clean build_cpp_ignore run_cpp_ignore

test_all : test_c test_c_ignore test_cpp test_cpp_ignore


build_c:
	$(info )
	$(info $$ Building test_c...)
	$(info )
	${CC} ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN}_c -std=${C_STD} ${ARGS}
run_c:
	./${MCHK_BIN}_c
build_c_ignore:
	$(info )
	$(info $$ Building test_c_ignore...)
	$(info )
	${CC} -DMEMCHECK_IGNORE ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN}_c_ignore -std=${C_STD} ${ARGS}
run_c_ignore:
	./${MCHK_BIN}_c_ignore
build_cpp:
	$(info )
	$(info $$ Building test_cpp...)
	$(info )
	${CXX} ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN}_cpp -std=${CXX_STD} ${ARGS}
run_cpp:
	./${MCHK_BIN}_cpp
build_cpp_ignore:
	$(info )
	$(info $$ Building test_cpp_ignore...)
	$(info )
	${CXX} -DMEMCHECK_IGNORE ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN}_cpp_ignore -std=${CXX_STD} ${ARGS}
run_cpp_ignore:
	./${MCHK_BIN}_cpp_ignore

clean:
	rm -f ${MCHK_BIN}_c          ${MCHK_BIN}_c.exe
	rm -f ${MCHK_BIN}_c_ignore   ${MCHK_BIN}_c_ignore.exe
	rm -f ${MCHK_BIN}_cpp        ${MCHK_BIN}_cpp.exe
	rm -f ${MCHK_BIN}_cpp_ignore ${MCHK_BIN}_cpp_ignore.exe
