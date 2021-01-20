CXX      	 := clang++-11
CXXFLAGS 	 := @../Compilerflags

LDFLAGS  	 := -lgtest -lpthread
BUILD    	 := ./build
TARGET   	 := muskat-test

OBJ_DIR_DBG  := $(BUILD)/$(TARGET)-debug/objects
OBJ_DIR_TST  := $(BUILD)/$(TARGET)-test/objects

APP_DIR  	 := ./bin
INCLUDE  	 := -iquote ../src/include -iquote src/include -isystem ../src/external
SRC      	 := $(wildcard src/*.cpp)

OBJECTS_DBG := $(SRC:%.cpp=$(OBJ_DIR_DBG)/%.o)
OBJECTS_TST := $(SRC:%.cpp=$(OBJ_DIR_TST)/%.o)


all: test

build_binary_debug: build $(APP_DIR)/$(TARGET)-debug
build_binary_test: build $(APP_DIR)/$(TARGET)-test

$(OBJ_DIR_DBG)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(OBJ_DIR_TST)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(APP_DIR)/$(TARGET)-debug: $(OBJECTS_DBG)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET)-debug $(OBJECTS_DBG) $(LDFLAGS)

$(APP_DIR)/$(TARGET)-test: $(OBJECTS_TST)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET)-test $(OBJECTS_TST) $(LDFLAGS)

.PHONY: all build clean debug

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR_TST)
	@mkdir -p $(OBJ_DIR_DBG)

debug: CXXFLAGS += -ggdb
debug: build_binary_debug

test: CXXFLAGS += -O3
test: build_binary_test

clean:
	-@rm -rvf $(OBJ_DIR_DBG)/*
	-@rm -rvf $(OBJ_DIR_TST)/*
	-@rm -rvf $(APP_DIR)/*

-include $(OBJECTS_DBG:.o=.d)
-include $(OBJECTS_TST:.o=.d)