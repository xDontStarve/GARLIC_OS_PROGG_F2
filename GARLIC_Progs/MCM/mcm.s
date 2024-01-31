	.arch armv5te
	.eabi_attribute 23, 1
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 1
	.eabi_attribute 30, 6
	.eabi_attribute 34, 0
	.eabi_attribute 18, 4
	.file	"MCM.c"
	.section	.rodata
	.align	2
.LC0:
	.ascii	"(%d) \000"
	.align	2
.LC1:
	.ascii	"%2El minimo comun divisor de %0%d %3y %0%d%0 es: \000"
	.align	2
.LC2:
	.ascii	"%d\012\000"
	.align	2
.LC3:
	.ascii	"%1*%q*\012\000"
	.align	2
.LC4:
	.ascii	"%2*%Q*\012\000"
	.align	2
.LC5:
	.ascii	"%3*%Q*\012\000"
	.align	2
.LC6:
	.ascii	"%0*%q*\012\000"
	.text
	.align	2
	.global	_start
	.syntax unified
	.arm
	.fpu softvfp
	.type	_start, %function
_start:
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	str	lr, [sp, #-4]!
	sub	sp, sp, #36
	str	r0, [sp, #4]
	ldr	r3, [sp, #4]
	add	r3, r3, #1
	mov	r2, #1000
	mul	r3, r2, r3
	str	r3, [sp, #24]
	mov	r3, #0
	str	r3, [sp, #28]
	b	.L2
.L3:
	mov	r3, #0
	str	r3, [sp, #12]
	mov	r3, #0
	str	r3, [sp, #8]
	bl	GARLIC_random
	mov	r3, r0
	mov	r0, r3
	ldr	r3, [sp, #24]
	add	r3, r3, #1
	mov	r1, r3
	add	r3, sp, #8
	add	r2, sp, #12
	bl	GARLIC_divmod
	ldr	r3, [sp, #8]
	str	r3, [sp, #20]
	bl	GARLIC_random
	mov	r3, r0
	mov	r0, r3
	ldr	r3, [sp, #24]
	add	r3, r3, #1
	mov	r1, r3
	add	r3, sp, #8
	add	r2, sp, #12
	bl	GARLIC_divmod
	ldr	r3, [sp, #8]
	str	r3, [sp, #16]
	ldr	r1, [sp, #28]
	ldr	r0, .L5
	bl	GARLIC_printf
	ldr	r2, [sp, #16]
	ldr	r1, [sp, #20]
	ldr	r0, .L5+4
	bl	GARLIC_printf
	ldr	r1, [sp, #16]
	ldr	r0, [sp, #20]
	bl	minimo_comun_multiple
	mov	r3, r0
	mov	r1, r3
	ldr	r0, .L5+8
	bl	GARLIC_printf
	ldr	r3, [sp, #28]
	add	r3, r3, #1
	str	r3, [sp, #28]
.L2:
	ldr	r3, [sp, #28]
	cmp	r3, #49
	ble	.L3
	mov	r2, #0
	ldr	r1, .L5+12
	ldr	r0, .L5+16
	bl	GARLIC_printf
	mov	r2, #0
	ldr	r1, .L5+12
	ldr	r0, .L5+20
	bl	GARLIC_printf
	mov	r2, #0
	ldr	r1, .L5+24
	ldr	r0, .L5+28
	bl	GARLIC_printf
	mov	r2, #0
	mov	r1, #-2147483648
	ldr	r0, .L5+32
	bl	GARLIC_printf
	mov	r2, #0
	mvn	r1, #0
	ldr	r0, .L5+20
	bl	GARLIC_printf
	mov	r3, #0
	mov	r0, r3
	add	sp, sp, #36
	@ sp needed
	ldr	pc, [sp], #4
.L6:
	.align	2
.L5:
	.word	.LC0
	.word	.LC1
	.word	.LC2
	.word	-2147475457
	.word	.LC3
	.word	.LC4
	.word	2147481600
	.word	.LC5
	.word	.LC6
	.size	_start, .-_start
	.align	2
	.global	maximo_comun_divisor
	.syntax unified
	.arm
	.fpu softvfp
	.type	maximo_comun_divisor, %function
maximo_comun_divisor:
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	str	lr, [sp, #-4]!
	sub	sp, sp, #28
	str	r0, [sp, #4]
	str	r1, [sp]
	b	.L8
.L9:
	ldr	r3, [sp]
	str	r3, [sp, #20]
	mov	r3, #0
	str	r3, [sp, #16]
	mov	r3, #0
	str	r3, [sp, #12]
	ldr	r0, [sp, #4]
	ldr	r1, [sp]
	add	r3, sp, #12
	add	r2, sp, #16
	bl	GARLIC_divmod
	ldr	r3, [sp, #12]
	str	r3, [sp]
	ldr	r3, [sp, #20]
	str	r3, [sp, #4]
.L8:
	ldr	r3, [sp]
	cmp	r3, #0
	bne	.L9
	ldr	r3, [sp, #4]
	mov	r0, r3
	add	sp, sp, #28
	@ sp needed
	ldr	pc, [sp], #4
	.size	maximo_comun_divisor, .-maximo_comun_divisor
	.align	2
	.global	minimo_comun_multiple
	.syntax unified
	.arm
	.fpu softvfp
	.type	minimo_comun_multiple, %function
minimo_comun_multiple:
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	str	lr, [sp, #-4]!
	sub	sp, sp, #28
	str	r0, [sp, #4]
	str	r1, [sp]
	mov	r3, #0
	str	r3, [sp, #12]
	mov	r3, #0
	str	r3, [sp, #8]
	ldr	r3, [sp, #4]
	ldr	r2, [sp]
	mul	r3, r2, r3
	str	r3, [sp, #20]
	ldr	r1, [sp]
	ldr	r0, [sp, #4]
	bl	maximo_comun_divisor
	str	r0, [sp, #16]
	add	r3, sp, #8
	add	r2, sp, #12
	ldr	r1, [sp, #16]
	ldr	r0, [sp, #20]
	bl	GARLIC_divmod
	ldr	r3, [sp, #12]
	mov	r0, r3
	add	sp, sp, #28
	@ sp needed
	ldr	pc, [sp], #4
	.size	minimo_comun_multiple, .-minimo_comun_multiple
	.ident	"GCC: (devkitARM release 46) 6.3.0"
