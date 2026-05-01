# High-Performance Makefile for Monte Carlo Engine
# Optimized for macOS with maximum performance flags

# Compiler settings
CXX := clang++
TARGET := monte_carlo
SRCDIR := .
OBJDIR := .

# Source files
SOURCES := main.cc
OBJECTS := $(SOURCES:%.cc=$(OBJDIR)/%.o)
DEPS := $(OBJECTS:.o=.d)

# Compiler flags for maximum performance
CXXFLAGS := -std=c++17 \
            -O3 \
            -DNDEBUG \
            -march=native \
            -mtune=native \
            -flto \
            -ffast-math \
            -funroll-loops \
            -finline-functions \
            -pipe

# macOS specific optimizations
CXXFLAGS += -mmacosx-version-min=10.15 \
            -stdlib=libc++ \
            -Wno-unused-command-line-argument

# Threading flags
CXXFLAGS += -pthread

# Linker flags
LDFLAGS := -flto \
           -pthread \
           -Wl,-dead_strip \
           -stdlib=libc++

# Warning flags (optional, can be removed for maximum speed)
WARNINGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter

# Debug flags (only used in debug build)
DEBUG_FLAGS := -g -O0 -DDEBUG -fsanitize=address -fsanitize=undefined

# Build rules
.PHONY: all clean debug release fast help simple

# Default target - maximum performance
all: release

# Simple test build
simple: TARGET := simple_test
simple: SOURCES := simple_test.cc
simple: OBJECTS := $(SOURCES:%.cc=$(OBJDIR)/%.o)
simple: $(TARGET)

# Maximum performance build
release: CXXFLAGS += -DNDEBUG
release: $(TARGET)

# Ultra-fast build (removes all safety checks)
fast: CXXFLAGS += -DNDEBUG -Ofast -fno-stack-protector -fno-bounds-check
fast: CXXFLAGS := $(filter-out $(WARNINGS),$(CXXFLAGS))
fast: $(TARGET)

# Debug build
debug: CXXFLAGS = -std=c++17 $(DEBUG_FLAGS) $(WARNINGS) -pthread
debug: LDFLAGS = -pthread -fsanitize=address -fsanitize=undefined
debug: $(TARGET)

# Main target
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Object file compilation with dependency generation
$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Include dependencies
-include $(DEPS)

# Performance test targets
test: $(TARGET)
	@echo "Running performance test with 100M samples..."
	@time ./$(TARGET) 100000000

benchmark: $(TARGET)
	@echo "Running comprehensive benchmark..."
	@echo "10M samples:" && time ./$(TARGET) 10000000
	@echo "100M samples:" && time ./$(TARGET) 100000000
	@echo "1B samples:" && time ./$(TARGET) 1000000000

# Profile-guided optimization (PGO) build
pgo-generate: CXXFLAGS += -fprofile-generate
pgo-generate: LDFLAGS += -fprofile-generate
pgo-generate: clean $(TARGET)
	@echo "Running profile generation..."
	@./$(TARGET) 50000000 > /dev/null

pgo-use: CXXFLAGS += -fprofile-use -fprofile-correction
pgo-use: LDFLAGS += -fprofile-use
pgo-use: $(TARGET)
	@echo "PGO optimized build complete"

pgo: pgo-generate pgo-use
	@echo "Profile-guided optimization complete"

# System info
info:
	@echo "System Information:"
	@echo "Compiler: $(shell $(CXX) --version | head -1)"
	@echo "CPU: $(shell sysctl -n machdep.cpu.brand_string)"
	@echo "Cores: $(shell sysctl -n hw.ncpu)"
	@echo "L1 Cache: $(shell sysctl -n hw.l1dcachesize) bytes"
	@echo "L2 Cache: $(shell sysctl -n hw.l2cachesize) bytes"
	@echo "L3 Cache: $(shell sysctl -n hw.l3cachesize) bytes"

# Clean up
clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(TARGET) $(OBJECTS) $(DEPS) *.profdata *.profraw
	@echo "Clean complete"

# Install (copy to /usr/local/bin)
install: release
	@echo "Installing $(TARGET) to /usr/local/bin..."
	@cp $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Show available targets
help:
	@echo "Available targets:"
	@echo "  all/release  - Build with maximum performance optimizations (default)"
	@echo "  fast         - Ultra-fast build (removes safety checks)"
	@echo "  debug        - Build with debug information and sanitizers"
	@echo "  test         - Run performance test with 100M samples"
	@echo "  benchmark    - Run comprehensive benchmark"
	@echo "  pgo          - Profile-guided optimization build"
	@echo "  info         - Show system information"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install to /usr/local/bin"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make              # Build with default optimizations"
	@echo "  make fast         # Build with maximum speed"
	@echo "  make debug        # Build for debugging"
	@echo "  make pgo          # Build with profile-guided optimization"
	@echo "  make benchmark    # Build and run benchmarks"