GPP = g++
CXX = gcc
SPECSUSER_INC = -I../Libraries -I../Driver/src
CPPFLAGS = -c -DPCI_CODE -DPLX_LINUX -DPLX_LITTLE_ENDIAN -Wall -pipe
APP_CPPFLAGS = -DGCC -Wno-write-strings
OUTPUT_OPTION = -o $@

%.o : %.cpp
	@echo Compiling: $<
	@$(CXX) $(OUTPUT_OPTION) $(CPPFLAGS) $(APP_CPPFLAGS) $(SPECSUSER_INC) $<
	@echo

testMezza.exe: MainProgram.o TestCommands.o
	@echo
	@echo Linking application: $@
	@$(GPP) -o $@ $^ -lpthread -L../Libraries -lSpecs -lSpecsUser
	@echo
	@echo
	@echo Executable $@ built successfully
	@echo
	@echo

clean:
	rm -f testMezza.exe *.o 
