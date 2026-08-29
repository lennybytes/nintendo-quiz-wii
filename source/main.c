#include <stdio.h>
#include <stdlib.h>

#include <gccore.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

typedef struct {
    const char *question;
    const char *options[4];
    int answer;
} Question;

static const Question quiz[] = {
    {
        "In which year was the Nintendo Wii released?",
        {"2005", "2006", "2007", "2008"},
        2,
    },
    {
        "What is the motion controller of the Wii called?",
        {"DualShock", "GamePad", "Nunchuk", "Wiimote"},
        3,
    },
    {
        "What does the Wii sensor bar actually emit?",
        {"Infrared light", "Sound waves", "Radio signals", "Ultraviolet light"},
        0,
    },
    {
        "What was the first Zelda game for the Wii?",
        {"Ocarina of Time", "Twilight Princess", "Skyward Sword", "Breath of the Wild"},
        1,
    },
};

#define NUM_QUESTIONS ((int)(sizeof(quiz) / sizeof(quiz[0])))
#define BUTTONS_PER_OPTION 4

static const char *OPTION_BUTTONS = "AB12";

static int option_from_buttons(u32 buttons)
{
    if (buttons & WPAD_BUTTON_A)
        return 0;
    if (buttons & WPAD_BUTTON_B)
        return 1;
    if (buttons & WPAD_BUTTON_1)
        return 2;
    if (buttons & WPAD_BUTTON_2)
        return 3;
    return -1;
}

static void print_options(const Question *cur)
{
    for (int i = 0; i < BUTTONS_PER_OPTION; i++) {
        printf("(%c) %s\n", OPTION_BUTTONS[i], cur->options[i]);
    }
    printf("\n");
}

static void wait_for_home(void)
{
    while (1) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
            break;
        VIDEO_WaitVSync();
    }
}

int main(void)
{
    // Video initialisieren
    VIDEO_Init();

    // Wiimote-Subsystem initialisieren (sonst geht die Wiimote aus)
    WPAD_Init();

    // Bevorzugten Video-Modus holen
    rmode = VIDEO_GetPreferredMode(NULL);

    // Framebuffer reservieren
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    // Console initialisieren
    console_init(
        xfb,
        20,
        20,
        rmode->fbWidth,
        rmode->xfbHeight,
        rmode->fbWidth * VI_DISPLAY_PIX_SZ
    );

    // Video konfigurieren
    VIDEO_Configure(rmode);

    // Framebuffer setzen
    VIDEO_SetNextFramebuffer(xfb);

    // Bildschirm einschalten
    VIDEO_SetBlack(FALSE);

    // Video-Ausgabe aktualisieren
    VIDEO_Flush();

    // Auf VSync warten
    VIDEO_WaitVSync();

    if (rmode->viTVMode & VI_NON_INTERLACE)
    {
        VIDEO_WaitVSync();
    }

    int points = 0;
    int running = 1;

    printf("Welcome to the Nintendo quiz!\n");
    printf("Answer with (A), (B), (1) or (2).\n");
    printf("Press (HOME) to quit.\n\n");

    for (int q = 0; q < NUM_QUESTIONS && running; q++) {
        const Question *cur = &quiz[q];

        printf("Question %d of %d\n", q + 1, NUM_QUESTIONS);
        printf("%s\n", cur->question);
        print_options(cur);

        int answered = 0;
        while (!answered && running) {
            WPAD_ScanPads();
            u32 buttons = WPAD_ButtonsDown(0);

            if (buttons & WPAD_BUTTON_HOME) {
                running = 0;
                break;
            }

            int idx = option_from_buttons(buttons);
            if (idx >= 0) {
                if (idx == cur->answer) {
                    printf("Correct!\n\n");
                    points++;
                } else {
                    printf("Wrong! The answer was (%c).\n\n",
                           OPTION_BUTTONS[cur->answer]);
                }
                answered = 1;
            }

            VIDEO_WaitVSync();
        }

        if (running) {
            if (q + 1 < NUM_QUESTIONS)
                printf("Press (A) for the next question.\n");
            else
                printf("Press (A) to see your score.\n");

            int advance = 0;
            while (!advance && running) {
                WPAD_ScanPads();
                u32 buttons = WPAD_ButtonsDown(0);

                if (buttons & WPAD_BUTTON_HOME) {
                    running = 0;
                    break;
                }
                if (buttons & WPAD_BUTTON_A)
                    advance = 1;

                VIDEO_WaitVSync();
            }
        }
    }

    if (running) {
        printf("\n=== Results ===\n");
        printf("Score: %d of %d\n", points, NUM_QUESTIONS);
        printf("Press (HOME) to exit.\n");
        wait_for_home();
    }

    return 0;
}
