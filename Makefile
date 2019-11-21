all: compiler-wrapper

debug: CXXFLAGS += -DDEBUG -g
debug: compiler-wrapper

compiler-wrapper: compiler-wrapper.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@
	ln -sf compiler-wrapper compiler-wrapper++

test: compiler-wrapper
	./compiler-wrapper helloworld.c -o helloworld-c
	./compiler-wrapper++ helloworld.cpp -o helloworld-cpp

clean:
	rm -rf compiler-wrapper compiler-wrapper++ hello-world-c hello-world-cpp
