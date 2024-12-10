# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
LDFLAGS = -lws2_32

# Targets
TARGETS = client.exe server.exe

# Default rule
all: $(TARGETS)

# Rule for building client executable
client.exe: client.o
	$(CXX) -o $@ $^ $(LDFLAGS)

# Rule for building server executable
server.exe: server.o 
	$(CXX) -o $@ $^ $(LDFLAGS)

# Compile client object file
client.o: client.cpp json.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile server object file
server.o: server.cpp json.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	del /f /q *.o *.exe 

# Phony targets
.PHONY: all clean
