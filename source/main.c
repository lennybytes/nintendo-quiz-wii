#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gccore.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

typedef struct {
    const char *question;
    const char *options[4];
    int answer;
    int difficulty; // 0 = easy, 1 = medium, 2 = hard
} Question;

static const Question quiz[] = {
    // Easy questions
    {
        "In which year was the Nintendo Wii released?",
        {"2005", "2006", "2007", "2008"},
        1, 0,
    },
    {
        "What is the motion controller of the Wii called?",
        {"DualShock", "GamePad", "Nunchuk", "Wiimote"},
        3, 0,
    },
    {
        "What is Mario's brother's name?",
        {"Luigi", "Wario", "Yoshi", "Toad"},
        0, 0,
    },
    {
        "What is the name of Mario's dinosaur friend?",
        {"Birdo", "Yoshi", "Koopa", "Diddy"},
        1, 0,
    },
    {
        "Which company created the Nintendo Entertainment System (NES)?",
        {"Sega", "Nintendo", "Sony", "Atari"},
        1, 0,
    },
    {
        "What was the first Nintendo handheld console with interchangeable cartridges?",
        {"Game Boy", "Nintendo DS", "Game Boy Advance", "Nintendo 3DS"},
        0, 0,
    },
    {
        "Which Nintendo console uses Wii Remotes?",
        {"Wii", "GameCube", "Wii U", "Nintendo 64"},
        0, 0,
    },
    {
        "What color is Mario's hat?",
        {"Blue", "Red", "Green", "Yellow"},
        1, 0,
    },
    {
        "What is the name of the princess Mario often rescues?",
        {"Zelda", "Peach", "Daisy", "Rosalina"},
        1, 0,
    },
    {
        "Which Nintendo character is a yellow electric mouse?",
        {"Pikachu", "Kirby", "Fox", "Link"},
        0, 0,
    },
    // Medium questions
    {
        "What does the Wii sensor bar actually emit?",
        {"Infrared light", "Sound waves", "Radio signals", "Ultraviolet light"},
        0, 1,
    },
    {
        "What was the first Zelda game for the Wii?",
        {"Ocarina of Time", "Twilight Princess", "Skyward Sword", "Breath of the Wild"},
        1, 1,
    },
    {
        "Which Nintendo console was released first?",
        {"Nintendo 64", "GameCube", "NES", "Wii"},
        2, 1,
    },
    {
        "What is the name of Link's main enemy in The Legend of Zelda?",
        {"Bowser", "Ganondorf", "Ridley", "King Boo"},
        1, 1,
    },
    {
        "Which Pokemon is number 001 in the National Pokedex?",
        {"Pikachu", "Charmander", "Bulbasaur", "Squirtle"},
        2, 1,
    },
    {
        "In which game series can you find the character Samus Aran?",
        {"Star Fox", "Metroid", "F-Zero", "Kirby"},
        1, 1,
    },
    {
        "Which Nintendo console was known for using small optical discs?",
        {"Game Boy", "Nintendo 64", "GameCube", "SNES"},
        2, 1,
    },
    {
        "What is the name of the main character in the Star Fox series?",
        {"Falco", "Fox", "Wolf", "Owl"},
        1, 1,
    },
    {
        "Which Nintendo console was the first to use optical discs?",
        {"Nintendo 64", "GameCube", "Wii", "Wii U"},
        1, 1,
    },
    {
        "What is the name of the main character in the F-Zero series?",
        {"Captain Falcon", "Fox McCloud", "Samus Aran", "Link"},
        0, 1,
    },
    // Hard questions
    {
        "What was the codename of the Nintendo Wii during development?",
        {"Revolution", "Nitro", "Fusion", "Atlas"},
        0, 2,
    },
    {
        "Which Nintendo console was the first to feature a built-in modem?",
        {"Nintendo 64", "GameCube", "Wii", "Satellaview"},
        3, 2,
    },
    {
        "What is the name of the main character in the EarthBound series?",
        {"Ness", "Lucas", "Claus", "Poo"},
        0, 2,
    },
    {
        "Which Nintendo console was the first to feature a touchscreen?",
        {"Nintendo DS", "Game Boy Advance", "Nintendo 3DS", "Wii U"},
        0, 2,
    },
    {
        "What is the name of the main character in the Kid Icarus series?",
        {"Pit", "Dark Pit", "Palutena", "Medusa"},
        0, 2,
    },
    {
        "Which Nintendo console was the first to feature motion controls?",
        {"Wii", "GameCube", "Nintendo 64", "Switch"},
        0, 2,
    },
    {
        "What is the name of the main character in the Fire Emblem series?",
        {"Marth", "Ike", "Robin", "Chrom"},
        0, 2,
    },
    {
        "Which Nintendo console was the first to feature a built-in camera?",
        {"Nintendo DSi", "Nintendo 3DS", "Wii U", "Switch"},
        0, 2,
    },
    {
        "What is the name of the main character in the Pikmin series?",
        {"Olimar", "Louie", "Alph", "Charlie"},
        0, 2,
    },
    {
        "Which Nintendo console was the first to feature a dual-screen design?",
        {"Nintendo DS", "Game Boy Advance", "Nintendo 3DS", "Wii U"},
        0, 2,
    },
};

#define NUM_QUESTIONS ((int)(sizeof(quiz) / sizeof(quiz[0])))
#define BUTTONS_PER_OPTION 4
#define QUESTION_TIME 15 // seconds per question

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

static void shuffle_questions(int *indices, int count)
{
    for (int i = count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}

static int ask_question(const Question *cur, int time_left)
{
    printf("Question: %s\n", cur->question);
    print_options(cur);
    printf("Time left: %d seconds\n", time_left);

    int answered = 0;
    int result = 0;

    while (!answered && time_left > 0) {
        WPAD_ScanPads();
        u32 buttons = WPAD_ButtonsDown(0);

        if (buttons & WPAD_BUTTON_HOME) {
            return -1;
        }

        int idx = option_from_buttons(buttons);
        if (idx >= 0) {
            if (idx == cur->answer) {
                printf("Correct!\n\n");
                result = 1;
            } else {
                printf("Wrong! The answer was (%c).\n\n",
                       OPTION_BUTTONS[cur->answer]);
            }
            answered = 1;
        }

        VIDEO_WaitVSync();
        time_left--;
    }

    if (!answered) {
        printf("Time's up! The answer was (%c).\n\n",
               OPTION_BUTTONS[cur->answer]);
    }

    return result;
}

static int select_difficulty(void)
{
    printf("Select difficulty:\n");
    printf("(A) Easy\n");
    printf("(B) Medium\n");
    printf("(1) Hard\n");
    printf("(2) Mixed\n\n");

    while (1) {
        WPAD_ScanPads();
        u32 buttons = WPAD_ButtonsDown(0);

        if (buttons & WPAD_BUTTON_HOME)
            return -1;
        if (buttons & WPAD_BUTTON_A)
            return 0;
        if (buttons & WPAD_BUTTON_B)
            return 1;
        if (buttons & WPAD_BUTTON_1)
            return 2;
        if (buttons & WPAD_BUTTON_2)
            return 3;

        VIDEO_WaitVSync();
    }
}

static int filter_by_difficulty(int *indices, int difficulty)
{
    int count = 0;
    for (int i = 0; i < NUM_QUESTIONS; i++) {
        if (difficulty == 3 || quiz[i].difficulty == difficulty) {
            indices[count++] = i;
        }
    }
    return count;
}

int main(void)
{
    // Initialize video
    VIDEO_Init();

    // Initialize Wiimote subsystem
    WPAD_Init();

    // Get the preferred video mode
    rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate the framebuffer
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    // Initialize the console
    console_init(
        xfb,
        20,
        20,
        rmode->fbWidth,
        rmode->xfbHeight,
        rmode->fbWidth * VI_DISPLAY_PIX_SZ
    );

    // Configure video
    VIDEO_Configure(rmode);

    // Set the framebuffer
    VIDEO_SetNextFramebuffer(xfb);

    // Turn the screen on
    VIDEO_SetBlack(FALSE);

    // Flush the video output
    VIDEO_Flush();

    // Wait for VSync
    VIDEO_WaitVSync();

    if (rmode->viTVMode & VI_NON_INTERLACE)
    {
        VIDEO_WaitVSync();
    }

    // Seed random number generator
    srand((unsigned int)time(NULL));

    int running = 1;

    while (running) {
        printf("\n=== Nintendo Quiz ===\n");
        printf("(A) Play Quiz\n");
        printf("(B) View Stats\n");
        printf("(1) Quit\n\n");

        int choice = -1;
        while (choice < 0) {
            WPAD_ScanPads();
            u32 buttons = WPAD_ButtonsDown(0);

            if (buttons & WPAD_BUTTON_HOME) {
                running = 0;
                break;
            }
            if (buttons & WPAD_BUTTON_A)
                choice = 0;
            if (buttons & WPAD_BUTTON_B)
                choice = 1;
            if (buttons & WPAD_BUTTON_1)
                choice = 2;

            VIDEO_WaitVSync();
        }

        if (!running || choice == 2) {
            break;
        }

        if (choice == 1) {
            printf("\n=== Stats ===\n");
            printf("Total questions: %d\n", NUM_QUESTIONS);
            printf("Easy: %d\n", 10);
            printf("Medium: %d\n", 10);
            printf("Hard: %d\n", 10);
            printf("\nPress (A) to return to menu.\n");
            int back = 0;
            while (!back) {
                WPAD_ScanPads();
                if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A)
                    back = 1;
                VIDEO_WaitVSync();
            }
            continue;
        }

        // Play quiz
        int difficulty = select_difficulty();
        if (difficulty < 0)
            break;

        int indices[NUM_QUESTIONS];
        int num_questions = filter_by_difficulty(indices, difficulty);
        shuffle_questions(indices, num_questions);

        int points = 0;
        int total_questions = num_questions < 10 ? num_questions : 10;

        printf("\nStarting quiz! %d questions, %d seconds each.\n\n",
               total_questions, QUESTION_TIME);

        for (int q = 0; q < total_questions && running; q++) {
            const Question *cur = &quiz[indices[q]];

            printf("Question %d of %d\n", q + 1, total_questions);
            int result = ask_question(cur, QUESTION_TIME);
            if (result < 0) {
                running = 0;
                break;
            }
            if (result > 0)
                points++;

            if (running && q + 1 < total_questions) {
                printf("Press (A) for the next question.\n");
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
            printf("Score: %d of %d\n", points, total_questions);

            if (points == total_questions) {
                printf("Perfect score! You're a Nintendo master!\n");
            } else if (points >= total_questions * 0.8) {
                printf("Great job! You know your Nintendo!\n");
            } else if (points >= total_questions * 0.5) {
                printf("Good effort! Keep learning!\n");
            } else {
                printf("Keep practicing! You'll get better!\n");
            }

            printf("\nPress (A) to return to menu.\n");
            int back = 0;
            while (!back) {
                WPAD_ScanPads();
                if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A)
                    back = 1;
                VIDEO_WaitVSync();
            }
        }
    }

    printf("\nThanks for playing!\n");
    printf("Press (HOME) to exit.\n");
    wait_for_home();

    return 0;
}
