#include <nds.h>
#include <dswifi9.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CONTACTS 10
#define MAX_MSG 10

// Estados de la app
enum {
    STATE_SETUP_NAME,
    STATE_CONTACTS,
    STATE_ADD_FRIEND,
    STATE_CHAT,
    STATE_SETTINGS
};

int state = STATE_SETUP_NAME;

char my_name[16] = "";
char my_code[8] = "";
char server_ip[32] = "192.168.1.99";
int server_port = 7777;
int sock = -1;
bool connected = false;

// Contactos
char contacts[MAX_CONTACTS][16];
int contact_count = 0;
int selected_contact = -1;

// Chat actual
char chatlog[MAX_MSG][50];
int chat_count = 0;

char input[40] = {0};
int input_pos = 0;

// ---------- Funciones de dibujo ----------

void clear_screens(void) {
    consoleClear();
}

void draw_header(const char* title) {
    printf("\x1b[0;0H========================\n");
    printf("  %s\n", title);
    printf("========================\n\n");
}

void draw_setup_name(void) {
    clear_screens();
    draw_header("DS CHAT - Configurar");
    printf("Escribe tu nombre:\n\n");
    printf("> %s\n\n", input);
    printf("ENTER = Continuar\n");
}

void draw_contacts(void) {
    clear_screens();
    draw_header("DS CHAT");
    printf("Tu codigo: %s\n", my_code);
    printf("Nombre: %s\n\n", my_name);
    printf("CONTACTOS:\n");

    if (contact_count == 0) {
        printf("  (ninguno)\n");
    } else {
        for (int i = 0; i < contact_count; i++) {
            printf("  %d. %s\n", i + 1, contacts[i]);
        }
    }

    printf("\n");
    printf("A = Agregar amigo\n");
    printf("1-9 = Chatear\n");
    printf("Y = Ajustes\n");
}

void draw_add_friend(void) {
    clear_screens();
    draw_header("Agregar amigo");
    printf("Escribe el codigo:\n\n");
    printf("> %s\n\n", input);
    printf("ENTER = Agregar\n");
    printf("B = Volver\n");
}

void draw_chat(void) {
    clear_screens();
    printf("\x1b[0;0HChat con: %s\n", contacts[selected_contact]);
    printf("------------------------\n");

    for (int i = 0; i < chat_count; i++) {
        printf("%s\n", chatlog[i]);
    }

    printf("\n\x1b[22;0HEscribir: %s", input);
}

// ---------- Red ----------

void add_chat_msg(const char* text) {
    if (chat_count >= MAX_MSG) {
        for (int i = 0; i < MAX_MSG - 1; i++)
            strcpy(chatlog[i], chatlog[i + 1]);
        chat_count = MAX_MSG - 1;
    }

    strncpy(chatlog[chat_count], text, 49);
    chatlog[chat_count][49] = 0;
    chat_count++;
}

bool connect_server(void) {
    if (!Wifi_InitDefault(WFC_CONNECT)) {
        return false;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_port = htons(server_port);
    sa.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        close(sock);
        sock = -1;
        return false;
    }

    // Login
    char buf[64];
    snprintf(buf, sizeof(buf), "LOGIN|%s\n", my_name);
    send(sock, buf, strlen(buf), 0);

    connected = true;
    return true;
}

void receive_messages(void) {
    if (sock < 0) return;

    char buf[128];

    int len = recv(
        sock,
        buf,
        sizeof(buf) - 1,
        MSG_DONTWAIT
    );

    if (len <= 0) return;

    buf[len] = 0;

    char* p = strchr(buf, '\n');
    if (p) *p = 0;

    if (strncmp(buf, "CODE|", 5) == 0) {

        strncpy(my_code, buf + 5, 7);
        my_code[7] = 0;

    }
    else if (strncmp(buf, "ADDED|", 6) == 0) {

        // ADDED|Nombre
        if (contact_count < MAX_CONTACTS) {

            strncpy(
                contacts[contact_count],
                buf + 6,
                15
            );

            contacts[contact_count][15] = 0;
            contact_count++;
        }

    }
    else if (strncmp(buf, "MSG|", 4) == 0) {

        // MSG|De|Texto
        char* from = buf + 4;

        char* text = strchr(from, '|');

        if (text) {

            *text = 0;
            text++;

            char line[50];

            snprintf(
                line,
                sizeof(line),
                "%s: %s",
                from,
                text
            );

            add_chat_msg(line);

            if (state == STATE_CHAT)
                draw_chat();
        }

    }
    else if (strncmp(buf, "ERR|", 4) == 0) {

        add_chat_msg(buf + 4);
    }
}

// ---------- Main ----------

int main(void) {

    videoSetMode(MODE_0_2D);

    vramSetBankA(VRAM_A_MAIN_BG);

    consoleInit(
        NULL,
        0,
        BgType_Text4bpp,
        BgSize_T_256x256,
        31,
        0,
        true,
        true
    );

    videoSetModeSub(MODE_0_2D);

    vramSetBankC(VRAM_C_SUB_BG);

    consoleInit(
        NULL,
        0,
        BgType_Text4bpp,
        BgSize_T_256x256,
        31,
        0,
        false,
        true
    );

    keyboardDemoInit();
    keyboardShow();

    draw_setup_name();

    while (1) {

        swiWaitForVBlank();

        scanKeys();

        u16 keys = keysDown();

        receive_messages();

        int key = keyboardUpdate();

        // ===== ESTADO: Poner nombre =====

        if (state == STATE_SETUP_NAME) {

            if (key == DVK_ENTER && input_pos > 0) {

                strncpy(my_name, input, 15);
                my_name[15] = 0;

                input_pos = 0;
                memset(input, 0, sizeof(input));

                if (connect_server()) {

                    state = STATE_CONTACTS;
                    draw_contacts();

                } else {

                    printf("\nError de conexion\n");
                }
            }

            else if (
                key == DVK_BACKSPACE &&
                input_pos > 0
            ) {

                input[--input_pos] = 0;

                draw_setup_name();
            }

            else if (
                key >= 32 &&
                key < 127 &&
                input_pos < 12
            ) {

                input[input_pos++] = key;
                input[input_pos] = 0;

                draw_setup_name();
            }
        }

        // ===== ESTADO: Lista de contactos =====

        else if (state == STATE_CONTACTS) {

            if (keys & KEY_A) {

                state = STATE_ADD_FRIEND;

                input_pos = 0;
                memset(input, 0, sizeof(input));

                draw_add_friend();

            }

            else if (keys & KEY_Y) {

                // Por ahora no hacemos nada extra

            }

            else if (keys >= KEY_1 && keys <= KEY_9) {

                int num = -1;

                if (keys & KEY_1) num = 0;
                if (keys & KEY_2) num = 1;
                if (keys & KEY_3) num = 2;
                if (keys & KEY_4) num = 3;
                if (keys & KEY_5) num = 4;
                if (keys & KEY_6) num = 5;
                if (keys & KEY_7) num = 6;
                if (keys & KEY_8) num = 7;
                if (keys & KEY_9) num = 8;

                if (
                    num >= 0 &&
                    num < contact_count
                ) {

                    selected_contact = num;

                    chat_count = 0;

                    state = STATE_CHAT;

                    input_pos = 0;
                    memset(input, 0, sizeof(input));

                    draw_chat();
                }
            }
        }

        // ===== ESTADO: Agregar amigo =====

        else if (state == STATE_ADD_FRIEND) {

            if (
                key == DVK_ENTER &&
                input_pos > 0
            ) {

                char buf[32];

                snprintf(
                    buf,
                    sizeof(buf),
                    "ADD|%s\n",
                    input
                );

                send(
                    sock,
                    buf,
                    strlen(buf),
                    0
                );

                input_pos = 0;
                memset(input, 0, sizeof(input));

                state = STATE_CONTACTS;

                draw_contacts();
            }

            else if (
                key == DVK_BACKSPACE &&
                input_pos > 0
            ) {

                input[--input_pos] = 0;

                draw_add_friend();
            }

            else if (
                key >= 32 &&
                key < 127 &&
                input_pos < 8
            ) {

                input[input_pos++] = key;
                input[input_pos] = 0;

                draw_add_friend();
            }

            if (keys & KEY_B) {

                state = STATE_CONTACTS;

                draw_contacts();
            }
        }

        // ===== ESTADO: Chat =====

        else if (state == STATE_CHAT) {

            if (
                key == DVK_ENTER &&
                input_pos > 0
            ) {

                char packet[80];

                snprintf(
                    packet,
                    sizeof(packet),
                    "PRIV|%s|%s\n",
                    contacts[selected_contact],
                    input
                );

                send(
                    sock,
                    packet,
                    strlen(packet),
                    0
                );

                char line[50];

                snprintf(
                    line,
                    sizeof(line),
                    "TU: %s",
                    input
                );

                add_chat_msg(line);

                input_pos = 0;
                memset(input, 0, sizeof(input));

                draw_chat();
            }

            else if (
                key == DVK_BACKSPACE &&
                input_pos > 0
            ) {

                input[--input_pos] = 0;

                draw_chat();
            }

            else if (
                key >= 32 &&
                key < 127 &&
                input_pos < 30
            ) {

                input[input_pos++] = key;
                input[input_pos] = 0;

                draw_chat();
            }

            if (keys & KEY_B) {

                state = STATE_CONTACTS;

                draw_contacts();
            }
        }
    }

    if (sock >= 0)
        close(sock);

    return 0;
}
