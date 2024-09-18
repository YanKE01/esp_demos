#include <stdio.h>

/**
 * @brief Used to self-increment the variable a, the number of which is determined by count.
 *
 * @param a
 * @param count
 */
void loopgtz_test(int *a, int count);

void app_main(void)
{
    int a = 0;
    loopgtz_test(&a, 10);
    printf("%d\n", a);
}
