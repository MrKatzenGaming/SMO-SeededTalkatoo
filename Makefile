.PHONY: debug clean

check_symbols:
	python check_symbols.py

debug: format
	cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build && $(MAKE) -C build

release: clean format
	cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build && $(MAKE) -C build
	python ./make-Release/release.py

format:
	find ./src -name "*.*" | xargs clang-format -i

clean:
	rm -r build || true
