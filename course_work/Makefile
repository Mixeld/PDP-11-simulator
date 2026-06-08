# Корневой Makefile
.PHONY: all clean sim asm test help

all: sim asm

sim:
	$(MAKE) -C simulator

asm:
	$(MAKE) -C assembler

test: sim asm
	cd test && chmod +x run_test.sh && ./run_test.sh

clean:
	$(MAKE) -C simulator clean
	$(MAKE) -C assembler clean
	rm -rf tmp/
	@echo "Очищено"

help:
	@echo "Доступные команды:"
	@echo "  make       - собрать всё"
	@echo "  make sim   - собрать симулятор"
	@echo "  make asm   - собрать ассемблер"
	@echo "  make test  - запустить тесты"
	@echo "  make clean - очистить всё"