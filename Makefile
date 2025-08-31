CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Wextra -O2
SRC       := main.cpp
TARGET    := compiler 

BUILD_DIR := build
ASM_SUB   := asm
ASM_DIR   := $(BUILD_DIR)/$(ASM_SUB)

ASM_SRC   := $(ASM_DIR)/main1.asm
OBJ       := $(ASM_DIR)/main1.o
ASM_OUT   := $(ASM_DIR)/main1

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR) $(ASM_DIR)

run: $(BUILD_DIR)/$(TARGET)
	@if [ -z "$(INPUT)" ]; then \
	  echo "Usage: make run INPUT=path/to/sourcefile"; \
	  exit 1; \
	fi
	@echo "Running generator from $(BUILD_DIR) and writing asm to $(ASM_DIR)/main1.asm"
	cd $(BUILD_DIR) && ./$(TARGET) "$(abspath $(INPUT))"

asm: $(ASM_SRC)
	@if [ ! -f $(ASM_SRC) ]; then \
	  echo "No asm found at $(ASM_SRC). Run 'make run INPUT=...' first."; \
	  exit 1; \
	fi
	nasm -felf64 $(ASM_SRC) -o $(OBJ)
	ld -o $(ASM_OUT) $(OBJ)

run-asm: asm
	@echo "Running assembled program: $(ASM_OUT)"
	$(ASM_OUT)

clean:
	-rm -rf $(BUILD_DIR)

