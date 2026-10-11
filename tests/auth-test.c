#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <termios.h>

#include "auth.h"

#define PASSWORD_SIZE 256

static void clear_secret(char *password, size_t size)
{
    volatile char *p = password;

    while (size > 0)
    {
        *p++ = '\0';
        size--;
    }
}

int main(void)
{
    struct passwd *account = getpwuid(getuid());

    if (account == NULL)
    {
        fprintf(stderr, "auth-test: cannot determine current user\n");
        return 1;
    }

    char password[PASSWORD_SIZE] = {0};
    struct termios original;
    struct termios hidden;

    if (tcgetattr(STDIN_FILENO, &original) != 0)
    {
        perror("auth-test: tcgetattr");
        return 1;
    }

    hidden = original;
    hidden.c_lflag &= (tcflag_t)~ECHO;

    printf("Username: %s\n", account->pw_name);
    printf("Password: ");

    fflush(stdout);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &hidden) != 0)
    {
        perror("\nauth-test: tcsetattr");
        return 1;
    }

    char *input = fgets(password, sizeof(password), stdin);

    int restore_result = tcsetattr(
        STDIN_FILENO, TCSAFLUSH, &original);

    putchar('\n');

    if (restore_result != 0)
    {
        perror("auth-test: restoring terminal");
        clear_secret(password, sizeof(password));
        return 1;
    }

    if (input == NULL)
    {
        fprintf(stderr, "auth-test: failed to read password\n");
        clear_secret(password, sizeof(password));
        return 1;
    }

    password[strcspn(password, "\n")] = '\0';

    int authenticated = auth_user(account->pw_name, password);

    clear_secret(password, sizeof(password));

    if (authenticated)
    {
        puts("auth-test: authentication successful");
        return 0;
    }

    puts("auth-test: authentication failed");
    return 1;
}