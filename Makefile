.PHONY: debug clean

debug: format
	cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build && $(MAKE) -C build

release: clean format
	cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build && $(MAKE) -C build
	python ./make-Release/release.py

format:
	find ./src -name "*.*" | xargs clang-format -i

clean:
	rm -r build || true
