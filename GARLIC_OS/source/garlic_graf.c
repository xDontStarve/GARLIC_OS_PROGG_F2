/*------------------------------------------------------------------------------

	"garlic_graf.c" : fase 2 / programador G

	Funciones de gestiÃ³n de las ventanas de texto (grÃ¡ficos), para GARLIC 2.0

------------------------------------------------------------------------------*/
#include <nds.h>

#include <garlic_system.h>	// definiciÃ³n de funciones y variables de sistema
#include <garlic_font.h>	// definiciÃ³n grÃ¡fica de caracteres

/* definiciones para realizar cÃ¡lculos relativos a la posiciÃ³n de los caracteres
	dentro de las ventanas grÃ¡ficas, que pueden ser 4 o 16 */
#define NVENT	16				// nÃºmero de ventanas totales
#define PPART	4				// nÃºmero de ventanas horizontales o verticales
								// (particiones de pantalla)
#define VCOLS	32				// columnas y filas de cualquier ventana
#define VFILS	24
#define PCOLS	VCOLS * PPART	// nÃºmero de columnas totales (en pantalla)
#define PFILS	VFILS * PPART	// nÃºmero de filas totales (en pantalla)


const unsigned int char_colors[] = {240, 96, 64};	// amarillo, verde, rojo
const char espacios4[]="    ";
const char espacios8[]="        ";
char buffer[9];

int fondo2, fondo3, map2ptr;
/* _gg_generarMarco: dibuja el marco de la ventana que se indica por parÃ¡metro,
												con el color correspondiente */
void _gg_generarMarco(int v, int color)
{
	if (v>PPART*PPART) return;
	//Posicionar en la posiciÃ³n correcta del mapa
	//fila de ventana (0-(PPART-1)) * tamaÃ±o de una fila de ventanas (PCOLS*VFILS) + Desplazamiento horizontal (columna de ventana * tam. de 1 ventana).
	u16 * windowPointer = bgGetMapPtr(fondo3)+(((v/PPART)*PCOLS*VFILS)+(v%PPART)*VCOLS);
	for (int i=1; i<VCOLS-1; i++){
		*(windowPointer+i)=99+128*color;										//Norte: base + desplazamiento columnas
		*(windowPointer+(i+(VFILS-1)*PCOLS))=97+128*color;					//Sud: base + posiciones hasta ultima fila + desplazamiento columnas
		if (i<24)
		{	//Hay menos filas que columnas.
		*(windowPointer+(i*PCOLS))=96+128*color;								//Oeste: base + i*total de columnas (mapa[i][0]) == desplazar i filas hacia abajo desde la base
		*(windowPointer+(i*PCOLS)+VCOLS-1)=98+128*color;						//Este: Oeste + Columnas hasta la ultima columna de la ventana
		}
	}
	*windowPointer=103+128*color;												//Noroeste
	*(windowPointer+VCOLS-1)=102+128*color;									//Noreste
	*(windowPointer+((VFILS-1)*PCOLS))=100+128*color;							//Sudoeste
	*(windowPointer+((VFILS-1)*PCOLS+VCOLS-1))=101+128*color;					//Sudeste
}


/* _gg_iniGraf: inicializa el procesador grÃ¡fico A para GARLIC 2.0 */
void _gg_iniGrafA()
{
	videoSetMode(MODE_5_2D);				//Mode 2D para fondos
	lcdMainOnTop();
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	//bgInit(int layer, BgType type, BgSize size, int mapBase, int tileBase), devuelve el  indice de fondo.
	//Por defecto los mapas de la NDS son de 32x32 baldosas, y ocupan 2 Kbytes. La pantalla es de 256x192 pixeles.
	fondo2 = bgInit(2, BgType_ExRotation, BgSize_ER_1024x1024, 0, 4); //128x128 baldosas = 128x128x2B=32KB por mapa.
	fondo3 = bgInit(3, BgType_ExRotation, BgSize_ER_1024x1024, 16, 4); //16X2KB = 32KB, 4*16KB = 64KB
	map2ptr = (int) bgGetMapPtr(fondo2);
		/* MapBase indica el indice del mapa, en nuestro caso cada mapa ocupa 32768Bytes, ya que en 1024x1024 pixeles hay
		   128x128 baldosas y cada baldosa ocupa 2Bytes en memoria (guarda el Ã­ndice de baldosa en 2Bytes).
		   Podemos reutilizar TileBase porque son las mismas baldosas para cualquier fondo.
		   
		   En esta organizacion de virtual VRAM quedaria una estructura asi:
		   Base fondo2 = 0x06000000
		   Base fondo3 = 0x06008000
		   Base Baldosas = 0x06010000
		*/
	//Rango de prioridad = 0-3, menor* significa mÃ¡s prioridad
	bgSetPriority(fondo2, 2);
	bgSetPriority(fondo3, 1);
	//		  data to decompress, destination, decompress type.
	decompress(garlic_fontTiles, bgGetGfxPtr(fondo3), LZ77Vram); 
	/*Hay que indicar el sitio donde dejar las grÃ¡ficas, en este caso como los dos fondos comparten tiles,
	  Los podemos dejar en la zona de memoria para tiles del fondo3.
	  BG_PALETTE=((u16*)0x05000000)*/
	dmaCopy(garlic_fontPal, BG_PALETTE, sizeof(garlic_fontPal));
	
	
	/*
		Cada baldosa contiene 8x8 pixeles, para cada pixel se guarda 8 bits para la posicion dentro de la paleta (256 colores).
		Por lo tanto, en memoria (0x06010000), cada baldosa ocupa 64B.
		La base de memoria donde se guarda las baldosas es 0x06010000, la base de la paleta de colores es 0x05000000.
		
		Amarillo = posicion 240 de la paleta de colores = 0xF0
		Verde = posicion 96 de la paleta de colores = 0x60
		Rojo = posicion 64 de la paleta de colores = 0x40
		Blanco = posicion 256 de la paleta de colores = 0xFF
		
		Los accesos a VRAM son de 2B por acceso. Por lo tanto para modificar los colores habrÃ¡ que tener en cuenta todas posiciones dentro
		 de una carga (que contiene 2 colores, por ejemplo 0x00FF, 00 indicando negro y FF blanco).
		  Habra 3 casos posibles por carga, 2x 1 pixel blanco (lado izquiero y derecho) o 2 pixeles blancos (0xFFFF):
			- 0xFF00, 0x00FF, 0xFFFF.
		Tamano total de las baldosas en la VRAM:
			128 baldosas en total * 64B por baldosa = 8192B en total por una copia de las baldosas en 1 color
				==> 4096 accesos en total (8192B / 2B)
	*/
	
	u16* tile_base = bgGetGfxPtr(fondo3);
	for (int color=1; color<=3; color++){
		for (int i=0; i<4096; i++){
			switch (tile_base[i]){
				case 0x00FF:
					tile_base[4096*color+i]=(u16)char_colors[color-1];
					break;
				case 0xFF00:
					tile_base[4096*color+i]=(u16)char_colors[color-1]<<8; //Moverlo a la "izquierda" del halfword
					break;
				case 0xFFFF:
					tile_base[4096*color+i]=(u16)char_colors[color-1]+((u16)char_colors[color-1]<<8); //Actualizar ambos pixeles
					break;
			}
		}
	}
	
	// Generar marcos
	for (int i=0 ; i< NVENT ; i++)
	{
		_gg_generarMarco(i, 3);
	}
	//	para hacer zoom de 50%, hay que dividir los 512x512 entre 2 para caber en la pantalla de NDS (zoom x2).
	bgSetScale(fondo2, 512, 512);// 2 -> 2^1 = 2: 0...10,0000 0000 = 0d512
	bgSetScale(fondo3, 512, 512);// 24b Parte Entera-^ ^-8 bits de decimal
	
	bgUpdate();
}

/*
	_gg_long2str: Transforma un nÃºmero long long de C a una string acabado en \0

	ParÃ¡metros:
		int pointed: 0: No poner puntos cada 3 digitos, 1: Poner puntos cada 3 digitos.
		unsigned int numPtr: Puntero (int) a un numero long long de C
		char* str:	Buffer donde ir guardando el resultado, como mÃ¡ximo se guardarÃ¡ 19 dÃ­gitos.
		
	Resultado -> Mensaje guardado en la string pasado por parametro.
	
	ObservaciÃ³n: Un numero long tiene rango -9,223,372,036,854,775,808 - 9,223,372,036,854,775,807
				No se hace control de tamaño maximo de string en esta funcion.
*/

void _gg_long2str(int pointed, unsigned int numPtr, char* str)
{
	int i=0, j=0, negativo=0;
	long long* num=(long long*)numPtr;		//Convertir el uint a puntero a long
	long long longNum=*num;					//Coger el valor del long.
	char temp;
	int digitCount=0;
	
	if (longNum<0)
	{
		negativo=1;
		longNum=-longNum;						//Tratar el numero en positivo, luego anadir el signo de negativo.
	}
	do {
		if (pointed && (digitCount > 0) && (digitCount%3==0)) {
			str[i++]='.';						//Si pointed es 1 se añaden periodos cada 3 digitos.
		}
		str[i++]=(longNum % 10) + '0';		//Sumar '0' para poner el offset de los numeros en formato ASCII
		longNum/=10;							//Dividir el long de 10 en 10 para coger los valores. 
		digitCount++;
	} while (longNum > 0);
	if (negativo) str[i++]='-';		
	str[i]='\0';								//Añadir centinela.
	
	//Invertir el orden de la string para representarlo correctamente.
	//Si se quiere guardar el nÃºmero en formato string directamente en el orden correcto,
	//se tendria que calcular la longitud del num long long, teniendo en cuenta el signo y los puntos, por lo que resulta mas costoso.
	for (j=0; j<i/2; j++) {
		temp=str[j];
		str[j]=str[i-j-1];
		str[i-j-1]=temp;
	}
}

void add_dots(char *ptr) {
    int length = 0;
    int digitCount = 0;

    //calcular tamaño de digitos
    while (ptr[length]) {
        if (ptr[length] >= '0' && ptr[length] <= '9') {
            digitCount++;
        }
        length++;
    }

    // Calcular nueva longitud
    int newLength=length+(digitCount-1)/3;
    char result[newLength+1]; // centinela
    int j = 0;

    for (int i = 0, count = 0; i < length; ++i) {
        result[j++] = ptr[i];
        if ((ptr[i]!=' ')&&(ptr[i]!='\0')) {
            count++;
            if (count%3 == 0 && i<length-1) {
                result[j++]='.';
            }
        }
    }

    result[newLength] = '\0'; // centinela
	
	//copiar resultados
    for (int i = 0; i <= newLength; ++i) {
        ptr[i] = result[i];
    }
}

void q12_to_str(int pointed, int num, char* ptr){
	int i=0;
	int signo = (num>>31) & 0x0001;				//Ocupa 1 char como maximo
	int entera = (num >> 12);	//Maximo ocupa 6 chars, incluyendo el punto
	if (entera < 0) {entera=-entera;}	//Si el bit de signo es 1, se iria desplazando 1's cuando se hace >> 12
	int decimal = num & 0x0fff;			//
	
	char buffer[8];
	_gs_num2str_dec(buffer, 8, entera & 0x7ffff);
	
	if (signo == 1){
		ptr[i++]='-';
	}
	
	if (pointed){
		add_dots(buffer);
	}
	
	int j=0;
	while (buffer[j]!='\0'){
		if (buffer[j]!=' '){
			ptr[i++]=buffer[j];
		}
		j++;
	}
	
	ptr[i++]=',';
	
	for (int k=0; k<12; ++k) {
        decimal*= 10;
        ptr[i++]= '0'+(decimal>>12);
        decimal &=0x0FFF;
    }
	ptr[i]='\0';
}

/* _gg_procesarFormato: copia los caracteres del string de formato sobre el
					  string resultante, pero identifica los c digos de formato
					  precedidos por '%' e inserta la representaci n ASCII de
					  los valores indicados por par metro.
	Par metros:
		formato	->	string con c digos de formato (ver descripci n _gg_escribir);
		val1, val2	->	valores a transcribir, sean n mero de c digo ASCII (%c),
					un n mero natural (%d, %x) o un puntero a string (%s);
		resultado	->	mensaje resultante.
	Observaci n:
		Se supone que el string resultante tiene reservado espacio de memoria
		suficiente para albergar todo el mensaje, incluyendo los caracteres
		literales del formato y la transcripci n a c digo ASCII de los valores.
*/
void _gg_procesarFormato(char *formato, unsigned int val1, unsigned int val2,
																char *resultado)
{
	//char=1Bytes, %d = 1.32, %x = hexa
	int message_length=0, currentParamNumber=1;
	//Hace falta un contador para saber quÃ© posiciÃ³n de formato coger el carÃ¡cter.
	int i=0;
	char *token=formato, *currentChar;
	/*Mientras no sea el centinela ni se ha pasado de 3 lÃ­neas(sin contar \0), seguir procesando
		Las funciones de sistema para convertir nÃºmeros usa los parÃ¡metros de la siguiente manera:
			- Guarda el nÃºmero decimal (0d) en formato string en el puntero pasado en el primer parÃ¡metro (acabado en \0)
			- El segundo parÃ¡metro es el nÃºmero que bytes (char) que tiene el parÃ¡metro 1
			- El tercer parÃ¡metro es la variable con el nÃºmero natural a transformar
			* Devuelve 0 si se ha podido transformar.
	*/
	while ((token[i]!='\0') && message_length<3*VCOLS)				//+1: que escribir el centinela
	{
		if (token[i]=='%')
		{
			if (currentParamNumber>2)
			{
				//Si ha superado el lÃ­mite de parÃ¡metros, imprimir tal cual el formato
				//Puede que supere el lÃ­mite de 3 lÃ­neas pero se ha guardado 3*VCOLS+1 espacios en la array, por lo cual el Ãºltimo carÃ¡cter se sustituirÃ­a por \0 en escribirlinea
				resultado[message_length++]='%';
				resultado[message_length++]=token[i+1];
				i +=2;													//Se suma 2 porque el formato ocupa 2 ASCII (%c)
			}else
			{
				switch (token[i+1])
				{
					case '0':
					case '1':
					case '2':
					case '3':
					resultado[message_length++]='%';
					resultado[message_length++]=token[i+1];
					i +=2;													//Se suma 2 porque el formato ocupa 2 ASCII (%0,1,2,3)
					break;
				
					case '%':											//Copiar % al formato final
					resultado[message_length]='%';
					message_length++;
					i +=2;												//Se suma 2 porque el formato ocupa 2 ASCII (%%)
					break;
					
					case 'c':											//Copiar el carÃ¡cter ASCII al formato final
					if (currentParamNumber==1) 
					{
						resultado[message_length++]=(char)val1;
						currentParamNumber++;
					}else if (currentParamNumber==02)
					{
						resultado[message_length++]=(char)val2;
						currentParamNumber++;
					}
					i +=2;												//Se suma 2 porque el formato ocupa 2 ASCII (%c)
					break;
					
					case 'd':
					// Sumar nÃºm de carÃ¡cteres segÃºn tamaÃ±o del numero decimal
					if (currentParamNumber<=2)
					{
						int charLeft=10;								// Un int32 ocupara como maximo 10 digitos decimales
						char decimal[charLeft];
						
						if (currentParamNumber == 1)
						{
							_gs_num2str_dec(decimal, charLeft, val1);
							currentParamNumber++;
						} else if (currentParamNumber == 2)
						{
							_gs_num2str_dec(decimal, charLeft, val2);
							currentParamNumber++;
						}
						
						currentChar=decimal;							//puntero para ir iterando sobre la array de chars resultante.
						while (*currentChar==' ') currentChar++;
						while (*currentChar!='\0')
						{
							resultado[message_length++]=*currentChar;
							currentChar++;
						}
					}
					//Si ha superado el lÃ­mite de parÃ¡metros, imprimir tal cual %d
					//Puede que supere el lÃ­mite de 3 lÃ­neas pero se ha guardado 3*VCOLS+1 espacios en la array, por lo cual el Ãºltimo carÃ¡cter se sustituirÃ­a por \0 en escribirlinea
					else
					{
						resultado[message_length++]='%';
						resultado[message_length++]='d';
					}
					i +=2;												//Se suma 2 porque el formato ocupa 2 ASCII (%d)
					break;
					
					case 'x':
					// Sumar nÃºm de carÃ¡cteres segÃºn tamaÃ±o del numero hexa
					{
						int charLeft=8;									// Un int32 ocupara como maximo 8 posiciones hexadecimales
						char hexa[charLeft];
						
						if (currentParamNumber == 1)
						{
							_gs_num2str_hex(hexa, charLeft, val1);
							currentParamNumber++;
						} else if (currentParamNumber == 2)
						{
							_gs_num2str_hex(hexa, charLeft, val2);
							currentParamNumber++;
						}
						
						currentChar=hexa;								//puntero para ir iterando sobre la array de chars resultante.
						//controlar que no se introduzca los carÃ¡cteres del inicio, a no ser que solo queden 2 espacios en el buffer, en ese caso puede que el nÃºmero sea '0'.
						while ((*currentChar=='0') && (charLeft>2))  currentChar++;
						while (*currentChar!='\0')
						{
							resultado[message_length++]=*currentChar;
							currentChar++;
						}
					}
					i +=2;												//Se suma 2 porque el formato ocupa 2 ASCII (%x)
					break;
					
					case 's':											//Copiar la string al formato final
					{
						char *currentChar='\0';							//Guardar centinela en caso de que supere el numero mÃ¡ximo de parÃ¡metros y no se actualice el valor de currentChar
						if (currentParamNumber == 1)
						{
							currentChar=(char *)val1;					//la variable contiene el puntero a la string
							currentParamNumber++;
						}else if (currentParamNumber == 2)
						{
							currentChar=(char *)val2;
							currentParamNumber++;
						}
						
						while ((*currentChar!='\0') && (message_length<3*VCOLS))
						{
							resultado[message_length++]=*currentChar;
							currentChar++;
						}
						i +=2;											//Se suma 2 porque el formato ocupa 2 ASCII (%s)
						break;
					}
					
					case 'l':		// El cÃ³digo es practicamente el mismo para los dos, por eso se hace un fall-through
					case 'L':
					// Sumar nÃºm de carÃ¡cteres segÃºn tamaÃ±o del numero long
					// Si no cabe el nÃºmero long long, se llenarÃ© hasta que se llene el buffer
					{
						int pointed=1;
						if (token[i+1]=='l'){pointed=0;}
						char longlong[26];	//Como mÃ¡ximo un long ocupa 19-20 dÃ­gitos. Hay que tener en cuenta el signo y ls puntos '.' -> 26
						
						if (currentParamNumber == 1)
						{
							_gg_long2str(pointed, val1, longlong);
							currentParamNumber++;
						} else if (currentParamNumber == 2)
						{
							_gg_long2str(pointed, val2, longlong);
							currentParamNumber++;
						}
						
						currentChar=longlong;								//puntero para ir iterando sobre la array de chars resultante.
						while (*currentChar!='\0')
						{
							resultado[message_length++]=*currentChar;
							currentChar++;
						}
					}
					i +=2;												//Se suma 2 porque el formato ocupa 2 ASCII (%x)
					break;
					
					case 'q':
					case 'Q':
					{
						int pointed=1;
						char buffer[23];
						if (token[i+1]=='q'){pointed=0;}
						
						if (currentParamNumber == 1)
						{
							q12_to_str(pointed, val1, buffer);
							currentParamNumber++;
						} else if (currentParamNumber == 2)
						{
							q12_to_str(pointed, val2, buffer);
							currentParamNumber++;
						}
						
						currentChar=buffer;							//puntero para ir iterando sobre la array de chars resultante.
						while (*currentChar!='\0')
						{
							resultado[message_length++]=*currentChar;
							currentChar++;
						}
						
					}
					i+=2;
					break;
					
					
					default:											//error, no existe el formato, se ignorarÃ¡.
					{
						i +=2;											//Se ignorarÃ¡ tanto el '%' como la letra de formato inexistente.
						break;
					}
				}
			}
		}else														//Es un char ASCII normal o un carÃ¡cter de escape
		{
			resultado[message_length++]=formato[i];
			i++;													//Si es un ASCII normal solo se incrementa 1
		}
	}
	resultado[3*VCOLS]='\0';										//AÃ±adir centinela
}


/* _gg_escribir: escribe una cadena de caracteres en la ventana indicada;
	ParÃ¡metros:
		formato	->	cadena de formato, terminada con centinela '\0';
					admite '\n' (salto de lÃ­nea), '\t' (tabulador, 4 espacios)
					y cÃ³digos entre 32 y 159 (los 32 Ãºltimos son caracteres
					grÃ¡ficos), ademÃ¡s de marcas de format %c, %d, %h y %s (max.
					2 marcas por cadena) y de las marcas de cambio de color 
					actual %0 (blanco), %1 (amarillo), %2 (verde) y %3 (rojo)
		val1	->	valor a sustituir en la primera marca de formato, si existe
		val2	->	valor a sustituir en la segunda marca de formato, si existe
					- los valores pueden ser un cÃ³digo ASCII (%c), un valor
					  natural de 32 bits (%d, %x) o un puntero a string (%s)
		ventana	->	nÃºmero de ventana (de 0 a 3)
*/
void _gg_escribir(char *formato, unsigned int val1, unsigned int val2, int ventana)
{
	// Reservar 3 lÃ­neas para la variable que recibe el texto definitivo (+1 para el centinela, ya que no se imprime en pantalla).
	char textoDefinitivo[3*VCOLS+1]="", *token;						//Hay que guardar el centinela
	int filaActual, espaciosAInsertar, charInBuffer, message_length=0, color=(_gd_wbfs[ventana].pControl & 0xf0000000) >> 28;
	
	filaActual=((_gd_wbfs[ventana].pControl >> 16) & 0x0fff);		//12 bits medios: nÃºmero de lÃ­nea (0-23)
	charInBuffer=(_gd_wbfs[ventana].pControl & 0xFFFF);				//16 bits bajos: caracteres pendientes (0-32)

	_gg_procesarFormato(formato, val1, val2, textoDefinitivo);

	token=textoDefinitivo;
	//Nunca llegarÃ¡ un mensaje mÃ¡s largo que 32 carÃ¡cteres (sin incluir el centinela). 
	while ((*token!='\0') && (message_length<3*VCOLS))	//Se hace el control de carÃ¡cteres mÃ¡ximos en procesar formato. Pero \t o \n puede provocar overflow de lÃ­neas igualmente (que no se controla en procesarformato).
	{
		if ((charInBuffer==VCOLS) || (*token=='\n'))
		//Comprobar si se ha llenado el buffer o hay salto de linea.
		//Al hacer esta comprovaciÃ³n no hace falta comprovar que charInBuffer<VCOLS (excepto para introducir n espacios).
		{
			_gp_WaitForVBlank();
			if (filaActual==VFILS)
			{
				_gg_desplazar(ventana);
				filaActual--;										//Escribir en la ultima fila (23)
			}
			_gg_escribirLinea(ventana, filaActual, charInBuffer);
			charInBuffer=0;
			filaActual++;											//Volver a incrementar la fila para la siguiente linea.
			if (*token=='\n')
			{	//Sumar los carÃ¡cteres restantes de la lÃ­nea que se han saltado. Ya que aunque no se llenaron sÃ­ que ocuparon el resto de la lÃ­nea.
				message_length=message_length+VCOLS-message_length;
				token++;
			}
		}else if (*token=='\t')
		//Decison de diseÃ±o: Si el \t nos causa un salto de linea, se ignoran los espacios que falten
		//Ya que la primera posiciÃ³n de la siguiente fila ya es multiplo de 4
		{
			//Multiplo de 4, si ya lo es hay que insertar 4 espacios para llegar al siguiente multiplo.
			espaciosAInsertar=4-(charInBuffer%4);
			while ((charInBuffer<VCOLS) && (espaciosAInsertar>0) && (message_length<3*VCOLS))
			{
				_gd_wbfs[ventana].pChars[charInBuffer++]=' ';
				message_length++; espaciosAInsertar--;
			}
			token++;
		}else if ((*token=='%') && ( (*(token+1)>=48) && (*(token+1)<=57) ) ){
			color = *(token+1)-48;
			token+=2;
		}
		else
		//Si no es \t y tampoco es \n  y hay sitio suficiente para un char mas en la fila.
		{
			_gd_wbfs[ventana].pChars[charInBuffer++]=*token+color*128;	//Sumarle el color
			message_length++;token++;
		} //No hacer nada en caso de no entrar en ningun if, ya que no se imprimiÃ³ porque la linea esta llena
		
		//Actualizar variable de control
		_gd_wbfs[ventana].pControl=((filaActual & 0xffff) << 16) + (charInBuffer & 0xffff) + (color << 28);
	}
}