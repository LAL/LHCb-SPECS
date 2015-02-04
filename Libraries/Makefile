# Makefile template for shared library

CC = gcc # C compiler
CXXFLAGS = -DPLX_LINUX -DSPECS_EXPORTS -fPIC -I/group/online/ecs/Specs/include
CFLAGS = -g -fPIC -I/group/online/ecs/Specs/include # C flags
LDFLAGS = -shared -fPIC # linking flags
RM = rm -f # rm command

TARGET_LIB_1 = libSpecs.so # target lib
TARGET_LIB_2 = libSpecsUser.so 

SRCS_1 = Specs.cpp 
SRCS_1_c = Plx.c # source files
OBJS_1 = $(SRCS_1:.cpp=.o) $(SRCS_1_c:.c=.o)

SRCS_2 = SpecsUser.cpp # source files
OBJS_2 = $(SRCS_2:.cpp=.o)

SRCS = ${SRCS_1} ${SRCS_2} 
SRCS_c = ${SRCS_1_c}

.PHONY: all
all: ${TARGET_LIB_1} ${TARGET_LIB_2}

$(TARGET_LIB_1): $(OBJS_1)
	$(CC) ${LDFLAGS} -o $@ $^

$(TARGET_LIB_2): $(OBJS_2)
	$(CC) ${LDFLAGS} -o $@ $^

$(SRCS:.cpp=.d):%.d:%.cpp
	$(CC) $(CFLAGS) -MM $< >$@

$(SRCS_c:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.cpp=.d) $(SRCS_c:.c=.d)

.PHONY: clean
clean:
	-${RM} ${TARGET_LIB_1}  ${TARGET_LIB_2} ${OBJS_1} ${OBJS_2} $(SRCS:.cpp=.d) $(SRCS_c:.c=.d)
