#include "canfilter_internal.h"

#if defined(STM32G0xx) || defined(STM32H7xx)

/* Include the appropriate HAL header based on MCU */
#if defined(STM32G0xx)
#include "stm32g0xx_hal_fdcan.h"
#elif defined(STM32H7xx)
#include "stm32h7xx_hal_fdcan.h"
#endif

/* Unified bulk programming function - works for ALL FDCAN variants */
HAL_StatusTypeDef HAL_FDCAN_ConfigFilterBulk(FDCAN_HandleTypeDef *hfdcan, const uint32_t *std_filters,
                                             const uint64_t *ext_filters, uint16_t std_count, uint16_t ext_count) {

    /* Check FDCAN state */
    if ((hfdcan->State != HAL_FDCAN_STATE_READY) && (hfdcan->State != HAL_FDCAN_STATE_BUSY)) {
        hfdcan->ErrorCode |= HAL_FDCAN_ERROR_NOT_INITIALIZED;
        return HAL_ERROR;
    }

    /* 1. Program standard ID filters - DIRECT REGISTER WRITES */
    uint32_t *std_base = (uint32_t *)hfdcan->msgRam.StandardFilterSA;
    for (uint16_t i = 0; i < std_count; i++) {
        std_base[i] = std_filters[i];
    }

    /* 2. Program extended ID filters - DIRECT REGISTER WRITES */
    uint32_t *ext_base = (uint32_t *)hfdcan->msgRam.ExtendedFilterSA;
    for (uint16_t i = 0; i < ext_count; i++) {
        uint64_t ext_filter = ext_filters[i];
        ext_base[i * 2] = ext_filter & 0xFFFFFFFF;
        ext_base[i * 2 + 1] = (ext_filter >> 32) & 0xFFFFFFFF;
    }

    /* 3. Configure global filters */
    return HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);
}

/* Small FDCAN wrapper */
canfilter_error_t canfilter_stm32_fdcan_small_program(const canfilter_config_fdcan_small_t *fdcan_config) {
    FDCAN_HandleTypeDef *hfdcan = &hfdcan1; // Assume hfdcan1 is defined in user code

    if (!hfdcan)
        return CANFILTER_ERROR_PLATFORM;

    /* Validate controller type */
    if (fdcan_config->controller != CANFILTER_CTRL_FDCAN_SMALL) {
        return CANFILTER_ERROR_INVALID_PARAM;
    }

    HAL_StatusTypeDef hal_status =
        HAL_FDCAN_ConfigFilterBulk(hfdcan, fdcan_config->std_filters, fdcan_config->ext_filters,
                                   CANFILTER_FDCAN_SMALL_STD, // 28
                                   CANFILTER_FDCAN_SMALL_EXT  // 8
        );

    return (hal_status == HAL_OK) ? CANFILTER_SUCCESS : CANFILTER_ERROR_PLATFORM;
}

/* Large FDCAN wrapper */
canfilter_error_t canfilter_stm32_fdcan_large_program(const canfilter_config_fdcan_large_t *fdcan_config) {
    FDCAN_HandleTypeDef *hfdcan = &hfdcan1; // Assume hfdcan1 is defined in user code

    if (!hfdcan)
        return CANFILTER_ERROR_PLATFORM;

    /* Validate controller type */
    if (fdcan_config->controller != CANFILTER_CTRL_FDCAN_LARGE) {
        return CANFILTER_ERROR_INVALID_PARAM;
    }

    HAL_StatusTypeDef hal_status =
        HAL_FDCAN_ConfigFilterBulk(hfdcan, fdcan_config->std_filters, fdcan_config->ext_filters,
                                   CANFILTER_FDCAN_LARGE_STD, // 128
                                   CANFILTER_FDCAN_LARGE_EXT  // 64
        );

    return (hal_status == HAL_OK) ? CANFILTER_SUCCESS : CANFILTER_ERROR_PLATFORM;
}

#else
/* Stub implementations when not on STM32 */
HAL_StatusTypeDef HAL_FDCAN_ConfigFilterBulk(FDCAN_HandleTypeDef *hfdcan, const uint32_t *std_filters,
                                             const uint64_t *ext_filters, uint16_t std_count, uint16_t ext_count) {
    (void)hfdcan;
    (void)std_filters;
    (void)ext_filters;
    (void)std_count;
    (void)ext_count;
    return HAL_ERROR;
}

canfilter_error_t canfilter_stm32_fdcan_small_program(const canfilter_config_fdcan_small_t *fdcan_config) {
    (void)fdcan_config;
    return CANFILTER_ERROR_PLATFORM;
}

canfilter_error_t canfilter_stm32_fdcan_large_program(const canfilter_config_fdcan_large_t *fdcan_config) {
    (void)fdcan_config;
    return CANFILTER_ERROR_PLATFORM;
}
#endif
