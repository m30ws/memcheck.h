# this makefile could be improved but it's not really important
# don't forget to vcvarsall.bat or init64/32.bat before invoking msvc
MCHK_SRC = ./my_test.c
MCHK_BIN = ./my_test
MCHK_MOD = ./some_module.c

C_STD = c89
CXX_STD = c++98
MSVC_STD = c11  # msvc only has "c11" and "c17" apparently

# Gcc with pedantic will yell about not supporting %zu before c99
# Clang++ with -pedantic will yell about not supporting long long & its printf fmt prior to c++11 (gcc doesn't seem to mind)
C_FLAGS = -O2 -std=${C_STD} -Wall -Wextra  # -pedantic
CXX_FLAGS = -O2 -std=${CXX_STD} -Wall -Wextra  # -pedantic
MSVC_FLAGS = -O2 /TC /std:${MSVC_STD} /EHsc /MP /MTd /Zi /Oi /Wall /wd4711 /wd4710 /wd4820 /wd4464 /wd4189 /D_CRT_SECURE_NO_WARNINGS  # -pedantic

CC = gcc
CXX = g++
# CC = clang
# CXX = clang++
# CC = clang --target=x86_64-w64-windows-gnu
# CXX = clang++ --target=x86_64-w64-windows-gnu

.PHONY: default
.PHONY: example test_c test_c_ignore test_cpp test_cpp_ignore test_msvc_c test_all
.PHONY: run clean build_c build_c_ignore build_cpp build_cpp_ignore build_msvc_c

# 
default:
	@echo "Make-target one of:  example(runs the test_c)  test_c  test_c_ignore  test_cpp  test_cpp_ignore  test_msvc_c"

# Compile and run example as C
example: test_c

# Or try with any of these combinations (yes, these are lousy excuses for "tests")
test_c          : clean build_c run_c
test_c_ignore   : clean build_c_ignore run_c_ignore
test_cpp        : clean build_cpp run_cpp
test_cpp_ignore : clean build_cpp_ignore run_cpp_ignore

test_msvc_c     : clean build_msvc_c run_msvc_c

test_all : test_c test_c_ignore test_cpp test_cpp_ignore # test_msvc_c
	

build_c:
	$(info )
	$(info $$ Building test_c...)
	$(info )
	${CC} ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN}_c ${C_FLAGS}
run_c:
	./${MCHK_BIN}_c

build_c_ignore:
	$(info )
	$(info $$ Building test_c_ignore...)
	$(info )
	${CC} ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN}_c_ignore ${C_FLAGS} -DMEMCHECK_IGNORE
run_c_ignore:
	./${MCHK_BIN}_c_ignore

build_cpp:
	$(info )
	$(info $$ Building test_cpp...)
	$(info )
	${CXX} ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN}_cpp ${CXX_FLAGS}
run_cpp:
	./${MCHK_BIN}_cpp

build_cpp_ignore:
	$(info )
	$(info $$ Building test_cpp_ignore...)
	$(info )
	${CXX} ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN}_cpp_ignore ${CXX_FLAGS} -DMEMCHECK_IGNORE
run_cpp_ignore:
	./${MCHK_BIN}_cpp_ignore

build_msvc_c:
	cl ${MCHK_SRC} ${MCHK_MOD} /Fe:${MCHK_BIN}_msvc_c ${MSVC_FLAGS}
run_msvc_c:
	./${MCHK_BIN}_msvc_c

clean:
	rm -f ${MCHK_BIN}_c          ${MCHK_BIN}_c.exe
	rm -f ${MCHK_BIN}_c_ignore   ${MCHK_BIN}_c_ignore.exe
	rm -f ${MCHK_BIN}_cpp        ${MCHK_BIN}_cpp.exe
	rm -f ${MCHK_BIN}_cpp_ignore ${MCHK_BIN}_cpp_ignore.exe
	rm -f ${MCHK_BIN}_msvc_c.exe $(MCHK_SRC:.c=.obj) $(MCHK_MOD:.c=.obj) ${MCHK_BIN}_msvc_c.pdb ${MCHK_BIN}_msvc_c.ilk ./vc*.pdb
