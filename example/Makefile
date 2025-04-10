MCHK_SRC = ./my_test.c
MCHK_BIN = ./my_test
MCHK_MOD = ./some_module.c
C_STD = c89
CXX_STD = c++98
CC = clang
CXX = clang++

.PHONY: example test_c test_c_ignore test_cpp test_cpp_ignore test_all
.PHONY: run clean build_c build_c_ignore build_cpp build_cpp_ignore


# Compile and run example as C
example: test_c

# Or try with any of these combinations
test_c          : clean build_c run
test_c_ignore   : clean build_c_ignore run
test_cpp        : clean build_cpp run
test_cpp_ignore : clean build_cpp_ignore run

test_all : test_c test_c_ignore test_cpp test_cpp_ignore


run:
	./${MCHK_BIN}
clean:
	rm -f ./${MCHK_BIN} ./${MCHK_BIN}.exe
build_c:
	${CC} ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN} -std=${C_STD}
build_c_ignore:
	${CC} -DMEMCHECK_IGNORE ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN} -std=${C_STD}
build_cpp:
	${CXX} ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN} -std=${CXX_STD}
build_cpp_ignore:
	${CXX} -DMEMCHECK_IGNORE ${MCHK_SRC} ${MCHK_MOD} -o ${MCHK_BIN} -std=${CXX_STD}
