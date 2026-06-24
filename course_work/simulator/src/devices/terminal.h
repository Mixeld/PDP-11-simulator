#ifndef PDP11_TERMINAL_H
#define PDP11_TERMINAL_H

#include <stdint.h>


// Адреса регистров терминала в памяти 

#define TKS  0177560    // статус клавиатуры   
#define TKB  0177562    /* Keyboard Buffer Register   */
#define TPS  0177564    /* Printer Status Register    */
#define TPB  0177566    /* Printer Buffer Register    */


#define TERMINAL_READY  0x80    /* бит 7: устройство готово */


void terminal_init (void);
void terminal_cleanup(void);
uint16_t terminal_read_tks(void);
uint16_t terminal_read_tkb (void);
uint16_t terminal_read_tps(void);
void terminal_write_tpb(uint16_t val);
int terminal_is_io_address(uint16_t addr);

#endif