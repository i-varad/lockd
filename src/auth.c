#define _POSIX_C_SOURCE 200809L

#include <security/pam_appl.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "auth.h"

#define LOCKD_PAM_SERVICE "lockd"

struct auth_context
{
    const char *username;
    const char *password;
};

static void clear_secret(char *secret)
{
    if (secret != NULL)
    {
        size_t length = strlen(secret);

        /*
            Use a volatile pointer so the compiler cannot
            optimize away these clearing writes.
        */
        volatile char *p = secret;

        while (length > 0)
        {
            *p++ = '\0';
            length--;
        }
    }
}

static int auth_conversation(
    int message_count,
    const struct pam_message **messages,
    struct pam_response **responses,
    void *user_data)
{
    if (message_count <= 0 || messages == NULL ||
        responses == NULL || user_data == NULL)
    {
        return PAM_CONV_ERR;
    }

    *responses = NULL;

    struct auth_context *context = user_data;

    struct pam_response *reply = calloc(
        (size_t)message_count, sizeof(*reply));

    if (reply == NULL)
    {
        return PAM_BUF_ERR;
    }

    for (int i = 0; i < message_count; i++)
    {
        if (messages[i] == NULL)
        {
            goto error;
        }

        const char *response = NULL;

        switch (messages[i]->msg_style)
        {
        case PAM_PROMPT_ECHO_OFF:
            response = context->password;
            break;

        case PAM_PROMPT_ECHO_ON:
            response = context->username;
            break;

        case PAM_ERROR_MSG:
        case PAM_TEXT_INFO:
            reply[i].resp = NULL;
            continue;

        default:
            goto error;
        }

        reply[i].resp = strdup(response);

        if (reply[i].resp == NULL)
        {
            goto error;
        }
    }

    *responses = reply;
    return PAM_SUCCESS;

error:
    for (int i = 0; i < message_count; i++)
    {
        if (reply[i].resp != NULL)
        {
            clear_secret(reply[i].resp);
            free(reply[i].resp);
        }
    }

    free(reply);
    return PAM_CONV_ERR;
}

int auth_user(const char *username, const char *password)
{
    if (username == NULL || password == NULL ||
        username[0] == '\0')
    {
        return 0;
    }

    struct auth_context context = {
        .username = username,
        .password = password
    };

    struct pam_conv conversation = {
        .conv = auth_conversation,
        .appdata_ptr = &context
    };

    pam_handle_t *pamh = NULL;

    int result = pam_start(
        LOCKD_PAM_SERVICE,
        username,
        &conversation,
        &pamh);

    if (result != PAM_SUCCESS)
    {
        return 0;
    }

    result = pam_authenticate(pamh, 0);

    if (result == PAM_SUCCESS)
    {
        result = pam_acct_mgmt(pamh, 0);
    }

    int end_result = pam_end(pamh, result);

    return result == PAM_SUCCESS && end_result == PAM_SUCCESS;
}