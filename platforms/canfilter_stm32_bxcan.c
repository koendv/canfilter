#include "canfilter_internal.h"

#if defined(STM32F4xx) || defined(STM32F1xx)
#include "stm32f4xx.h" // or stm32f1xx.h

static CAN_TypeDef *get_can_instance(int can_num) {
    /* Simple implementation - extend for multiple CAN instances */
    return CAN1;
}

static uint32_t disable_can_interrupts(CAN_TypeDef *can) {
    uint32_t active_interrupts = can->IER;
    can->IER = 0x00000000;
    __ISB();
    return active_interrupts;
}

static void restore_can_interrupts(CAN_TypeDef *can, uint32_t active_interrupts) {
    can->IER = active_interrupts;
    __ISB();
}

canfilter_error_t canfilter_stm32_bxcan_program(const void *config, uint32_t size) {
    if (size != sizeof(canfilter_config_bxcan_t)) {
        return CANFILTER_ERROR_INVALID_PARAM;
    }

    const canfilter_config_bxcan_t *bxcan_config = (const canfilter_config_bxcan_t *)config;
    CAN_TypeDef *can = get_can_instance(0); /* CAN1 */

    if (!can)
        return CANFILTER_ERROR_PLATFORM;

    /* Disable interrupts during filter programming */
    uint32_t was_irq_enabled = disable_can_interrupts(can);

    /* Enter filter initialization mode */
    can->FMR |= CAN_FMR_FINIT;

    /* Program filter registers */
    can->FS1R = bxcan_config->fs1r;
    can->FM1R = bxcan_config->fm1r;
    can->FFA1R = bxcan_config->ffa1r;

    /* Copy filter banks */
    for (uint32_t bank = 0; bank < CANFILTER_BXCAN_MAX_BANKS; bank++) {
        can->sFilterRegister[bank].FR1 = bxcan_config->fr1[bank];
        can->sFilterRegister[bank].FR2 = bxcan_config->fr2[bank];
    }

    /* Enable filter banks */
    can->FA1R = bxcan_config->fa1r;

    /* Ensure writes complete */
    __DSB();

    /* Exit filter initialization mode */
    can->FMR &= ~CAN_FMR_FINIT;

    /* Restore interrupts */
    restore_can_interrupts(can, was_irq_enabled);

    return CANFILTER_SUCCESS;
}

#else
/* Stub implementation when not on STM32 */
canfilter_error_t canfilter_stm32_bxcan_program(const void *config, uint32_t size) {
    (void)config;
    (void)size;
    return CANFILTER_ERROR_PLATFORM;
}
#endif
