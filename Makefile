include config.mk

TARGET_EXEC := $(BIN_DIR)/$(TARGET_NAME)

SRCS := $(shell find $(SRC_DIR) -name '*.cc')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS += $(SRC_DIR)
INC_FLAGS := $(patsubst %,-I%,$(INC_DIRS))
CXX_FLAGS += $(INC_FLAGS)

.PHONY: all clean

all: $(TARGET_EXEC)

$(TARGET_EXEC): $(OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LD_FLAGS) -o $@ $^ $(LIB_FLAGS)

$(BUILD_DIR)/%.cc.o: %.cc config.mk
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXX_FLAGS) -o $@ $<

clean:
	$(RM) -r $(BUILD_DIR) $(BIN_DIR)

-include $(DEPS)
