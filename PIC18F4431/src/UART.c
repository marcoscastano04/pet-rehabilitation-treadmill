#include "../include/UART.h"
#include "../include/protocol.h"

void UART_TX_Init(void)
{
    TRISC6 = OUTPUT;    //Tx
    TRISC7 = INPUT;     //Rx
    
    TXSTA = 0;
    TXSTAbits.BRGH = 1;
    TXSTAbits.TXEN = 1;
    
    RCSTA = 0;
    RCSTAbits.SPEN = 1;
    RCSTAbits.CREN = 1;
    
    BRG16 = 1;
    SPBRGH = 0x00;
    SPBRG  = 0x56;  // 115200 @ 40 MHz
    
    PIR1bits.RCIF = 0;
    PIE1bits.RCIE = 1;
    RC1IP = 1;   //Interrupción de recepción como de alta prioridad.
}

//TRANSMISION---------------------------------------------------------------
//Estados (privados al módulo)
typedef enum {
    UART_TX_IDLE,
    UART_TX_START,
    UART_TX_DATA,
    UART_TX_ESCAPE,
    UART_TX_END
} uart_tx_state_t;

//Variables internas (no visibles desde main)
static volatile uart_tx_state_t uart_tx_state = UART_TX_IDLE;
#define UART_TX_PAYLOAD_LEN  (sizeof(tx_frame_t))
static uint8_t tx_payload[UART_TX_PAYLOAD_LEN];
static volatile uint8_t tx_len;
static volatile uint8_t tx_index;
static volatile uint8_t tx_escape_byte;


bool uart_start_frame(const uint8_t *data, uint8_t len)
{
    if (uart_tx_state != UART_TX_IDLE)
        return false;

    for (uint8_t i = 0; i < len; i++) {
        tx_payload[i] = data[i];
    }

    tx_len   = len;
    tx_index = 0;
    uart_tx_state = UART_TX_START;
    
    return true;
}

// Transmitir la trama, escapando los bytes reservados del protocolo.
void uart_tx_task(void)
{
    if (!TXIF) return;

    switch (uart_tx_state)
    {
        case UART_TX_IDLE:
            break;

        case UART_TX_START:
            TXREG = 0xAA;
            uart_tx_state = UART_TX_DATA;
            break;

        case UART_TX_DATA:
            if (tx_index >= tx_len) {
                uart_tx_state = UART_TX_END;
                break;
            }

            uint8_t b = tx_payload[tx_index++];

            if (b == 0xAA) {
                TXREG = 0x7E;
                tx_escape_byte = 0x01;
                uart_tx_state = UART_TX_ESCAPE;
            }
            else if (b == 0x55) {
                TXREG = 0x7E;
                tx_escape_byte = 0x02;
                uart_tx_state = UART_TX_ESCAPE;
            }
            else if (b == 0x7E) {
                TXREG = 0x7E;
                tx_escape_byte = 0x7E;
                uart_tx_state = UART_TX_ESCAPE;
            }
            else {
                TXREG = b;
            }
            break;

        case UART_TX_ESCAPE:
            TXREG = tx_escape_byte;
            uart_tx_state = UART_TX_DATA;
            break;

        case UART_TX_END:
            TXREG = 0x55;
            uart_tx_state = UART_TX_IDLE;
            break;
    }
}

//RECEPCION--------------------------------------------------------------------

#define UART_BUFFER_SIZE 32

static volatile uint8_t rx_buffer[UART_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

/* Frame parser internal */
static char rx_frame_buf[UART_BUFFER_SIZE];
static uint8_t rx_frame_idx = 0;

typedef enum {                
    UART_RX_WAIT_START,
    UART_RX_IN_FRAME
} uart_rx_state_t;

static uart_rx_state_t uart_state = UART_RX_WAIT_START;

// Puntero al callback de la aplicación, registrado desde main.
// Se invoca desde uart_rx_task() al recibir una trama completa.
// Recibe una cadena terminada en '\0', sin los delimitadores '<' y '>'.
static void (*command_callback)(char *cmd) = 0;

// Registrar el callback de la aplicación.
// Debe llamarse durante la inicialización. Pasar NULL lo desregistra.
void uart_register_command_callback(void (*cb)(char *cmd))
{
    command_callback = cb;
}

// Procesar el byte recibido desde la interrupción UART.
// Gestionar errores de recepción y almacenar el byte si hay espacio.
void uart_on_rx_interrupt(void)
{
    // Error de sobreescritura: reiniciar el receptor y descartar el byte.
    if (RCSTAbits.OERR)
    {
        CREN = 0;
        NOP();
        CREN = 1;

        // Lee RCREG para limpiarlo.
        volatile uint8_t dummy = RCREG;
        (void)dummy;
        return;  // discard
    }
    
    // Error de trama: descartar el byte recibido.
    if (RCSTAbits.FERR)
    {
        volatile uint8_t dummy = RCREG;
        (void)dummy;
        return;  
    }

    // Byte valido
    uint8_t c = RCREG;

    uint8_t next = (rx_head + 1) % UART_BUFFER_SIZE;
    if (next != rx_tail)
    {
        rx_buffer[rx_head] = c;
        rx_head = next;
    }
    else
    {
        // Si el búfer está lleno, descartar el byte recibido.
    }
}

// Procesar los bytes recibidos y reconstruir las tramas delimitadas por '<' y '>'.
// Ejecutar el callback al recibir una trama completa.
void uart_rx_task(void)
{
    while (rx_tail != rx_head)
    {
        uint8_t b = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % UART_BUFFER_SIZE;

        switch (uart_state)
        {
            case UART_RX_WAIT_START:
                if (b == '<')
                {
                    rx_frame_idx = 0;
                    uart_state = UART_RX_IN_FRAME;
                }
                break;

            case UART_RX_IN_FRAME:
                if (b == '>')
                {
                    rx_frame_buf[rx_frame_idx] = '\0';
                    if (command_callback) {
                        // Invocar el callback. Si necesita conservar el comando, debe copiar la cadena.
                        command_callback(rx_frame_buf);
                    }
                    uart_state = UART_RX_WAIT_START;
                }
                else
                {
                    if (rx_frame_idx < (sizeof(rx_frame_buf) - 1))
                    {
                        rx_frame_buf[rx_frame_idx++] = (char)b;
                    }
                    else
                    {
                        // Desbordamiento del búfer: descartar el comando incompleto.
                        rx_frame_idx = 0;
                        rx_frame_buf[0] = '\0';
                        uart_state = UART_RX_WAIT_START;
                    }
                }
                break;
        }
    }
}