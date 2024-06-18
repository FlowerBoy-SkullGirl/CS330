#test:
#	gcc -o test source/test.c -lm -lncurses -g
SHAPES := 3DShapes/$(wildcard *.h) 3DShapes/$(wildcard *.cpp)
SHAPE_PATH := 3DShapes
UTILITIES := Utilities/$(wildcard *.h) Utilities/$(wildcard *.cpp)
UTILITY_PATH := Utilities
SOURCE_PATH := Source
INCLUDES := -lGL -lGLEW -lGLU -lGL -lglfw -lm -lXrandr -lXi -lXxf86vm -lpthread -g
CONTENT_DIR := ../CS330Content


SOURCE := Source/SceneManager.cpp Source/ViewManager.cpp Source/MainCode.cpp 
COMP_LIB := bin/*.o

unzip:
	7z x *.zip

stage:
	cp -r $(CONTENT_DIR)/$(UTILITY_PATH) $(CONTENT_DIR)/$(SHAPE_PATH) .
	mkdir ./bin

prepare: stage
	sed -i 's/\"shapemeshes.h\"/\"ShapeMeshes.h\"/g' $(SHAPE_PATH)/ShapeMeshes.cpp
	sed -i 's/ M_PI/ SM_PI/g' $(SHAPE_PATH)/ShapeMeshes.cpp
	find . -type f ! -name 'Makefile' -exec sed -i '0,/#include <glm/{s/#include <glm/#define GLM_ENABLE_EXPERIMENTAL\n#include <glm/}' {} \;
	find ./$(SOURCE_PATH) ./$(SHAPE_PATH) ./$(UTILITY_PATH) -type f -exec sed -i 's///g' {} \;

linux: $(SHAPES) $(UTILITIES) $(SOURCE) 
	g++ -o bin/linux_binary_x86_64 -I$(UTILITY_PATH) -I$(SHAPE_PATH) -I$(SOURCE_PATH) $(SOURCE) $(UTILITY_PATH)/*.cpp $(SHAPE_PATH)/*.cpp $(INCLUDES)

clean: 
	rm bin/$(wildcard *.o)

destroy:
	rm -r $(SOURCE_PATH) $(UTILITY_PATH) $(SHAPE_PATH) shaders textures bin Debug glew32.dll 7-1*

reset: destroy unzip prepare
