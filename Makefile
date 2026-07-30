# this makefile runs cmake to build the project and then copies the resulting .so file to the correct location for use as a LV2 plugin


all:
	
	mkdir -p build
	cd build && cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.lv2 ..
	cd build && cmake --build . -j$(shell nproc || echo 4) 

install: all
	cd build && cmake --install .

debian: all
	cd build && cpack -G DEB .

clean:
	rm -rf build

debug: 
	mkdir -p build
	cd build && cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=~/.lv2 ..
	cd build && cmake --build . -j$(shell nproc || echo 4) 
	cd build && cmake --install .