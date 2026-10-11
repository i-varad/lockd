#ifndef LOCKD_AUTH_H
#define LOCKD_AUTH_H

/*
    Authenticate a user through Linux-PAM.

    Returns 1 if authentication and account checks succeed.
    Returns 0 otherwise.
*/
int auth_user(const char *username, const char *password);

#endif