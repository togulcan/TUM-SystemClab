# Create build directory for each exercise and recreate hierarchy from exercise directory's pov
BUILD_DIR := .build
EX_BUILD_DIR := $(BUILD_DIR)/$(notdir $(abspath .))

OBJS := $(SRCS:%=$(EX_BUILD_DIR)/%.o)
DEPS := $(SRCS:%=$(EX_BUILD_DIR)/%.d)
EXE  := $(MODULE).x

CXX      := g++
LD       := g++
CXXFLAGS := -g -DSC_INCLUDE_DYNAMIC_PROCESSES -Wno-deprecated -Wall -I. -I$(SYSTEMC)/include -I../npu_common
LDFLAGS  := -lm -lpthread $(EXTRA_LIBS)

TARGET_ARCH := linux64

.PHONY: all clean
.PRECIOUS: $(EX_BUILD_DIR)/%.cpp.o $(EX_BUILD_DIR)/%.cpp.d libsystemc.a

all: $(EXE)

clean:
	@echo "(CLN) Clean"
	@rm -rf $(BUILD_DIR) $(EXE)

$(EX_BUILD_DIR)/%.cpp.o: %.cpp
	@echo "(CXX) $@"
	@mkdir -p $(EX_BUILD_DIR)
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -M -MT $@ -MF $(@:%.o=%.d) $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/libsystemc.a: $(SYSTEMC)/lib-$(TARGET_ARCH)/libsystemc.a
	@echo "(CPY) $@"
	@mkdir -p $(@D)
	@objcopy -g $< $@

$(EXE): $(OBJS) $(BUILD_DIR)/libsystemc.a
	@echo "(LNK) $@"
	@$(LD) -o $@ $^ $(LDFLAGS)

-include $(DEPS)
