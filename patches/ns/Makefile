.PHONY: all clean

all:
	@mkdir -p build out
	@armips main.s
	@flips -c code.bin build/patched_code.bin out/code.ips
	@cp out/code.ips ../../romfs/patches/0004013000008002.ips

clean:
	@rm -rf build out
