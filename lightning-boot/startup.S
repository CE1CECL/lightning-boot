#include "include/autoconf.h"

.section .init
.arm
.align 0

.global StartUp

.org	StartUp

ResetHandler:
	ldr		sp,	=__stack_start__
#ifdef SELF_BOOTSTRAP
	bl		bootstrap
#endif
	b		main

.end
