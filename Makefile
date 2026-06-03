# Корневой Makefile
.PHONY: all clean sim asm test help

all: sim asm
	@echo "Готово. Запусти: cd test && ./run_test.sh"

sim:
	@echo "Сборка симулятора..."
	@cd simulator && $(MAKE)
	@echo "Симулятор собран"

asm:
	@echo "Сборка ассемблера..."
	@cd assembler && $(MAKE)
	@echo "Ассемблер собран"

test: sim asm
	@cd test && chmod +x run_test.sh && ./run_test.sh

test-%: sim asm
	@cd assembler && ./asm ../test/asm_programs/$*.asm -o /tmp/test_$*
	@cd simulator && ./pdp11 /tmp/test_$*.lda
	@rm -f /tmp/test_$*.lda /tmp/test_$*.lst

run-%: sim asm
	@cd assembler && ./asm ../test/asm_programs/$*.asm -o /tmp/$*
	@cd simulator && ./pdp11 /tmp/$*.lda

clean:
	@cd simulator && $(MAKE) clean 2>/dev/null || true
	@cd assembler && $(MAKE) clean 2>/dev/null || true
	@rm -rf tmp/ test/tmp/ test/*.lda test/*.lst

status:
	@echo "Статус сборки:"
	@if [ -f simulator/pdp11 ]; then echo "  Симулятор: собран"; else echo "  Симулятор: не собран"; fi
	@if [ -f assembler/asm ]; then echo "  Ассемблер: собран"; else echo "  Ассемблер: не собран"; fi

help:
	@echo "Доступные команды:"
	@echo "  make          - собрать все"
	@echo "  make sim      - собрать симулятор"
	@echo "  make asm      - собрать ассемблер"
	@echo "  make test     - запустить тесты"
	@echo "  make test-XXX - запустить тест XXX"
	@echo "  make run-XXX  - запустить программу XXX"
	@echo "  make status   - показать статус"
	@echo "  make clean    - очистить"