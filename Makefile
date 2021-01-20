#############
# variables #
#############

# general settings
CXX      	 := clang++-11
CXXFLAGS 	 := @Compilerflags
LDFLAGS  	 :=
INCLUDE  	 := -iquote include -isystem submodules -isystem external_header
SRC      	 := $(wildcard src/*.cpp)

# project settings
BUILD    	 := ./build
APP_DIR  	 := ./bin

# CI settings
TARGET   	 := muskat

OBJ_DIR_DBG  := $(BUILD)/$(TARGET)-debug/objects
OBJ_DIR_TST  := $(BUILD)/$(TARGET)-test/objects
OBJ_DIR_REL  := $(BUILD)/$(TARGET)-release/objects

OBJECTS_DBG := $(SRC:%.cpp=$(OBJ_DIR_DBG)/%.o)
OBJECTS_TST := $(SRC:%.cpp=$(OBJ_DIR_TST)/%.o)
OBJECTS_REL := $(SRC:%.cpp=$(OBJ_DIR_REL)/%.o)

###########
# targets #
###########

all: test

build_binary_debug: build $(APP_DIR)/$(TARGET)-debug
build_binary_test: build $(APP_DIR)/$(TARGET)-test
build_binary_release: build $(APP_DIR)/$(TARGET)-release

$(OBJ_DIR_DBG)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(OBJ_DIR_TST)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(OBJ_DIR_REL)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(APP_DIR)/$(TARGET)-debug: $(OBJECTS_DBG)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET)-debug $(OBJECTS_DBG) $(LDFLAGS)

$(APP_DIR)/$(TARGET)-test: $(OBJECTS_TST)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET)-test $(OBJECTS_TST) $(LDFLAGS)

$(APP_DIR)/$(TARGET)-release: $(OBJECTS_REL)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET)-release $(OBJECTS_REL) $(LDFLAGS)

.PHONY: all build clean debug release

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR_REL)
	@mkdir -p $(OBJ_DIR_DBG)
	@mkdir -p $(OBJ_DIR_TST)

debug: CXXFLAGS += -ggdb
debug: build_binary_debug

release: CXXFLAGS += -O3 -DNDEBUG
release: build_binary_release

test: CXXFLAGS += -O3
test: build_binary_test

clean:
	-@rm -rvf $(OBJ_DIR_REL)/*
	-@rm -rvf $(OBJ_DIR_DBG)/*
	-@rm -rvf $(OBJ_DIR_TST)/*
	-@rm -rvf $(APP_DIR)/*

docker:
	docker build --no-cache .

-include $(OBJECTS_REL:.o=.d)
-include $(OBJECTS_DBG:.o=.d)
-include $(OBJECTS_TST:.o=.d)
