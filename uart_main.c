#include <gpio.h>
#include <stm32.h>
#include <string.h>
#include "buffer.h"

// **************** USART ******************************

#define USART_Mode_Rx_Tx (USART_CR1_RE | USART_CR1_TE)
#define USART_Enable USART_CR1_UE

#define USART_WordLength_8b 0x0000
#define USART_WordLength_9b USART_CR1_M

#define USART_Parity_No 0x0000
#define USART_Parity_Even USART_CR1_PCE
#define USART_Parity_Odd (USART_CR1_PCE | USART_CR1_PS)

#define USART_StopBits_1 0x0000

#define USART_FlowControl_None 0x0000
#define USART_FlowControl_RTS USART_CR3_RTSE
#define USART_FlowControl_CTS USART_CR3_CTSE

#define USART_New_Input (USART2->SR & USART_SR_RXNE)

#define USART_Free_Output (USART2->SR & USART_SR_TXE)

// **************** Buttons ***************************

// active on low
#define JOY_GPIO GPIOB
#define JOY_LEFT_PIN 3
#define JOY_RIGHT_PIN 4
#define JOY_UP_PIN 5
#define JOY_DOWN_PIN 6
#define JOY_PUSH_PIN 10

#define JOY_BITMASK 0x478

// active on low
#define USR_GPIO GPIOC
#define USR_PIN 13

#define USR_BITMASK 0x2000

// active on high
#define AT_MODE_GPIO GPIOA
#define AT_MODE_PIN 0

#define AT_MODE_BITMASK 0x1

// ***************** Misc ******************************

#define HSI_HZ 16000000U

#define PCLK1_HZ HSI_HZ

#define BAUD_RATE 9600U

#define IN_MSG_LEN 3

#define OUT_MSG_MAX_LEN 17

// ****************** LEDs *******************************

#define RED_LED_GPIO GPIOA
#define GREEN_LED_GPIO GPIOA
#define BLUE_LED_GPIO GPIOB
#define GREEN2_LED_GPIO GPIOA
#define RED_LED_PIN 6
#define GREEN_LED_PIN 7
#define BLUE_LED_PIN 0
#define GREEN2_LED_PIN 5

#define RedLEDon() \
    RED_LED_GPIO->BSRR = 1 << (RED_LED_PIN + 16)
#define RedLEDoff() \
    RED_LED_GPIO->BSRR = 1 << RED_LED_PIN
#define RedLEDflip() \
    RED_LED_GPIO->BSRR = 1 << (RED_LED_PIN + 16 * (1 & (RED_LED_GPIO->ODR >> RED_LED_PIN)))

#define GreenLEDon() \
    GREEN_LED_GPIO->BSRR = 1 << (GREEN_LED_PIN + 16)
#define GreenLEDoff() \
    GREEN_LED_GPIO->BSRR = 1 << GREEN_LED_PIN
#define GreenLEDflip() \
    GREEN_LED_GPIO->BSRR = 1 << (GREEN_LED_PIN + 16 * (1 & (GREEN_LED_GPIO->ODR >> GREEN_LED_PIN)))

#define BlueLEDon() \
    BLUE_LED_GPIO->BSRR = 1 << (BLUE_LED_PIN + 16)
#define BlueLEDoff() \
    BLUE_LED_GPIO->BSRR = 1 << BLUE_LED_PIN
#define BlueLEDflip() \
    BLUE_LED_GPIO->BSRR = 1 << (BLUE_LED_PIN + 16 * (1 & (BLUE_LED_GPIO->ODR >> BLUE_LED_PIN)))

#define Green2LEDon() \
    GREEN2_LED_GPIO->BSRR = 1 << GREEN2_LED_PIN
#define Green2LEDoff() \
    GREEN2_LED_GPIO->BSRR = 1 << (GREEN2_LED_PIN + 16)
#define Green2LEDflip() \
    GREEN2_LED_GPIO->BSRR = 1 << (GREEN2_LED_PIN + 16 * (1 & (GREEN2_LED_GPIO->ODR >> GREEN2_LED_PIN)))

// ***************** Code *********************************

void setNewMessage(int button_code, int new_state_code, char *msg)
{
    memset(msg, '\0', OUT_MSG_MAX_LEN);
    switch (button_code)
    {
    case AT_MODE_PIN:
        strncpy(msg, "MODE ", 15);
        break;
    case JOY_LEFT_PIN:
        strncpy(msg, "LEFT ", 15);
        break;
    case JOY_RIGHT_PIN:
        strncpy(msg, "RIGHT ", 15);
        break;
    case JOY_DOWN_PIN:
        strncpy(msg, "DOWN ", 15);
        break;
    case JOY_UP_PIN:
        strncpy(msg, "UP ", 15);
        break;
    case JOY_PUSH_PIN:
        strncpy(msg, "FIRE ", 15);
        break;
    case USR_PIN:
        strncpy(msg, "USER ", 15);
        break;
    default:
        strncpy(msg, "ERR ", 15);
    }
    while (*msg != '\0')
    {
        msg++;
    }
    switch (new_state_code)
    {
    case 0:
        strcpy(msg, "PRESSED\r\n");
        break;
    case 1:
        strcpy(msg, "RELEASED\r\n");
        break;
    default:
        strcpy(msg, "ERR\r\n");
    }
}

void handleOutput(uint32_t but_change, uint32_t prev_but_state, buffer *buff)
{
    int shift = 0;
    while (but_change)
    {
        if (but_change & 1)
        {
            // we just drop old messages from the buffer instead of checking if it's full
            // todo is this ok?
            buff_push(buff, shift);
            // push 0 if pressed, 1 if released
            buff_push(buff, prev_but_state & 1);
        }
        shift++;
        but_change >>= 1;
        prev_but_state >>= 1;
    }
}

void ledOn(int *buff)
{
    switch (buff[1])
    {
    case 'R':
        RedLEDon();
        break;
    case 'G':
        GreenLEDon();
        break;
    case 'B':
        BlueLEDon();
        break;
    case 'g':
        Green2LEDon();
        break;
    default:
        return;
    }
}

void ledOff(int *buff)
{
    switch (buff[1])
    {
    case 'R':
        RedLEDoff();
        break;
    case 'G':
        GreenLEDoff();
        break;
    case 'B':
        BlueLEDoff();
        break;
    case 'g':
        Green2LEDoff();
        break;
    default:
        return;
    }
}

void ledFlip(int *buff)
{
    switch (buff[1])
    {
    case 'R':
        RedLEDflip();
        break;
    case 'G':
        GreenLEDflip();
        break;
    case 'B':
        BlueLEDflip();
        break;
    case 'g':
        Green2LEDflip();
        break;
    default:
        return;
    }
}

void handleInput(int *buff)
{
    if (buff[0] != 'L')
        return;
    switch (buff[2])
    {
    case '1':
        ledOn(buff);
        break;
    case '0':
        ledOff(buff);
        break;
    case 'T':
        ledFlip(buff);
        break;
    default:
        return;
    }
    // todo set given led, change above defines to dinamically choose led maybe, wathc out for green2 being on a different part of board
}

// bitmap of pins where a 1 means the button is engaged regardless of wether
// the buttons is active on high or low
uint32_t buttonState()
{
    uint32_t ans = 0;
    ans |= (~JOY_GPIO->IDR & JOY_BITMASK);
    ans |= (~USR_GPIO->IDR & USR_BITMASK);
    ans |= (AT_MODE_GPIO->IDR & AT_MODE_BITMASK);
    return ans;
}

int main()
{

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    __NOP();
    RedLEDoff();
    GreenLEDoff();
    BlueLEDoff();
    Green2LEDoff();
    GPIOoutConfigure(RED_LED_GPIO,
                     RED_LED_PIN,
                     GPIO_OType_PP,
                     GPIO_Low_Speed,
                     GPIO_PuPd_NOPULL);

    GPIOoutConfigure(GREEN_LED_GPIO,
                     GREEN_LED_PIN,
                     GPIO_OType_PP,
                     GPIO_Low_Speed,
                     GPIO_PuPd_NOPULL);

    GPIOoutConfigure(BLUE_LED_GPIO,
                     BLUE_LED_PIN,
                     GPIO_OType_PP,
                     GPIO_Low_Speed,
                     GPIO_PuPd_NOPULL);

    GPIOoutConfigure(GREEN2_LED_GPIO,
                     GREEN2_LED_PIN,
                     GPIO_OType_PP,
                     GPIO_Low_Speed,
                     GPIO_PuPd_NOPULL);

    // GPIOinConfigure()

    GPIOafConfigure(GPIOA,
                    2,
                    GPIO_OType_PP,
                    GPIO_Fast_Speed,
                    GPIO_PuPd_NOPULL,
                    GPIO_AF_USART2);

    GPIOafConfigure(GPIOA,
                    3,
                    GPIO_OType_PP,
                    GPIO_Fast_Speed,
                    GPIO_PuPd_UP,
                    GPIO_AF_USART2);

    USART2->CR1 = USART_Mode_Rx_Tx | USART_WordLength_8b | USART_Parity_No;

    USART2->CR2 = USART_StopBits_1;

    USART2->CR3 = USART_FlowControl_None;

    USART2->BRR = (PCLK1_HZ + (BAUD_RATE / 2U)) / BAUD_RATE;

    // starts listening here
    USART2->CR1 |= USART_Enable;

    uint32_t prev_but_state = buttonState();
    int in_buff[3];
    int in_size = 0;
    buffer out_buff;
    buff_init(&out_buff);
    char out_msg[OUT_MSG_MAX_LEN] = {};
    char *out_char_ptr = out_msg;
    while (1)
    {
        if (USART_New_Input)
        {
            char c;
            c = USART2->DR;
            in_size = c == 'L' ? 1 : in_size + 1;
            in_buff[in_size - 1] = c;
            if (in_size == IN_MSG_LEN)
            {
                handleInput(in_buff);
                in_size = 0;
            }
        }
        uint32_t but_change = buttonState() ^ prev_but_state;
        if (but_change)
        {
            handleOutput(but_change, prev_but_state, &out_buff);
            prev_but_state = but_change ^ prev_but_state;
        }
        if (USART_Free_Output)
        {
            if (*out_char_ptr != '\0')
            {
                USART2->DR = *out_char_ptr;
                out_char_ptr++;
            }
            else if (!buff_empty(&out_buff))
            {
                int button_code = buff_pop(&out_buff);
                int state_code = buff_pop(&out_buff);
                setNewMessage(button_code, state_code, out_msg);
                out_char_ptr = out_msg;
                USART2->DR = *out_char_ptr;
                out_char_ptr++;
            }
        }
    }
    return 1;
}