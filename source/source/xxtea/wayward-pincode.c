//gcc -Wno-deprecated-declarations  test.c xxtea/xxtea.c xxtea/base64.c  -o test

#include <stdio.h>
#include <string.h>
#include "xxtea/xxtea.h"
#include "xxtea/base64.h"

int main(int argc, char *argv[]) {
    const char *text = "Login password here super secret";

    //from settings
    const char *pin_keyb64 = "wyuQRH90gc4arvr9naCBMtuluE/eLxGKg/Bt9WNkRNfk+NS2";
    size_t len;


    if(argc == 2) {
      unsigned char *encrypt_data = xxtea_encrypt(text, strlen(text), argv[1], &len);
      char *base64_data = base64_encode(encrypt_data, len);
  //    printf("%s\n", encrypt_data);
      printf("%s\n", base64_data);

      return 0;
    } else if(argc == 3) {  //test

      unsigned char *encrypt_data = xxtea_encrypt(text, strlen(text), argv[1], &len);
      char *base64_data_encrypted = base64_encode(encrypt_data, len);
  //    printf("%s\n", encrypt_data);
      printf("Pin code B64 %s\n", base64_data_encrypted);

      if (strncmp(pin_keyb64, base64_data_encrypted, len) == 0) {
          printf("success!\n");
      } else {
          printf("wrong!\n");
      }

      free(encrypt_data);
      free(base64_data_encrypted);

      return 0;
    }

    return 0;
}
