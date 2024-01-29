#include <mik32_hal_spifi.h>

__attribute__((weak)) void HAL_SPIFI_MspInit()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_PCC_SPIFI_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_SERIAL;
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_NONE;
    HAL_GPIO_Init(GPIO_2, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_SERIAL;
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_UP;
    HAL_GPIO_Init(GPIO_2, &GPIO_InitStruct);
}

void HAL_SPIFI_MemoryMode_Init(SPIFI_MemoryModeConfig_HandleTypeDef *spifi)
{
    HAL_SPIFI_MspInit();

    spifi->Instance->STAT |= SPIFI_CONFIG_STAT_RESET_M;
    spifi->Instance->CLIMIT = spifi->CacheLimit; // Граница кеширования
    if (spifi->CacheEnable)
    {
        spifi->Instance->CTRL |= SPIFI_CONFIG_CTRL_CACHE_EN_M;
    }
    else
    {
        spifi->Instance->CTRL &= ~SPIFI_CONFIG_CTRL_CACHE_EN_M;
    }

    // Настройка команды чтения
    spifi->Instance->MCMD = ((spifi->Command.InterimLength << SPIFI_CONFIG_MCMD_INTLEN_S) |
                             (spifi->Command.FieldForm << SPIFI_CONFIG_MCMD_FIELDFORM_S) |
                             (spifi->Command.FrameForm << SPIFI_CONFIG_MCMD_FRAMEFORM_S) |
                             (spifi->Command.OpCode << SPIFI_CONFIG_MCMD_OPCODE_S));
}

void SPIFI_WaitIntrqTimeout()
{
    while (~(SPIFI_CONFIG->STAT & SPIFI_CONFIG_STAT_INTRQ_M))
        ;
}

void HAL_SPIFI_SendCommand(
    SPIFI_HandleTypeDef *spifi,
    SPIFI_CommandTypeDef *cmd,
    uint32_t address,
    uint16_t bufferSize,
    uint8_t *readBuffer,
    uint8_t *writeBuffer)
{
    HAL_SPIFI_SendCommand_LL(
        spifi,
        cmd->Direction |
            SPIFI_CONFIG_CMD_INTLEN(cmd->InterimLength) |
            SPIFI_CONFIG_CMD_FIELDFORM(cmd->FieldForm) |
            SPIFI_CONFIG_CMD_FRAMEFORM(cmd->FrameForm) |
            SPIFI_CONFIG_CMD_OPCODE(cmd->OpCode),
        address,
        bufferSize,
        readBuffer,
        writeBuffer,
        cmd->InterimData);
}

void HAL_SPIFI_SendCommand_LL(
    SPIFI_HandleTypeDef *spifi,
    uint32_t cmd,
    uint32_t address,
    uint16_t bufferSize,
    uint8_t *readBuffer,
    uint8_t *writeBuffer,
    uint32_t interimData)
{
    // spifi->Instance->STAT |= SPIFI_CONFIG_STAT_INTRQ_M;

    spifi->Instance->ADDR = address;
    spifi->Instance->IDATA = interimData;
    spifi->Instance->CMD = cmd | SPIFI_CONFIG_CMD_DATALEN(bufferSize);

    // SPIFI_WaitIntrqTimeout();

    if (cmd & SPIFI_CONFIG_CMD_DOUT_M)
    {
        if ((bufferSize > 0) && (writeBuffer == 0))
        {
            // xprintf("dataByte zero pointer\n");
            // Сделать возврат ошибки
            return;
        }
        for (int i = 0; i < bufferSize; i++)
        {
            spifi->Instance->DATA8 = writeBuffer[i];
        }
    }
    else
    {
        if ((bufferSize > 0) && (readBuffer == 0))
        {
            // xprintf("dataByte zero pointer\n");
            // Сделать возврат ошибки
            return;
        }
        for (int i = 0; i < bufferSize; i++)
        {
            readBuffer[i] = (uint8_t)spifi->Instance->DATA8;
        }
    }
}

bool HAL_SPIFI_IsMemoryModeEnabled(SPIFI_HandleTypeDef *spifi)
{
    return (spifi->Instance->STAT & SPIFI_CONFIG_STAT_MCINIT_M) != 0;
}

void HAL_SPIFI_Reset(SPIFI_HandleTypeDef *spifi)
{
    spifi->Instance->STAT = SPIFI_CONFIG_STAT_RESET_M;
}

bool HAL_SPIFI_IsReady(SPIFI_HandleTypeDef *spifi)
{
    return (spifi->Instance->STAT & SPIFI_CONFIG_STAT_RESET_M) == 0;
}
