mihijijnput_pos < 30
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
