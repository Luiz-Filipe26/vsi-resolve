#ifndef VSI_TYPES_H
#define VSI_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VSI_OK = 0,
    VSI_ERROR_GENERIC = 1,
    VSI_ERROR_INVALID_ARG = 2,
    VSI_ERROR_INVALID_HANDLE = 3,
    VSI_ERROR_UNSUPPORTED = 4,
    VSI_MODULE_ERROR = 5,
    VSI_MODULE_STATUS = 6,
} vsi_base_status_t;

#ifdef __cplusplus
}
#endif

#endif /* VSI_TYPES_H */
