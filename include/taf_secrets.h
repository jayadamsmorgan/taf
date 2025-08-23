#ifndef TAF_SECRETS_H
#define TAF_SECRETS_H

#include "util/da.h"

int taf_secrets_parse_file(da_t *out);

void taf_register_secrets(da_t *new);

da_t *taf_get_secrets();

int taf_parse_secrets();

void taf_free_secrets();

#endif // TAF_SECRETS_H
