.PHONY: build

build:
	chmod u+x ./build-cc.sh
	./build-cc.sh 2> /dev/null
	chmod u+x ./build-gm.sh
	./build-gm.sh 2> /dev/null

prerequisites: build

target: prerequisites 
