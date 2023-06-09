.sect .text
.extern	__compact_memory
.define	_compact_memory
.align 2

_compact_memory:
	jmp	__compact_memory
