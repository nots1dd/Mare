GCC := gcc 

INCLUDE_DIR := -I build/Runtime -L build/Runtime

LINK_STD_M := -l goostdm

OBJ_FILE := GooOutput.o 

FINAL_BIN := OutputBin

MAKE := make

link:
	$(GCC) $(INCLUDE_DIR) $(OBJ_FILE) $(LINK_STD_M) -o $(FINAL_BIN)

run:
	LD_LIBRARY_PATH=build/Runtime ./$(FINAL_BIN)

all:
	$(MAKE) link 
	$(MAKE) run
