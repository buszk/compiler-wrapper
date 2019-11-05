all: compiler-wrapper

compiler-wrapper: compiler-wrapper.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@
	ln -sf compiler-wrapper compiler-wrapper++
clean:
	rm -rf compiler-wrapper compiler-wrapper++