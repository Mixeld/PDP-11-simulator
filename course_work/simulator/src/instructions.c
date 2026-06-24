#include <stdio.h>
#include "instructions.h"
#include "decode.h"
#include "memory.h"
#include "cpu.h"

/* ================ Вспомогательные ================ */

static void do_branch(PDP11 *cpu, uint16_t instr)
{
    int8_t offset = INSTR_BRANCH_OFFSET(instr);
    cpu->reg[PC] += (int16_t)(offset * 2);
}

/* ================ Двухоперандные ================ */

static void instr_mov(PDP11 *cpu, uint16_t instr)
{
    int sm = INSTR_SRC_MODE(instr);
    int sr = INSTR_SRC_REG(instr);
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t src = fetch_operand(cpu, sm, sr);

    if (dm == 0) {
        cpu->reg[dr] = src;
    } else {
        uint16_t addr = resolve_dst_addr(cpu, dm, dr);
        mem_write_word(cpu, addr, src);
    }

    cpu_update_nz(cpu, src);
    cpu_clear_flag(cpu, PSW_V);
}

static void instr_add(PDP11 *cpu, uint16_t instr)
{
    int sm = INSTR_SRC_MODE(instr);
    int sr = INSTR_SRC_REG(instr);
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t src = fetch_operand(cpu, sm, sr);
    uint16_t dst;
    uint16_t dst_addr = 0;

    if (dm == 0) {
        dst = cpu->reg[dr];
    } else {
        dst_addr = resolve_dst_addr(cpu, dm, dr);
        dst = mem_read_word(cpu, dst_addr);
    }

    uint32_t result32 = (uint32_t)src + (uint32_t)dst;
    uint16_t result = (uint16_t)result32;

    if (dm == 0)
        cpu->reg[dr] = result;
    else
        mem_write_word(cpu, dst_addr, result);

    cpu_update_nz(cpu, result);

    if (result32 > 0xFFFF)
        cpu_set_flag(cpu, PSW_C);
    else
        cpu_clear_flag(cpu, PSW_C);

    if (((src ^ dst) & 0x8000) == 0 && ((src ^ result) & 0x8000) != 0)
        cpu_set_flag(cpu, PSW_V);
    else
        cpu_clear_flag(cpu, PSW_V);
}

static void instr_sub(PDP11 *cpu, uint16_t instr)
{
    int sm = INSTR_SRC_MODE(instr);
    int sr = INSTR_SRC_REG(instr);
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t src = fetch_operand(cpu, sm, sr);
    uint16_t dst;
    uint16_t dst_addr = 0;

    if (dm == 0) {
        dst = cpu->reg[dr];
    } else {
        dst_addr = resolve_dst_addr(cpu, dm, dr);
        dst = mem_read_word(cpu, dst_addr);
    }

    uint16_t result = dst - src;

    if (dm == 0)
        cpu->reg[dr] = result;
    else
        mem_write_word(cpu, dst_addr, result);

    cpu_update_nz(cpu, result);

    if (dst < src)
        cpu_set_flag(cpu, PSW_C);
    else
        cpu_clear_flag(cpu, PSW_C);

    if (((dst ^ src) & 0x8000) != 0 && ((dst ^ result) & 0x8000) != 0)
        cpu_set_flag(cpu, PSW_V);
    else
        cpu_clear_flag(cpu, PSW_V);
}

static void instr_cmp(PDP11 *cpu, uint16_t instr)
{
    int sm = INSTR_SRC_MODE(instr);
    int sr = INSTR_SRC_REG(instr);
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t src = fetch_operand(cpu, sm, sr);
    uint16_t dst = fetch_operand(cpu, dm, dr);

    uint16_t result = src - dst;

    cpu_update_nz(cpu, result);

    if (src < dst)
        cpu_set_flag(cpu, PSW_C);
    else
        cpu_clear_flag(cpu, PSW_C);

    if (((src ^ dst) & 0x8000) != 0 && ((src ^ result) & 0x8000) != 0)
        cpu_set_flag(cpu, PSW_V);
    else
        cpu_clear_flag(cpu, PSW_V);
}

static void instr_bit(PDP11 *cpu, uint16_t instr){
    int sm = INSTR_SRC_MODE(instr);
    int sr = INSTR_SRC_REG(instr);
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t src = fetch_operand(cpu, sm, sr);
    uint16_t dst = fetch_operand(cpu, dm, dr);

    uint16_t result = src & dst;
    
    cpu_update_nz(cpu, result);
    cpu_clear_flag(cpu,PSW_V);
}

static void instr_bic (PDP11 * cpu, uint16_t instr){
    int sm = INSTR_SRC_MODE(instr);
    int sr = INSTR_SRC_REG(instr);
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t src =fetch_operand(cpu, sm, sr);
    uint16_t dst;
    uint16_t dst_addr =0;

    if (dm == 0) {
        dst = cpu ->reg[dr];
    } else {
        dst_addr = resolve_dst_addr(cpu, dm, dr);
        dst = mem_read_word(cpu, dst_addr);
    }

    uint16_t result = dst & (~src);

    if (dm == 0){
        cpu -> reg[dr] = result;
    } else {
        mem_write_word (cpu, dst_addr, result);
    }

    cpu_update_nz(cpu, result);
    cpu_clear_flag(cpu, PSW_V);
}

static void instr_bis(PDP11 *cpu, uint16_t instr){
    int sm = INSTR_SRC_MODE(instr);
    int sr = INSTR_SRC_REG(instr);
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t src = fetch_operand(cpu, sm, sr);
    uint16_t dst;
    uint16_t dst_addr = 0;

    if (dm == 0){
        dst = cpu -> reg[dr];
    } else {
        dst_addr = resolve_dst_addr (cpu, dm, dr);
        dst = mem_read_word (cpu, dst_addr);
    }

    uint16_t result = dst | src;
    
    if (dm == 0){
        cpu -> reg[dr] = result;
    } else {
        mem_write_word (cpu, dst_addr, result);
    }

    cpu_update_nz(cpu, result);
    cpu_clear_flag(cpu, PSW_V);
}





/* ================ Однооперандные ================ */

static void instr_clr(PDP11 *cpu, uint16_t instr)
{
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    if (dm == 0) {
        cpu->reg[dr] = 0;
    } else {
        uint16_t addr = resolve_dst_addr(cpu, dm, dr);
        mem_write_word(cpu, addr, 0);
    }

    cpu_set_flag(cpu, PSW_Z);
    cpu_clear_flag(cpu, PSW_N);
    cpu_clear_flag(cpu, PSW_V);
    cpu_clear_flag(cpu, PSW_C);
}

static void instr_com (PDP11 *cpu, uint16_t instr){
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val;
    uint16_t addr = 0;

    if (dm == 0){
        val = cpu -> reg[dr];
    } else {
        addr = resolve_dst_addr(cpu, dm,dr);
        val = mem_read_word(cpu, addr);
    }

    uint16_t result = ~ val;

    if (dm == 0){
        cpu -> reg[dr] = result;
    } else {
        mem_write_word (cpu, addr, result);
    }

    cpu_update_nz(cpu, result);

    cpu_clear_flag(cpu, PSW_V);
    cpu_set_flag (cpu, PSW_C);
}

static void instr_adc (PDP11 *cpu, uint16_t instr){
    int dm = INSTR_DST_MODE (instr);
    int dr = INSTR_DST_REG (instr);

    uint16_t val;
    uint16_t addr = 0;

    if (dm == 0){
        val = cpu -> reg[dr];
    } else {
        addr = resolve_dst_addr(cpu, dm, dr);
        val = mem_read_word(cpu, addr);
    }

    uint16_t c = cpu_get_flag(cpu, PSW_C) ? 1 : 0;

    uint32_t result32 = (uint32_t)val + c;
    uint16_t result = (uint16_t)result32;

    if (dm == 0){
        cpu -> reg[dr] = result;
    } else {
        mem_write_word (cpu, addr, result);
    }

    cpu_update_nz(cpu, result);

    if (val == 0077777 && c == 1){
        cpu_set_flag(cpu, PSW_V);
    } else {
        cpu_clear_flag(cpu, PSW_V);
    }

    if (val == 0177777 && c == 1){
        cpu_set_flag(cpu, PSW_C);
    } else {
        cpu_clear_flag(cpu, PSW_C);
    }
}

static void instr_sbc (PDP11 *cpu, uint16_t instr){
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val;
    uint16_t addr = 0;

    if (dm == 0) {
        val = cpu -> reg[dr];
    } else {
        addr = resolve_dst_addr(cpu, dm, dr);
        val = mem_read_word(cpu, addr);
    }

    uint16_t c = cpu_get_flag(cpu, PSW_C) ? 1 : 0;

    uint32_t result32 = (uint32_t)val - c;
    uint16_t result = (uint16_t)result32;

    if (dm == 0){
        cpu -> reg[dr] = result;
    } else {
        mem_write_word(cpu, addr, result);
    }

    cpu_update_nz(cpu, result);

    if (val == 0100000 && c ==1){
        cpu_set_flag(cpu, PSW_V);
    } else {
        cpu_clear_flag(cpu, PSW_V);
    }

    if(val == 0 && c == 1){
        cpu_set_flag(cpu, PSW_C);
    } else {
        cpu_clear_flag(cpu, PSW_C);
    }
}

static void instr_sob (PDP11 *cpu, uint16_t instr){
    int reg = (instr >> 6) & 7;
    int offset = instr & 077;

    cpu -> reg[reg]--;

    if(cpu -> reg[reg] != 0){
        cpu -> reg[PC] -= offset * 2;
    }    
}

static void instr_swab(PDP11 *cpu, uint16_t instr){
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val;
    uint16_t addr = 0;

    if (dm == 0){
        val = cpu -> reg[dr];
    } else {
        addr = resolve_dst_addr(cpu, dm, dr);
        val = mem_read_word(cpu,addr);
    }

    uint16_t result = ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);

    if (dm == 0){
        cpu -> reg[dr] = result;
    } else {
        mem_write_word (cpu, addr, result);
    }

    uint8_t low_byte = result & 0xFF;

    if (low_byte == 0){
        cpu_set_flag(cpu, PSW_Z);
    } else {
        cpu_clear_flag(cpu, PSW_Z);
    }

    if (low_byte & 0x80){
        cpu_set_flag(cpu, PSW_N);
    } else {
        cpu_clear_flag (cpu, PSW_N);
    }

    cpu_clear_flag (cpu, PSW_V);
    cpu_clear_flag (cpu, PSW_C);
}

static void instr_jmp (PDP11 *cpu, uint16_t instr) {
    int dm = INSTR_DST_MODE (instr);
    int dr = INSTR_DST_REG (instr);

    if (dm == 0){
        printf ("JMP с регистровым выполнять нельзя\n");
        cpu -> running = 0;
        return;
    }

    uint16_t addr = resolve_dst_addr(cpu,dm, dr);

    cpu -> reg[PC] = addr;
}

static void instr_inc(PDP11 *cpu, uint16_t instr)
{
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val;
    uint16_t addr = 0;

    if (dm == 0) {
        val = cpu->reg[dr];
    } else {
        addr = resolve_dst_addr(cpu, dm, dr);
        val = mem_read_word(cpu, addr);
    }

    uint16_t result = val + 1;

    if (dm == 0)
        cpu->reg[dr] = result;
    else
        mem_write_word(cpu, addr, result);

    cpu_update_nz(cpu, result);

    if (val == 0x7FFF)
        cpu_set_flag(cpu, PSW_V);
    else
        cpu_clear_flag(cpu, PSW_V);
}

static void instr_dec(PDP11 *cpu, uint16_t instr)
{
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val;
    uint16_t addr = 0;

    if (dm == 0) {
        val = cpu->reg[dr];
    } else {
        addr = resolve_dst_addr(cpu, dm, dr);
        val = mem_read_word(cpu, addr);
    }

    uint16_t result = val - 1;

    if (dm == 0)
        cpu->reg[dr] = result;
    else
        mem_write_word(cpu, addr, result);

    cpu_update_nz(cpu, result);

    if (val == 0x8000)
        cpu_set_flag(cpu, PSW_V);
    else
        cpu_clear_flag(cpu, PSW_V);
}

static void instr_tst(PDP11 *cpu, uint16_t instr)
{
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val = fetch_operand(cpu, dm, dr);

    cpu_update_nz(cpu, val);
    cpu_clear_flag(cpu, PSW_V);
    cpu_clear_flag(cpu, PSW_C);
}

static void instr_neg(PDP11 *cpu, uint16_t instr)
{
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val;
    uint16_t addr = 0;

    if (dm == 0) {
        val = cpu->reg[dr];
    } else {
        addr = resolve_dst_addr(cpu, dm, dr);
        val = mem_read_word(cpu, addr);
    }

    uint16_t result = (~val) + 1;

    if (dm == 0)
        cpu->reg[dr] = result;
    else
        mem_write_word(cpu, addr, result);

    cpu_update_nz(cpu, result);

    if (result == 0x8000)
        cpu_set_flag(cpu, PSW_V);
    else
        cpu_clear_flag(cpu, PSW_V);

    if (result != 0)
        cpu_set_flag(cpu, PSW_C);
    else
        cpu_clear_flag(cpu, PSW_C);
}

/* ================ СДВИГИ ================*/

static void instr_asl(PDP11 *cpu, uint16_t instr){
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val;
    uint16_t addr =0;

    if (dm == 0){
        val = cpu -> reg[dr];
    } else {
        addr = resolve_dst_addr(cpu, dm , dr);
        val = mem_read_word(cpu, addr);
    }

    uint16_t result = val << 1;

    if (dm == 0){
        cpu -> reg[dr] = result;
    } else {
        mem_write_word(cpu, addr, result);
    }

    if (val & 0x8000){
        cpu_set_flag(cpu,PSW_C);
    } else {
        cpu_clear_flag(cpu,PSW_C);
    }

    cpu_update_nz(cpu, result);

    int n = cpu_get_flag(cpu, PSW_N);
    int c = cpu_get_flag(cpu, PSW_C);

    if (n ^ c) {
        cpu_set_flag(cpu,PSW_V);
    } else {
        cpu_clear_flag(cpu, PSW_V);
    }
}

static void instr_asr(PDP11 *cpu, uint16_t instr){
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val;
    uint16_t addr =0;

    if (dm == 0){
        val = cpu -> reg[dr];
    } else {
        addr = resolve_dst_addr(cpu, dm , dr);
        val = mem_read_word(cpu, addr);
    }

    uint16_t result = (val >> 1) | (val & 0x8000);

    if (dm == 0){
        cpu -> reg[dr] = result;
    } else {
        mem_write_word(cpu, addr, result);
    }

    if (val & 1){
        cpu_set_flag(cpu,PSW_C);
    } else {
        cpu_clear_flag(cpu,PSW_C);
    }

    cpu_update_nz(cpu, result);

    int n = cpu_get_flag(cpu, PSW_N);
    int c = cpu_get_flag(cpu, PSW_C);

    if (n ^ c) {
        cpu_set_flag(cpu,PSW_V);
    } else {
        cpu_clear_flag(cpu, PSW_V);
    }
}

static void instr_rol (PDP11 *cpu, uint16_t instr){
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val;
    uint16_t addr = 0;

    if (dm == 0){
        val = cpu -> reg[dr];
    } else {
        addr = resolve_dst_addr (cpu, dm, dr);
        val = mem_read_word(cpu, addr);
    }

    uint16_t old_c = cpu_get_flag(cpu, PSW_C) ? 1 : 0;

    if (val & 0x8000){
        cpu_set_flag(cpu, PSW_C);
    } else {
        cpu_clear_flag(cpu, PSW_C);
    }

    uint16_t result = (val << 1) | old_c;

    if (dm == 0){
        cpu -> reg[dr] = result;
    } else {
        mem_write_word(cpu, addr, result);
    }

    cpu_update_nz(cpu, result);

    int n = cpu_get_flag(cpu, PSW_N);
    int c = cpu_get_flag(cpu, PSW_C);
    if (n ^ c){
        cpu_set_flag(cpu, PSW_V);
    } else {
        cpu_clear_flag(cpu, PSW_V);
    }
}


static void instr_ror (PDP11 *cpu, uint16_t instr){
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t val;
    uint16_t addr = 0;

    if (dm == 0){
        val = cpu -> reg[dr];
    } else {
        addr = resolve_dst_addr (cpu, dm, dr);
        val = mem_read_word(cpu, addr);
    }

    uint16_t old_c = cpu_get_flag(cpu, PSW_C) ? 1 : 0;

    if (val & 1){
        cpu_set_flag(cpu, PSW_C);
    } else {
        cpu_clear_flag(cpu, PSW_C);
    }

    uint16_t result = (val >> 1) | (old_c << 15);

    if (dm == 0){
        cpu -> reg[dr] = result;
    } else {
        mem_write_word(cpu, addr, result);
    }

    cpu_update_nz(cpu, result);

    int n = cpu_get_flag(cpu, PSW_N);
    int c = cpu_get_flag(cpu, PSW_C);
    if (n ^ c){
        cpu_set_flag(cpu, PSW_V);
    } else {
        cpu_clear_flag(cpu, PSW_V);
    }
}


/* ================ Переходы ================ */

static void instr_br(PDP11 *cpu, uint16_t instr)
{
    do_branch(cpu, instr);
}

static void instr_beq(PDP11 *cpu, uint16_t instr)
{
    if (cpu_get_flag(cpu, PSW_Z))
        do_branch(cpu, instr);
}

static void instr_bne(PDP11 *cpu, uint16_t instr)
{
    if (!cpu_get_flag(cpu, PSW_Z))
        do_branch(cpu, instr);
}

static void instr_bpl(PDP11 *cpu, uint16_t instr)
{
    if (!cpu_get_flag(cpu, PSW_N))
        do_branch(cpu, instr);
}

static void instr_bmi(PDP11 *cpu, uint16_t instr)
{
    if (cpu_get_flag(cpu, PSW_N))
        do_branch(cpu, instr);
}

static void instr_bvc (PDP11 *cpu, uint16_t instr){
    if (!cpu_get_flag(cpu, PSW_V))
    do_branch(cpu,instr);
}

static void instr_bvs (PDP11 *cpu, uint16_t instr){
    if (cpu_get_flag(cpu, PSW_V))
    do_branch(cpu,instr);
}

static void instr_bcc (PDP11 *cpu, uint16_t instr){
    if (!cpu_get_flag(cpu, PSW_C))
        do_branch(cpu, instr);
}

static void instr_bcs (PDP11 *cpu, uint16_t instr){
    if (cpu_get_flag(cpu, PSW_C))
        do_branch(cpu, instr);
}


static void instr_bge (PDP11 *cpu, uint16_t instr){
    int n = cpu_get_flag(cpu, PSW_N);
    int v = cpu_get_flag(cpu, PSW_V);

    if ((n ^ v) == 0)
        do_branch(cpu,instr);
}

static void instr_blt (PDP11 *cpu, uint16_t instr){
    int n = cpu_get_flag(cpu, PSW_N);
    int v = cpu_get_flag(cpu, PSW_V);

    if ((n ^ v) == 1)
        do_branch(cpu,instr);
}


static void instr_bgt (PDP11 *cpu, uint16_t instr){
    int n = cpu_get_flag(cpu, PSW_N);
    int v = cpu_get_flag(cpu, PSW_V);
    int z = cpu_get_flag(cpu, PSW_Z);

    if ((n ^ v) == 0 && z == 0)
        do_branch(cpu, instr);
}

static void instr_ble (PDP11 *cpu, uint16_t instr){
    int n = cpu_get_flag(cpu, PSW_N);
    int v = cpu_get_flag(cpu, PSW_V);
    int z = cpu_get_flag(cpu, PSW_Z);

    if ((n ^ v) == 1 || z == 1)
        do_branch(cpu, instr);
}

static void instr_bhi (PDP11 *cpu, uint16_t instr){
    int c = cpu_get_flag (cpu, PSW_C);
    int z = cpu_get_flag (cpu, PSW_Z);

    if (c == 0 && z == 0)
        do_branch(cpu, instr);
}

static void instr_blos (PDP11 *cpu, uint16_t instr){
    int c = cpu_get_flag( cpu, PSW_C);
    int z = cpu_get_flag (cpu, PSW_Z);

    if (c == 1 || z == 1)
        do_branch(cpu, instr);
}

/* ================ управление флагами ================ */

static void instr_flags (PDP11 *cpu, uint16_t instr){
    int set = (instr >> 4) & 1;

    if (set) {
        if (instr & 0x01) cpu_set_flag (cpu, PSW_C);
        if (instr & 0x02) cpu_set_flag (cpu, PSW_V);
        if (instr & 0x04) cpu_set_flag (cpu, PSW_Z);
        if (instr & 0x08) cpu_set_flag (cpu, PSW_N);
    } else {
        if (instr & 0x01) cpu_clear_flag (cpu, PSW_C);
        if (instr & 0x02) cpu_clear_flag (cpu, PSW_V);
        if (instr & 0x04) cpu_clear_flag (cpu, PSW_Z);
        if (instr & 0x08) cpu_clear_flag (cpu, PSW_N);
    }
}

/* ================ прерывания ================ */

static void instr_trap (PDP11 *cpu, uint16_t instr){
   (void) instr;
   
    cpu -> reg[SP] -= 2;
    mem_write_word(cpu, cpu -> reg[SP], cpu -> psw);

    cpu -> reg[SP] -= 2;
    mem_write_word(cpu, cpu -> reg[SP], cpu -> reg[PC]);

    cpu -> reg[PC] = mem_read_word(cpu, 034);
    cpu -> psw = mem_read_word(cpu, 036);
}

static void instr_emt (PDP11 *cpu, uint16_t instr){
    (void)instr;
    
    cpu -> reg[SP] -= 2;
    mem_write_word (cpu, cpu -> reg[SP], cpu -> psw);

    cpu -> reg[SP] -= 2;
    mem_write_word (cpu, cpu -> reg [SP], cpu -> reg[PC]);

    cpu -> reg[PC] = mem_read_word(cpu, 030);
    cpu -> psw = mem_read_word (cpu, 032);
}

static void instr_rti(PDP11 *cpu, uint16_t instr)
{
    (void)instr;

    cpu->reg[PC] = mem_read_word(cpu, cpu->reg[SP]);
    cpu->reg[SP] += 2;

    cpu->psw = mem_read_word(cpu, cpu->reg[SP]);
    cpu->reg[SP] += 2;
}

/* ================ JSR / RTS ================ */

static void instr_jsr(PDP11 *cpu, uint16_t instr)
{
    int linkr = (instr >> 6) & 7;
    int dm = INSTR_DST_MODE(instr);
    int dr = INSTR_DST_REG(instr);

    uint16_t dst_addr = resolve_dst_addr(cpu, dm, dr);

    cpu->reg[SP] -= 2;
    mem_write_word(cpu, cpu->reg[SP], cpu->reg[linkr]);

    cpu->reg[linkr] = cpu->reg[PC];
    cpu->reg[PC] = dst_addr;
}

static void instr_rts(PDP11 *cpu, uint16_t instr)
{
    int reg = instr & 7;

    cpu->reg[PC] = cpu->reg[reg];
    cpu->reg[reg] = mem_read_word(cpu, cpu->reg[SP]);
    cpu->reg[SP] += 2;
}

/* ================ HALT ================ */

static void instr_halt(PDP11 *cpu, uint16_t instr)
{
    (void)instr;
    printf("HALT\n");
    cpu->running = 0;
}

/* ================================================================
 *  Диспетчер
 * ================================================================ */

void execute(PDP11 *cpu, uint16_t instr)
{
    uint16_t opcode;

    /* HALT */ 
    if (instr == 0000000) {
        instr_halt(cpu, instr);
        return;
    }

    if (instr == 0000002){
        instr_rti (cpu, instr);
    }
 
    if (instr >= 0000240 && instr <= 0000277) {
        instr_flags(cpu, instr);
        return;
    }

    if (((instr >> 9) & 0177) == 0077){
        instr_sob(cpu, instr);
        return;
    }

    /* Двухоперандные: биты 15-12 */
    opcode = (instr >> 12) & 0xF;
    switch (opcode) {
    case 001: instr_mov(cpu, instr); return;
    case 002: instr_cmp(cpu, instr); return;
    case 003: instr_bit(cpu, instr); return;
    case 004: instr_bic(cpu, instr); return;
    case 005: instr_bis(cpu, instr); return;
    case 006: instr_add(cpu, instr); return;
    case 016: instr_sub(cpu, instr); return;
    }

    /* Однооперандные: биты 15-6 */
    opcode = (instr >> 6) & 0x3FF;
    switch (opcode) {
    case 00001: instr_jmp(cpu, instr);  return;
    case 00003: instr_swab(cpu, instr); return;
    case 00050: instr_clr(cpu, instr);  return;
    case 00051: instr_com(cpu, instr);  return;
    case 00052: instr_inc(cpu, instr);  return;
    case 00053: instr_dec(cpu, instr);  return;
    case 00054: instr_neg(cpu, instr);  return;
    case 00055: instr_adc(cpu, instr);  return;
    case 00056: instr_sbc(cpu, instr); return;
    case 00057: instr_tst(cpu, instr);  return;
    case 00060: instr_ror(cpu, instr);  return;
    case 00061: instr_rol(cpu, instr);  return;
    case 00062: instr_asr(cpu, instr);  return;
    case 00063: instr_asl(cpu, instr);  return;
    }

    /* JSR */
    if ((instr & 0xFE00) == 0x0800) {
        instr_jsr(cpu, instr);
        return;
    }

    /* RTS */
    if ((instr & 0xFFF8) == 000200) {
        instr_rts(cpu, instr);
        return;
    }

    /* Переходы: биты 15-8 */
    opcode = instr >> 8;
    switch (opcode) {
    case 0001: instr_br(cpu, instr);   return;
    case 0002: instr_bne(cpu, instr);  return;
    case 0003: instr_beq(cpu, instr);  return;
    case 0004: instr_bge(cpu, instr);  return;
    case 0005: instr_blt(cpu, instr);  return;
    case 0006: instr_bgt(cpu, instr);  return;
    case 0007: instr_ble(cpu, instr);  return;
    case 0200: instr_bpl(cpu, instr);  return;
    case 0201: instr_bmi(cpu, instr);  return;
    case 0202: instr_bhi(cpu, instr);  return;
    case 0203: instr_blos(cpu, instr); return;
    case 0204: instr_bvc(cpu, instr);  return;
    case 0205: instr_bvs(cpu, instr);  return;
    case 0206: instr_bcc(cpu, instr);  return;
    case 0207: instr_bcs(cpu, instr);  return;
    case 0210: instr_emt(cpu, instr); return;
    case 0211: instr_trap(cpu, instr); return;
}


    printf("Неизвестная инструкция: %06o (PC=%06o)\n",
           instr, (uint16_t)(cpu->reg[PC] - 2));
    cpu->running = 0;
}   