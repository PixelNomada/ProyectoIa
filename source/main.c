#include <nds.h>
#include <stdio.h>
#include <string.h>

#define MAX_TEXTO 80
#define MAX_VISIBLE 7
#define MAX_MENSAJES_RAM 100

typedef struct
{
    char autor[16];
    char texto[MAX_TEXTO];
} Mensaje;

Mensaje mensajes[MAX_MENSAJES_RAM];

int cantidadMensajes = 0;
int desplazamiento = 0;

static void agregarMensaje(const char *autor, const char *texto)
{
    int i;

    if (cantidadMensajes < MAX_MENSAJES_RAM)
    {
        strcpy(mensajes[cantidadMensajes].autor, autor);
        strcpy(mensajes[cantidadMensajes].texto, texto);

        cantidadMensajes++;
    }
    else
    {
        for (i = 0; i < MAX_MENSAJES_RAM - 1; i++)
        {
            mensajes[i] = mensajes[i + 1];
        }

        strcpy(mensajes[MAX_MENSAJES_RAM - 1].autor, autor);
        strcpy(mensajes[MAX_MENSAJES_RAM - 1].texto, texto);
    }

    if (cantidadMensajes > MAX_VISIBLE)
        desplazamiento = cantidadMensajes - MAX_VISIBLE;
    else
        desplazamiento = 0;
}
static void mostrarConversacion(PrintConsole *pantalla)
{
    int i;
    int final;

    consoleSelect(pantalla);
    consoleClear();

    printf("          DSi IA CHAT\n");
    printf("------------------------------\n");

    if (cantidadMensajes == 0)
    {
        printf("\n");
        printf("   No hay mensajes.\n");
        return;
    }

    final = desplazamiento + MAX_VISIBLE;

    if (final > cantidadMensajes)
        final = cantidadMensajes;

    for (i = desplazamiento; i < final; i++)
    {
        printf("%s:\n", mensajes[i].autor);
        printf("%s\n", mensajes[i].texto);
        printf("------------------------------\n");
    }
}

static void mostrarEntrada(
    PrintConsole *pantalla,
    const char *texto)
{
    consoleSelect(pantalla);
    consoleClear();

    printf("        ESCRIBIR MENSAJE\n");
    printf("------------------------------\n\n");
    printf("> %s\n", texto);
    printf("\n\n");
    printf("ENTER = ENVIAR\n");
    printf("B = SALIR\n");
}
int main(void)
{
    PrintConsole *pantallaSuperior;
    PrintConsole *pantallaInferior;

    char texto[MAX_TEXTO];
    int posicion = 0;

    pantallaSuperior = consoleDemoInit();

    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);

    pantallaInferior = consoleInit(
        NULL,
        0,
        BgType_Text4bpp,
        BgSize_T_256x256,
        31,
        0,
        false,
        true
    );

    consoleSelect(pantallaInferior);

    keyboardDemoInit();
    keyboardShow();

    agregarMensaje(
        "DSi IA",
        "Hola! Bienvenido a DSi IA Chat."
    );

    agregarMensaje(
        "DSi IA",
        "Escribe un mensaje abajo."
    );

    mostrarConversacion(pantallaSuperior);

    memset(texto, 0, sizeof(texto));

    mostrarEntrada(
        pantallaInferior,
        texto
    );
    while (1)
    {
        int tecla;

        swiWaitForVBlank();
        scanKeys();

        if (keysDown() & KEY_B)
            break;

        tecla = keyboardUpdate();

        if (tecla == -1)
            continue;

        if (tecla == DVK_ENTER)
        {
            if (posicion > 0)
            {
                texto[posicion] = '\0';

                agregarMensaje("TU", texto);

                agregarMensaje(
                    "CHAT",
                    "Mensaje recibido."
                );

                memset(texto, 0, sizeof(texto));
                posicion = 0;

                mostrarConversacion(
                    pantallaSuperior
                );

                mostrarEntrada(
                    pantallaInferior,
                    texto
                );
            }

            continue;
        }

        if (tecla == DVK_BACKSPACE)
        {
            if (posicion > 0)
            {
                posicion--;
                texto[posicion] = '\0';

                mostrarEntrada(
                    pantallaInferior,
                    texto
                );
            }

            continue;
        }

        if (tecla >= 32 && tecla <= 126)
        {
            if (posicion < MAX_TEXTO - 1)
            {
                texto[posicion] = (char)tecla;
                posicion++;
                texto[posicion] = '\0';

                mostrarEntrada(
                    pantallaInferior,
                    texto
                );
            }
        }
    }

    return 0;
}
