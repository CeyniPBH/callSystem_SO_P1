# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -O2

# Target executable name
TARGET = imagen

# Source files
SRCS = buddy_allocator.cpp imagen.cpp main.cpp stb_wrapper.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Header files
HEADERS = buddy_allocator.h imagen.h stb_image.h stb_image_write.h

# Default target
all: $(TARGET)

# Link the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files to object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean run