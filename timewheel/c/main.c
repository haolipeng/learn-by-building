#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "timer_wheel.h"

// Timer callback function
void timer_callback(timer_entry_t *entry)
{
    printf("Timer expired! Entry at %p\n", (void*)entry);
}

// Another timer callback
void timer_callback_2(timer_entry_t *entry)
{
    printf("Timer 2 expired! Entry at %p\n", (void*)entry);
}

int main(int argc, char *argv[])
{
    timer_wheel_t wheel;
    timer_entry_t entry1, entry2, entry3;
    uint32_t current_time = 0;

    printf("Initializing timer wheel...\n");

    // Initialize the timer wheel
    timer_wheel_init(&wheel);

    // Start the timer wheel at time 0
    timer_wheel_start(&wheel, current_time);
    printf("Timer wheel started at time %u\n", current_time);

    // Initialize timer entries
    timer_wheel_entry_init(&entry1);
    timer_wheel_entry_init(&entry2);
    timer_wheel_entry_init(&entry3);

    // Start timers with different timeouts
    printf("\nStarting timers:\n");
    printf("  Timer 1: timeout = 10 seconds\n");
    timer_wheel_entry_start(&wheel, &entry1, timer_callback, 10, current_time);

    printf("  Timer 2: timeout = 5 seconds\n");
    timer_wheel_entry_start(&wheel, &entry2, timer_callback_2, 5, current_time);

    printf("  Timer 3: timeout = 15 seconds\n");
    timer_wheel_entry_start(&wheel, &entry3, timer_callback, 15, current_time);

    printf("\nActive timers: %u\n", timer_wheel_count(&wheel));

    // Simulate time progression
    printf("\nSimulating time progression...\n\n");
    for (int i = 0; i < 20; i++) {
        current_time++;
        printf("Time: %u - ", current_time);

        uint32_t expired = timer_wheel_roll(&wheel, current_time);
        if (expired > 0) {
            printf("%u timer(s) expired\n", expired);
        } else {
            printf("No timers expired\n");
        }

        printf("  Active timers remaining: %u\n", timer_wheel_count(&wheel));

        // Sleep for demonstration purposes (optional)
        // usleep(100000); // 100ms
    }

    printf("\nTimer wheel demo completed.\n");
    printf("Final active timers: %u\n", timer_wheel_count(&wheel));

    return 0;
}
