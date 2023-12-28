//by Aashish Dugar
/**
 * See https://wiki.openssl.org/index.php/Elliptic_Curve_Diffie_Hellman
 * for source. The function below are based on the ecdh_low algorithm
 * described on that page and utilizes the OpenSSL low-level APIs for
 * Elliptic Curve Diffie Hellman key exchange algorithm.
 */

#include <assert.h>
#include <stdio.h>
#include <iostream>

#include <openssl/ec.h>
#include <openssl/ecdh.h>
#include <openssl/evp.h>

using namespace std;

EC_KEY* create_key(void) {
    EC_KEY* key;
    if (NULL == (key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1))) {
        printf("Failed to create key curve\n");
        return NULL;
    }

    if (1 != EC_KEY_generate_key(key)) {
        printf("Failed to generate key\n");
        return NULL;
    }
    return key;
}

unsigned char* get_secret(EC_KEY* key, const EC_POINT* peer_pub_key,
    size_t* secret_len) {
    int field_size;
    unsigned char* secret;

    field_size = EC_GROUP_get_degree(EC_KEY_get0_group(key));
    *secret_len = (field_size + 7) / 8;

    if (NULL == (secret = (unsigned char*)OPENSSL_malloc(*secret_len))) {
        printf("Failed to allocate memory for secret");
        return NULL;
    }

    *secret_len = ECDH_compute_key(secret, *secret_len,
        peer_pub_key, key, NULL);

    if (*secret_len <= 0) {
        OPENSSL_free(secret);
        return NULL;
    }
    return secret;
}

int main_(int argc, char* argv[]) {
    EC_KEY* alice = create_key();
    EC_KEY* bob = create_key();
    assert(alice != NULL && bob != NULL);

    const EC_POINT* alice_public = EC_KEY_get0_public_key(alice);
    const EC_POINT* bob_public = EC_KEY_get0_public_key(bob);

    size_t alice_secret_len;
    size_t bob_secret_len;

    const EC_GROUP* ec_group = EC_KEY_get0_group(bob);
    EC_POINT* point = EC_POINT_new(ec_group);

    // unsigned char buf[1024];
    // size_t size = EC_POINT_point2oct(ec_group, alice_public, POINT_CONVERSION_HYBRID, buf, 1024, NULL);
    // EC_POINT_oct2point(ec_group, point, buf, size, NULL);

    char *buf = EC_POINT_point2hex(ec_group, alice_public, POINT_CONVERSION_HYBRID, NULL);
    EC_POINT_hex2point(ec_group, buf, point, NULL);

    unsigned char* alice_secret = get_secret(alice, bob_public, &alice_secret_len);
    // unsigned char* bob_secret = get_secret(bob, alice_public, &bob_secret_len);
    unsigned char* bob_secret = get_secret(bob, point, &bob_secret_len);
    
    alice_secret[alice_secret_len] = 0;
    bob_secret[alice_secret_len] = 0;

    assert(alice_secret != NULL && bob_secret != NULL
        && alice_secret_len == bob_secret_len);

    for (int i = 0; i < alice_secret_len; i++)
        assert(alice_secret[i] == bob_secret[i]);

    EC_KEY_free(alice);
    EC_KEY_free(bob);
    OPENSSL_free(alice_secret);
    OPENSSL_free(bob_secret);

    return 0;
}