@;==============================================================================
@;
@;	"garlic_itcm_graf.s":	código de rutinas de soporte a la gestión de
@;							ventanas gráficas (versión 2.0)
@;
@;==============================================================================

NVENT	= 16				@; número de ventanas totales
PPART	= 4					@; número de ventanas horizontales o verticales
							@; (particiones de pantalla)
L2_PPART = 2				@; log base 2 de PPART

VCOLS	= 32				@; columnas y filas de cualquier ventana
VFILS	= 24
PCOLS	= VCOLS * PPART		@; número de columnas totales (en pantalla)
PFILS	= VFILS * PPART		@; número de filas totales (en pantalla)

WBUFS_LEN = 68				@; longitud de cada buffer de ventana (64+4)


.section .itcm,"ax",%progbits

	.arm
	.align 2


	.global _gg_escribirLinea
	@; Rutina para escribir toda una linea de caracteres almacenada en el
	@; buffer de la ventana especificada;
	@;Parámetros:
	@;	R0: ventana a actualizar (int v)
	@;	R1: fila actual (int f)
	@;	R2: número de caracteres a escribir (int n)
_gg_escribirLinea:
	push {r3-r8, lr}
@;desplazamiento de ventanas = (((v/PPART)*PCOLS*VFILS)+(v%PPART)*VCOLS);

	push {r0}
	bl _gg_calcularPosVentana
	mov r3, r0
	pop {r0}
	mov r5, #PCOLS
	mul r4, r1, r5
	mov r4, r4, lsl #1
	add r3, r4
	ldr r4, =_gd_wbfs
	mov r6, #WBUFS_LEN		@; Tamaño de 1 posicion del buffer
	mul r5, r0, r6			@; Desplazamiento dentro del buffer hasta la ventana deseada
	add r4, r5				@; Posicion inicial del buffer de nuestra ventana
	add r4, #4				@; R4 = variable pChars
	mov r5, #0				@; R5 = Contador del bucle
	cmp r2, r5
	beq .fi_while
.while_charLeft:
	ldrh r6, [r4]			@; Cargar valor en la variable pChars, en F2 se necesita 16b para enumerar las baldosas
	sub r6, #32				@; Pasar de ASCII a codigo baldosa.
	strh r6, [r3]			@; Transferir el valor a la posición correspondiente. 16b
	add r5, #1				@; Actualizar variable de control de chars restantes.
	add r4, #2				@; Sumar 2 posición a ptr de pChars, va de 2 bytes en 2 bytes
	add r3, #2				@; Sumar 2 posiciones al ptr de la fila (cada baldosa son 2B).
	cmp r2, r5
	bne .while_charLeft
.fi_while:
	pop {r3-r8, pc}



	.global _gg_desplazar
	@; Rutina para desplazar una posición hacia arriba todas las filas de la
	@; ventana (v), y borrar el contenido de la última fila
	@;Parámetros:
	@;	R0: ventana a desplazar (int v)
_gg_desplazar:
	push {r1-r12,lr}
	push {r0}
	bl _gg_calcularPosVentana
	mov r1, r0
	pop {r0}
	
	mov r3, #PCOLS
	mov r2, #1				@; R2 = Contador de filas
	mov r4, #0				@; R4 = Contador de columnas
	mov r11, #0				@; R11 = Offset para mover el puntero de la fila
	mov r12, #0				@; R12 = baldosa vacía (0)
.desplazar_lineas:
	mov r4, #0
	mov r11, #0
	sub r5, r2, #1			@; Fila anterior donde copiar los elementos
	mul r6, r5, r3			
	mov r6, r6, lsl #1				@; Desplazamiento para llegar desde ptr de ventana a ventana anterior.
	mul r7, r2, r3			
	mov r7, r7, lsl #1				@; Desplazamiento para llegar a la fila actual.
	add r6, r1				@; R6 = puntero a fila anterior
	add r7, r1				@; R7 = puntero a fila actual
	.copiar_linea:
	cmp r4, #VCOLS
	bhs .final_linea
	ldrh r8, [r7, r11]		@; Cargar valor de fila actual y guardarlo en la anterior.
	strh r8, [r6, r11]
	strh r12, [r7, r11]		@; Guardar baldosa vacía
	add r4, #1
	add r11, #2
	b .copiar_linea
	.final_linea:
	add r2, #1				@; Actualizar fila a la siguiente
	cmp r2, #VFILS
	bls .desplazar_lineas
.final_desplazar:
	pop {r1-r12,pc}



	.global _gg_escribirLineaTabla
	@; escribe los campos básicos de una linea de la tabla correspondiente al
	@; zócalo indicado por parámetro con el color especificado; los campos
	@; son: número de zócalo, PID, keyName y dirección inicial
	@;Parámetros:
	@;	R0 (z)		->	número de zócalo
	@;	R1 (color)	->	número de color (de 0 a 3)
	@;
	@;typedef struct			// Estructura del bloque de control de un proceso
	@;{							// (PCB: Process Control Block)
	@;	int PID;				//	identificador del proceso (Process IDentifier)
	@;	int PC;					//	contador de programa (Program Counter)
	@;	int SP;					//	puntero al top de pila (Stack Pointer)
	@;	int Status;				//	estado del procesador (CPSR)
	@;	int keyName;			//	nombre en clave del proceso (cuatro chars)
	@;	int workTicks;			//	contador de ciclos de trabajo (24 bits bajos)
	@;							//		8 bits altos: uso de CPU (%)
	@;} PACKED garlicPCB;
	@;	extern garlicPCB _gd_pcbs[16];	// vector con los PCBs de los procesos activos
	@;
	@;	extern void _gs_escribirStringSub(char *string, int fil, int col, int color);
	@; _gs_num2str_dec(*ptr, length, num)
	@;	const char 4_espacios[]="    ";
	@;	const char 8_espacios[]="        ";

_gg_escribirLineaTabla:
	push {r0-r3, r11, r12, lr}
	mov r12, r0				@; R12 = Zocalo
	mov r11, r1				@; R11 = Color
	ldr r0, =_gd_pcbs		@; Cargar puntero a array de struct de garlicPCB
	mov r1, #24				@; Tamaño por PCB = 6*32b
	mul r1, r12
	add r10, r0, r1			@; R10 = Ptr a struct de PCB del proceso
	@; Mirar si es el proceso de S.O.
	ldr r0, [r10, #0]		@; Cargar el PID
	cmp r0, #0
	bne .escribir
	@; Mirar si es el zocalo 0
	cmp r12, #0
	beq .escribir			@; Es el proc de S.O, hay que escribir
	
	@; PID == 0, Z != 0 ==> Proceso acabado, borrar el contenido del zocalo
	ldr r0, =espacios4		@; Cargar 4 espacios para la funcion de _gs_escribirStringSub
	add r1, r12, #4			@; Offset de 4 filas, La info del zocalo 0 se escribe en la fila 4
	mov r2, #4				@; Offset para llegar a la columna de PID (4)
	mov r3, r11				@; Color
	bl _gs_escribirStringSub
	mov r2, #9				@; Offset para llegar al keyname
	bl _gs_escribirStringSub
	b .escribir_z
	
	@; Escribir info de PID y Keyname
	.escribir:
	ldr r0, =buffer			@; Cargar bufer para guardar el string de PID
	mov r1, #4				@; tamaño
	ldr r2, [r10, #0]		@; PID
	bl _gs_num2str_dec
	ldr r0, =buffer
	add r1, r12, #4			@; Offset para fila de PID
	mov r2, #5				@; Offset para columna del PID
	mov r3, r11				@; Color
	bl _gs_escribirStringSub
	
	@; Escribir keyname
	add r0, r10, #16		@; Cargar ptr a keyname
	add r1, r12, #4			@; Ir a la fila del z
	mov r2, #9				@; Col de keyname
	mov r3, r11				@; Color
	bl _gs_escribirStringSub
	
	@; Escribir info de Zocalo
	.escribir_z:
	ldr r0, =buffer
	mov r1, #3				@; max zocalo = 16, +1 de centinela
	mov r2, r12				@; Zocalo
	bl _gs_num2str_dec
	ldr r0, =buffer
	add r1, r12, #4
	mov r2, #1				@; Offset de zocalo
	mov r3, r11				@; Color
	bl _gs_escribirStringSub

	pop {r0-r3, r11, r12, lr}


	.global _gg_escribirCar
	@; escribe un carácter (baldosa) en la posición de la ventana indicada,
	@; con un color concreto;
	@;Parámetros:
	@;	R0 (vx)		->	coordenada x de ventana (0..31)
	@;	R1 (vy)		->	coordenada y de ventana (0..23)
	@;	R2 (car)	->	código del caràcter, como número de baldosa (0..127)
	@;	R3 (color)	->	número de color del texto (de 0 a 3)
	@; pila (vent)	->	número de ventana (de 0 a 15)
_gg_escribirCar:
	push {r0-r7, lr}
	ldr r4, [sp, #9*4]		@; cargar el parámetro según el num de registros del push. 1 reg = 32b
	push {r0}
	mov r0, r4
	bl _gg_calcularPosVentana
	mov r5, r0				@; pos 0,0 de ventana indicada
	pop {r0}
	
	mov r6, #PCOLS*2
	mla r5, r1, r6, r5		@; fila * desplazamiento por fila (32 pos. * 2) + ptr ventana.
	mov r6, r0, lsl #1		@; movimientos en eje x * 2
	add r5, r6				@; R5 = posicion del caracter a escribir
	
	mov r6, r3, lsl #7		@; num baldosas totales (de 1 color)
	add r2, r6				@; caracter a escribir
	strh r2, [r5]			@; Guardar Hword en la posicion.
	pop {r0-r7, pc}
	
	
	.global _gg_escribirMat
	@; escribe una matriz de 8x8 carácteres a partir de una posición de la
	@; ventana indicada, con un color concreto;
	@;Parámetros:
	@;	R0 (vx)		->	coordenada x inicial de ventana (0..31)
	@;	R1 (vy)		->	coordenada y inicial de ventana (0..23)
	@;	R2 (m)		->	puntero a matriz 8x8 de códigos ASCII (dirección)
	@;	R3 (color)	->	número de color del texto (de 0 a 3)
	@; pila	(vent)	->	número de ventana (de 0 a 15)
	@; Contador x = R11, y = R12
_gg_escribirMat:
	push {r0-r12, lr}
	ldr r4, [sp, #14*4]		@; cargar el parámetro según el num de registros del push. 1 reg = 32b
	push {r0}
	mov r0, r4
	bl _gg_calcularPosVentana
	mov r5, r0				@; R5 = pos 0,0 de ventana indicada
	pop {r0}
	
	mov r3, r3, lsl #7		@; suma de posiciones para el color
	mov r6, #PCOLS			@; R6 = PCOLS * 2
	mul r7, r6, r1			@; fil * PCOLS
	add r7, r5, r7, lsl #1
	mov r8, r0, lsl #1		@; movimientos en eje x * 2
	add r5, r8, r7			@; r5 = Puntero a la pos 0,0 de la matriz de 8x8
	
	mov r11, #0				@; inicializar contador de posicion x
	mov r12, #0				@; inicializar contador de matriz
	
	.inicio_bucle:
		cmp r11, #7
		ble .controlador_x
		mov r11, #0
		cmp r12, #63
		bgt .fin_bucle
		add r5, r6, lsl #1	@; sumar 1 fila
		sub r5, #16			@; restart 8 posiciones a la x
		.controlador_x:
		
		ldrb r7, [r2]
		cmp r7, #0
		beq .no_imprimir
		sub r7, #32
		add r7, r3
		strh r7, [r5]
		
		.no_imprimir:
		add r5, #2
		add r11, #1
		add r12, #1
		add r2, #1
		b .inicio_bucle
	.fin_bucle:
	pop {r0-r12, pc}

	.global _gg_calcularPosVentana
	@; R0 -> num ventana
	@; Return -> Dir. de la pos 0,0 de la ventana en el mapa
_gg_calcularPosVentana:
	push {r1-r5, lr}
	and r1, r0, #3				@; v%PPART
	mov r2, r0, lsr #L2_PPART	@; v/PPART
	mov r3, #PCOLS*VFILS
	mov r5, #VCOLS
	mul r4, r2, r3			@; (v/PPART)*PCOLS*VFILS
	mla r2, r1, r5, r4		@; (v/PPART)*PCOLS*VFILS+(v%PPART)*VCOLS
	mov r2, r2, lsl #1		@; Desplazamiento de ventanas * 2: Cada baldosa son 2 bytes, cada posicion de memoria es 1 byte
	ldr r1, =map2ptr		@; Cargar variable con el puntero a mapa
	ldr r1, [r1]			@; Cargar dir del mapa
	add r0, r1, r2			@; Sumar posiciones a mover desde la pos 0,0 del mapa a la pos 0,0 de la ventana
	pop {r1-r5, pc}

	.global _gg_rsiTIMER2
	@; Rutina de Servicio de Interrupción (RSI) para actualizar la representa-
	@; ción del PC actual.
_gg_rsiTIMER2:
	push {r0-r5, lr}
	mov r5, #0				@; Contador de bucle / zocalo
	ldr r4, =_gd_pcbs		@; Cargar puntero a array de struct de garlicPCB
	
.inicio_bucle_rsi:
	cmp r5, #16
	bhs	.fin_bucle_rsi
	@; Mirar si PID ==0, si es así saltar a siguiente posicion de PCBs
	ldr r0, [r4, #0]
	cmp r0, #0
	bne .escribir_pc
	@; Comparar zocalo
	cmp r5, #0
	beq .escribir_pc
	
	
	@; Borrar PC Anterior
	ldr r0, =espacios8
	add r1, r5, #4
	mov r2, #14				@; Offset para la columna de PC
	mov r3, #0				@; Color blanco
	bl _gs_escribirStringSub
	b .siguiente_iteracion_rsi
	
.escribir_pc:
	@; Escribir nuevo PC
	ldr r0, =buffer
	mov r1, #9				@; tamaño maximo de PC = 8
	ldr r2, [r4, #4]		@; Cargar el valor del PC
	bl _gs_num2str_hex
	ldr r0, =buffer
	add r1, r5, #4
	mov r2, #14				@; Offset para la columna de PC
	mov r3, #0				@; Color blanco
	bl _gs_escribirStringSub

.siguiente_iteracion_rsi:
	add r4, #24
	add r5, #1
	b .inicio_bucle_rsi
.fin_bucle_rsi:

	pop {r0-r5, pc}

.end

