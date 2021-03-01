#ifndef Z80_H
#define Z80_H

#include <stdlib.h>
#include <stdint.h>

#include <sys/cdefs.h>

__BEGIN_DECLS

typedef struct Z80_instance
{
	uint16_t PCX; /* external view of PC                        */
	int32_t AF; /* AF register                                  */
	int32_t BC; /* BC register                                  */
	int32_t DE; /* DE register                                  */
	int32_t HL; /* HL register                                  */
	int32_t IX; /* IX register                                  */
	int32_t IY; /* IY register                                  */
	uint16_t PC; /* program counter                             */
	int32_t SP; /* SP register                                  */
	int32_t AF1; /* alternate AF register                       */
	int32_t BC1; /* alternate BC register                       */
	int32_t DE1; /* alternate DE register                       */
	int32_t HL1; /* alternate HL register                       */
	int32_t IFF; /* Interrupt Flip Flop                         */
	int32_t IR; /* Interrupt (upper) / Refresh (lower) register */
	int32_t Status; /* Status of the CPU 0=running 1=end request 2=back to CCP */
	int32_t Debug;
	int32_t Break;
	int32_t Step;
	uint8_t* ram;
} Z80_instance_t;

void Z80_reset(Z80_instance_t*);

void Z80_step(Z80_instance_t*);

__END_DECLS

#endif /* Z80_H */
