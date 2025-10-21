#include <sys/types.h>
#include <string.h>
#include <stdint.h>

#include "timer_wheel.h"
#include "helper.h"

/**
 * timer_wheel_init - Initialize a timer wheel structure
 * @w: Pointer to the timer wheel to initialize
 *
 * Initializes all slots in the timer wheel by setting up empty circular
 * doubly-linked lists (using URCU list API). Resets the current time slot
 * and active timer count to zero.
 */
void timer_wheel_init(timer_wheel_t *w)
{
    int i;

    for (i = 0; i < MAX_TIMER_SLOTS; i ++) {
        CDS_INIT_LIST_HEAD(&w->slots[i]);
    }
    w->current = w->count = 0;
}

/**
 * timer_wheel_start - Start the timer wheel at a specific time
 * @w: Pointer to the timer wheel
 * @now: The current time value to set as the starting point
 *
 * Sets the timer wheel's current time position. This is typically called
 * once during initialization to establish the time baseline.
 */
void timer_wheel_start(timer_wheel_t *w, uint32_t now)
{
    w->current = now;
}

/**
 * timer_wheel_roll - Advance the timer wheel and process expired timers
 * @w: Pointer to the timer wheel
 * @now: The current time value
 *
 * Advances the timer wheel from its current position to 'now', processing
 * all timers that have expired in the time slots traversed. For each expired
 * timer, the callback function is invoked and the timer is removed from the
 * wheel.
 *
 * The function handles the case where callbacks may modify the timer list by
 * always removing from the head of each slot's list until it's empty, rather
 * than using a safe iterator.
 *
 * To prevent processing too many slots at once, it limits advancement to
 * MAX_TIMER_SLOTS even if 'now' is much larger than the current time.
 *
 * Return: The number of timers that expired and were processed
 */
uint32_t timer_wheel_roll(timer_wheel_t *w, uint32_t now)
{
    if (now < w->current) {
        return 0;
    }

    uint32_t cnt = 0;
    uint32_t s, m = min(now, w->current + MAX_TIMER_SLOTS);
    for (s = w->current; s < m; s ++) {
        struct cds_list_head *head = &w->slots[s % MAX_TIMER_SLOTS];

        // Because link entries can be modified in callback, so we cannot use
        // cds_list_for_each_entry_safe() to walk through the list; instead, we remove
        // the head every time and start over again until the link is empty.
        while (!cds_list_empty(head)) {
            timer_entry_t *itr = cds_list_first_entry(head, timer_entry_t, link);
            timer_wheel_expire_fct fn = itr->callback;
            timer_wheel_entry_remove(w, itr);
            fn(itr);
            cnt ++;
        }
    }

    w->current = now;

    return cnt;
}

/**
 * timer_wheel_entry_init - Initialize a timer entry
 * @n: Pointer to the timer entry to initialize
 *
 * Initializes a timer entry to a clean state with an empty list link,
 * invalid expire slot marker, and NULL callback. This must be called
 * before using a timer entry.
 */
void timer_wheel_entry_init(timer_entry_t *n)
{
    CDS_INIT_LIST_HEAD(&n->link);
    n->expire_slot = (uint16_t)(-1);
    n->callback = NULL;
}

#ifdef DEBUG_TIMER_WHEEL
#include <assert.h>
#define INSERT 0
#define REMOVE 1
#endif

/**
 * timer_wheel_entry_insert - Insert a timer entry into the timer wheel
 * @w: Pointer to the timer wheel
 * @n: Pointer to the timer entry to insert
 * @now: The current time value
 *
 * Inserts a timer entry into the appropriate slot of the timer wheel based
 * on its timeout value. The timer will expire at time (now + n->timeout).
 *
 * The expire slot is calculated using modulo arithmetic to wrap around the
 * circular timer wheel. The entry is added to the tail of the slot's list.
 *
 * When DEBUG_TIMER_WHEEL is defined, tracks the insertion in a debug history
 * buffer and validates that insertions and removals are properly paired.
 */
void timer_wheel_entry_insert(timer_wheel_t *w, timer_entry_t *n, uint32_t now)
{
#ifdef DEBUG_TIMER_WHEEL
    void *c1 = __builtin_return_address(0);
//    void *c2 = __builtin_return_address(1);
//    void *c3 = __builtin_return_address(2);
//    void *c4 = __builtin_return_address(3);
    if (n->debugs < 16) {
        n->history[n->debugs].caller[0] = c1;
//        n->history[n->debugs].caller2 = c2;
//        n->history[n->debugs].caller3 = c3;
//        n->history[n->debugs].caller4 = c4;
//        backtrace(n->history[n->debugs].caller, 4);
        n->history[n->debugs].callback = n->callback;
        n->history[n->debugs].act = INSERT;
        n->debugs ++;
        if (n->debugs >= 2 && n->history[n->debugs - 2].act == INSERT) {
            assert(0);
        }
    }
#endif

    uint32_t expire_at = now + n->timeout;
    n->expire_slot = expire_at % MAX_TIMER_SLOTS;
    cds_list_add_tail(&n->link, &w->slots[n->expire_slot]);
    w->count ++;
}

/**
 * timer_wheel_entry_refresh - Refresh a timer entry with a new timeout
 * @w: Pointer to the timer wheel
 * @n: Pointer to the timer entry to refresh
 * @now: The current time value
 *
 * Removes the timer entry from its current position in the wheel and
 * re-inserts it, effectively resetting its timeout to start from 'now'.
 * The callback function is preserved.
 *
 * This is useful for implementing keepalive or activity-based timers that
 * need to be reset when certain events occur.
 */
void timer_wheel_entry_refresh(timer_wheel_t *w, timer_entry_t *n, uint32_t now)
{
    timer_wheel_expire_fct fn = n->callback;
    timer_wheel_entry_remove(w, n);
    n->callback = fn;
    timer_wheel_entry_insert(w, n, now);
}

/**
 * timer_wheel_entry_remove - Remove a timer entry from the timer wheel
 * @w: Pointer to the timer wheel
 * @n: Pointer to the timer entry to remove
 *
 * Removes a timer entry from the timer wheel, canceling it before it expires.
 * The entry is unlinked from its slot's list, the active timer count is
 * decremented, and the callback is set to NULL to mark it as inactive.
 *
 * When DEBUG_TIMER_WHEEL is defined, tracks the removal in a debug history
 * buffer and validates proper pairing of insert/remove operations to catch
 * double-remove bugs.
 */
void timer_wheel_entry_remove(timer_wheel_t *w, timer_entry_t *n)
{
#ifdef DEBUG_TIMER_WHEEL
    void *c1 = __builtin_return_address(0);
//    void *c2 = __builtin_return_address(1);
//    void *c3 = __builtin_return_address(2);
//    void *c4 = __builtin_return_address(3);
    if (n->debugs < 16) {
        n->history[n->debugs].caller[0] = c1;
//        n->history[n->debugs].caller2 = c2;
//        n->history[n->debugs].caller3 = c3;
//        n->history[n->debugs].caller4 = c4;
//        backtrace(n->history[n->debugs].caller, 4);
        n->history[n->debugs].callback = n->callback;
        n->history[n->debugs].act = REMOVE;
        n->debugs ++;
        if (n->debugs < 2) {
            assert(0);
        }
        if (n->history[n->debugs - 2].act == REMOVE) {
            assert(0);
        }
        if (n->history[n->debugs - 2].act == INSERT) {
            n->debugs -= 2;
        }
    }
#endif

    cds_list_del(&n->link);
    w->count --;
    //n->expire_slot = (uint16_t)(-1);
    n->callback = NULL;
}

/**
 * timer_wheel_entry_start - Start a timer entry with a callback and timeout
 * @w: Pointer to the timer wheel
 * @n: Pointer to the timer entry to start
 * @cb: Callback function to invoke when the timer expires
 * @timeout: Timeout value in time units
 * @now: The current time value
 *
 * Initializes and starts a timer entry with the given callback and timeout.
 * If the timeout is greater than or equal to MAX_TIMER_SLOTS, it's clamped
 * to 0 to prevent out-of-bounds access.
 *
 * The timer will expire at time (now + timeout), at which point the callback
 * function will be invoked with the timer entry as its argument.
 */
void timer_wheel_entry_start(timer_wheel_t *w, timer_entry_t *n,
                             timer_wheel_expire_fct cb, uint16_t timeout, uint32_t now)
{
    if (unlikely(timeout >= MAX_TIMER_SLOTS)) {
        timeout = 0;
    }

    n->callback = cb;
    n->timeout = timeout;

    timer_wheel_entry_insert(w, n, now);
}

/**
 * get_expire_at - Calculate the absolute expiration time of a timer entry
 * @n: Pointer to the timer entry
 * @now: The current time value
 *
 * Calculates when the timer entry will expire in absolute time units.
 * This handles the circular nature of the timer wheel by determining
 * whether the expire slot is ahead of or behind the current slot position.
 *
 * If the timer is inactive (callback is NULL) or the expire slot is at or
 * after the current slot within the same wheel rotation, the expiration is
 * in the current rotation. Otherwise, it's in the next rotation.
 *
 * Return: The absolute time value when the timer will expire
 */
static uint32_t get_expire_at(const timer_entry_t *n, uint32_t now)
{
    uint16_t slot = now % MAX_TIMER_SLOTS;
    if (n->callback == NULL || n->expire_slot >= slot) {
        return now + n->expire_slot - slot;
    } else {
        return now + n->expire_slot + MAX_TIMER_SLOTS - slot;
    }
}

/**
 * timer_wheel_entry_get_idle - Get the idle time of a timer entry
 * @n: Pointer to the timer entry
 * @now: The current time value
 *
 * Calculates how much time has elapsed since the timer was started.
 * This is computed as the difference between the current time and when
 * the timer would have been inserted (expiration time minus timeout).
 *
 * Return: The number of time units the timer has been active
 */
uint16_t timer_wheel_entry_get_idle(const timer_entry_t *n, uint32_t now)
{
    return (uint16_t)(now - (get_expire_at(n, now) - n->timeout));
}

/**
 * timer_wheel_entry_get_life - Get the remaining lifetime of a timer entry
 * @n: Pointer to the timer entry
 * @now: The current time value
 *
 * Calculates how much time remains until the timer expires. This is the
 * difference between the timer's absolute expiration time and the current
 * time.
 *
 * Return: The number of time units remaining until expiration
 */
uint16_t timer_wheel_entry_get_life(const timer_entry_t *n, uint32_t now)
{
    return (uint16_t)(get_expire_at(n, now) - now);
}

