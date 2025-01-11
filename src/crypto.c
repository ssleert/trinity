#include <stdio.h>
#include <string.h>
#include "sha256.h"  // Ensure you have the SHA-256 library available

/*
 * Hash a user password combined with proof of work (POW) using SHA-256.
 *
 * result: A char array to store the resulting hash in hexadecimal format (must be large enough to hold SHA256_HEX_SIZE).
 * user_password: The user's password.
 * pow: The proof of work string to combine with the password.
 *
 * Returns: 0 on success, -1 if an error occurs (e.g., NULL inputs).
 */
int hash_user_password_with_pow(char* result, const char* user_password, const char* pow) {
    // Validate inputs
    if (!result || !user_password || !pow) {
        return -1; // Invalid input
    }

    // Create a buffer to hold the concatenated input (password + POW)
    size_t password_len = strlen(user_password);
    size_t pow_len = strlen(pow);
    size_t combined_len = password_len + pow_len;

    // Allocate memory for combined string
    char combined[combined_len + 1]; // +1 for null-terminator

    // Combine password and POW
    strcpy(combined, user_password);
    strcat(combined, pow);

    // Compute SHA-256 hash of the combined string
    sha256_hex(combined, combined_len, result);

    return 0; // Success
}
