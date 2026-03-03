.PHONY: all clean boss http ssl

all: boss cecd ssl

boss:
	@make -C boss
	@make -C cecd
	@make -C ns
	@make -C ssl
	@cp ssl/out/code.ips ../romfs/patches/0004013000002F02.ips

clean:
	@make -C boss clean
	@make -C cecd clean
	@make -C ns clean
	@make -C ssl clean
