CFLAGS = -g -lSDL2 -lm

build: src/*.c src/hdl-core/*.c src/hdl-core/*.h
	@echo "Building hdl-viewer"
	mkdir -p ./bin
	gcc src/hdl-core/*.c src/*.c $(CFLAGS) -o bin/hdl-viewer

install: build
	@echo "Installing hdl-viewer..."
	cp ./bin/hdl-viewer /usr/bin/hdl-viewer