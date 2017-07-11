#ifdef __PIC32MZ__

#include <USB.h>

#define KVA_TO_PA(v)  ((v) & 0x1fffffff)
#define PA_TO_KVA0(pa)  ((pa) | 0x80000000)  // cachable
#define PA_TO_KVA1(pa)  ((pa) | 0xa000000

/*-------------- USB FS ---------------*/

USBHS *USBHS::_this;

#define WFB(X) (((X) + 3) / 4)

#ifdef PIN_LED_TX
static volatile uint32_t gTXLedTimeout = 0;
# define TXOn() digitalWrite(PIN_LED_TX, HIGH); gTXLedTimeout = millis();
static void TXLedSwitchOff(int id, void *tptr) {
    if (gTXLedTimeout > 0) {
        if (millis() - gTXLedTimeout >= 10) {
            digitalWrite(PIN_LED_TX, LOW);
            gTXLedTimeout = 0;
        }
    }
}
#else
# define TXOn()
#endif

#ifdef PIN_LED_RX
static volatile uint32_t gRXLedTimeout = 0;
# define RXOn() digitalWrite(PIN_LED_RX, HIGH); gRXLedTimeout = millis();
static void RXLedSwitchOff(int id, void *tptr) {
    if (gRXLedTimeout > 0) {
        if (millis() - gRXLedTimeout >= 10) {
            digitalWrite(PIN_LED_RX, LOW);
            gRXLedTimeout = 0;
        }
    }
}
#else
# define RXOn()
#endif


bool USBFS::enableUSB() {
#ifdef PIN_LED_TX
    pinMode(PIN_LED_TX, OUTPUT);
    digitalWrite(PIN_LED_TX, LOW);
    createTask(TXLedSwitchOff, 10, TASK_ENABLE, NULL);
#endif

#ifdef PIN_LED_RX
    pinMode(PIN_LED_RX, OUTPUT);
    digitalWrite(PIN_LED_RX, LOW);
    createTask(RXLedSwitchOff, 10, TASK_ENABLE, NULL);
#endif

    USBCSR0bits.SOFTCONN = 1;        // D+/D- active
    USBCSR0bits.HSEN = 0;           // Full speed negotiation
    USBCSR0bits.FUNC = 0;           // Address 0

    USBCSR2bits.RESETIE = 1;

    clearIntFlag(_USB_VECTOR);
    setIntVector(_USB_VECTOR, _usbInterrupt);
    setIntPriority(_USB_VECTOR, 6, 0);
    setIntEnable(_USB_VECTOR);

    addEndpoint(0, EP_IN, EP_CTL, 64, _ctlRxA, _ctlRxB);
    addEndpoint(0, EP_OUT, EP_CTL, 64, _ctlTxA, _ctlTxB);
	return true;
}

bool USBHS::enableUSB() {
#ifdef PIN_LED_TX
    pinMode(PIN_LED_TX, OUTPUT);
    digitalWrite(PIN_LED_TX, LOW);
    createTask(TXLedSwitchOff, 10, TASK_ENABLE, NULL);
#endif

#ifdef PIN_LED_RX
    pinMode(PIN_LED_RX, OUTPUT);
    digitalWrite(PIN_LED_RX, LOW);
    createTask(RXLedSwitchOff, 10, TASK_ENABLE, NULL);
#endif

    USBCSR0bits.SOFTCONN = 1;        // D+/D- active
    USBCSR0bits.HSEN = 1;           // High speed negotiation
    USBCSR0bits.FUNC = 0;           // Address 0

    USBCSR2bits.RESETIE = 1;

    clearIntFlag(_USB_VECTOR);
    setIntVector(_USB_VECTOR, _usbInterrupt);
    setIntPriority(_USB_VECTOR, 6, 0);
    setIntEnable(_USB_VECTOR);

    addEndpoint(0, EP_IN, EP_CTL, 64, _ctlRxA, _ctlRxB);
    addEndpoint(0, EP_OUT, EP_CTL, 64, _ctlTxA, _ctlTxB);
	return true;
}

bool USBHS::disableUSB() {
	return true;
}
	

bool USBHS::addEndpoint(uint8_t id, uint8_t direction, uint8_t type, uint32_t size, uint8_t *a, uint8_t *b) {
	if (id > 7) return false;

    if (id == 0) {
        USBCSR1bits.EP0IE = 1;
        USBE0CSR0bits.TXMAXP = size;
        if (direction == EP_IN) {
            _endpointBuffers[0].rx[0] = a;
            _endpointBuffers[0].rx[1] = b;
        } else {
            _endpointBuffers[0].tx[0] = a;
            _endpointBuffers[0].tx[1] = b;
        }
        _endpointBuffers[0].size = 64;

    } else {

        uint8_t ep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = id;
        USBIENCSR0bits.TXMAXP = size;
        if (type == EP_ISO) {
            USBIENCSR1bits.ISO = 1;
        } else {
            USBIENCSR1bits.ISO = 0;
        }

        _endpointBuffers[id].size = size;

        if (direction == EP_IN) {
            _endpointBuffers[id].rx[0] = a;
            _endpointBuffers[id].rx[1] = b;
            USBIENCSR3bits.RXFIFOSZ = size;
            USBFIFOAbits.RXFIFOAD = _fifoOffset;
            _fifoOffset += size / 8;
            switch (id) {
                case 1: USBCSR2bits.EP1RXIE = 1;
                case 2: USBCSR2bits.EP2RXIE = 1;
                case 3: USBCSR2bits.EP3RXIE = 1;
                case 4: USBCSR2bits.EP4RXIE = 1;
                case 5: USBCSR2bits.EP5RXIE = 1;
                case 6: USBCSR2bits.EP6RXIE = 1;
                case 7: USBCSR2bits.EP7RXIE = 1;
            }

        } else if (direction == EP_OUT) {
            _endpointBuffers[id].tx[0] = a;
            _endpointBuffers[id].tx[1] = b;
            USBIENCSR3bits.TXFIFOSZ = size;
            USBFIFOAbits.TXFIFOAD = _fifoOffset;
            _fifoOffset += size / 8;
            switch (id) {
                case 1: USBCSR1bits.EP1TXIE = 1;
                case 2: USBCSR1bits.EP2TXIE = 1;
                case 3: USBCSR1bits.EP3TXIE = 1;
                case 4: USBCSR1bits.EP4TXIE = 1;
                case 5: USBCSR1bits.EP5TXIE = 1;
                case 6: USBCSR1bits.EP6TXIE = 1;
                case 7: USBCSR1bits.EP7TXIE = 1;
            }
        }

        USBIENCSR3bits.SPEED = 0b01; // High speed

        switch (type) {
            case EP_CTL: USBIENCSR3bits.PROTOCOL = 0b00; break;
            case EP_ISO: USBIENCSR3bits.PROTOCOL = 0b01; break;
            case EP_BLK: USBIENCSR3bits.PROTOCOL = 0b10; break;
            case EP_INT: USBIENCSR3bits.PROTOCOL = 0b11; break;
        }

        USBCSR3bits.ENDPOINT = ep;
    }
	return true;
}

bool USBHS::canEnqueuePacket(uint8_t ep) {
    if (ep == 0) {
        return (USBE0CSR0bits.TXRDY == 0);
    }

    bool rdy = false;
    uint8_t oep = USBCSR3bits.ENDPOINT;
    USBCSR3bits.ENDPOINT = ep;
    rdy = (USBIENCSR0bits.TXPKTRDY == 0);
    USBCSR3bits.ENDPOINT = oep;
    
    return rdy;
}

bool USBHS::enqueuePacket(uint8_t ep, const uint8_t *data, uint32_t len) {
    if (!canEnqueuePacket(ep)) return false;

    volatile uint8_t *fifo = NULL;
    switch (ep) {
        case 0: fifo = (uint8_t *)&USBFIFO0; break;
        case 1: fifo = (uint8_t *)&USBFIFO1; break;
        case 2: fifo = (uint8_t *)&USBFIFO2; break;
        case 3: fifo = (uint8_t *)&USBFIFO3; break;
        case 4: fifo = (uint8_t *)&USBFIFO4; break;
        case 5: fifo = (uint8_t *)&USBFIFO5; break;
        case 6: fifo = (uint8_t *)&USBFIFO6; break;
        case 7: fifo = (uint8_t *)&USBFIFO7; break;
    }
    if (fifo == NULL) return false;

    for (int i = 0; i < len; i++) {
        *fifo = data[i];
    }

    if (ep == 0) {
        USBE0CSR0bits.TXRDY = 1; 
    } else {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = ep;
        USBIENCSR0bits.MODE = 1;
        USBIENCSR0bits.TXPKTRDY = 1; 
        USBCSR3bits.ENDPOINT = oep;
    }
	return true;
}

bool USBHS::sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len) {
    uint32_t remain = len;
    uint32_t pos = 0;

    uint32_t psize = _endpointBuffers[ep].size;

    if (len == 0) {
        while (1) {
            if (canEnqueuePacket(ep)) {
                enqueuePacket(ep, NULL, 0);
                return true;
            }
        }
    }

    while (remain > 0) {
        if (canEnqueuePacket(ep)) {
            uint32_t toSend = min(remain, psize);
            enqueuePacket(ep, &data[pos], toSend);
            pos += toSend;
            remain -= toSend;
        }
    }

    return true;
}

void USBHS::handleInterrupt() {
    uint32_t csr0 = USBCSR0;
    bool isEP0IF = (csr0 & (1<<16)) ? true : false;
    bool isEP1TXIF = (csr0 & (1<<17)) ? true : false;
    bool isEP2TXIF = (csr0 & (1<<18)) ? true : false;
    bool isEP3TXIF = (csr0 & (1<<19)) ? true : false;
    bool isEP4TXIF = (csr0 & (1<<20)) ? true : false;
    bool isEP5TXIF = (csr0 & (1<<21)) ? true : false;
    bool isEP6TXIF = (csr0 & (1<<22)) ? true : false;
    bool isEP7TXIF = (csr0 & (1<<23)) ? true : false;
    uint32_t csr1 = USBCSR1;
    bool isEP1RXIF = (csr1 & (1 << 1)) ? true : false;
    bool isEP2RXIF = (csr1 & (1 << 2)) ? true : false;
    bool isEP3RXIF = (csr1 & (1 << 3)) ? true : false;
    bool isEP4RXIF = (csr1 & (1 << 4)) ? true : false;
    bool isEP5RXIF = (csr1 & (1 << 5)) ? true : false;
    bool isEP6RXIF = (csr1 & (1 << 6)) ? true : false;
    bool isEP7RXIF = (csr1 & (1 << 7)) ? true : false;
    uint32_t csr2 = USBCSR2;
    bool isRESUMEIF = (csr2 & (1 << 17)) ? true : false;
    bool isRESETIF = (csr2 & (1 << 18)) ? true : false;
    bool isSOFIF = (csr2 & (1 << 19)) ? true : false;
    bool isCONNIF = (csr2 & (1 << 20)) ? true : false;
    bool isDISCONIF = (csr2 & (1 << 21)) ? true : false;
    bool isSESSRQIF = (csr2 & (1 << 22)) ? true : false;
    bool isVBUSERRIF = (csr2 & (1 << 23)) ? true : false;

    if (isRESETIF) {
        addEndpoint(0, EP_IN, EP_CTL, 64, _ctlRxA, _ctlRxB);
        addEndpoint(0, EP_OUT, EP_CTL, 64, _ctlTxA, _ctlTxB);
    }

    if (isEP0IF) {
        if (USBE0CSR0bits.RXRDY) {

            volatile uint8_t *fifo = (uint8_t *)&USBFIFO0;
            uint32_t pktlen = USBE0CSR2bits.RXCNT;

            for (int i = 0; i < pktlen; i++) {
                _endpointBuffers[0].rx[0][i] = *(fifo + (i & 3));
            }

            USBE0CSR0bits.RXRDYC = 1;

            if (_manager) _manager->onSetupPacket(0, _endpointBuffers[0].rx[0], pktlen);
            USBE0CSR0bits.SETENDC = 1;
        } else {
            if (_manager) _manager->onInPacket(0, _endpointBuffers[0].tx[0], _endpointBuffers[0].size);
        }
    }

    if (isEP1RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 1;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;
        uint32_t *buf = (uint32_t *)alloca(WFB(pktlen));
        for (int i = 0; i < WFB(pktlen); i++) {
            buf[i] = USBFIFO1;
        }
        USBIENCSR1bits.RXPKTRDY = 0;
        memcpy(_endpointBuffers[1].rx[0], buf, pktlen);
        if (_manager) _manager->onOutPacket(1, _endpointBuffers[1].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP2RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 2;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;
        uint32_t *buf = (uint32_t *)alloca(WFB(pktlen));
        for (int i = 0; i < WFB(pktlen); i++) {
            buf[i] = USBFIFO2;
        }
        USBIENCSR1bits.RXPKTRDY = 0;
        memcpy(_endpointBuffers[2].rx[0], buf, pktlen);
        if (_manager) _manager->onOutPacket(2, _endpointBuffers[2].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP3RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 3;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;
        uint32_t *buf = (uint32_t *)alloca(WFB(pktlen));
        for (int i = 0; i < WFB(pktlen); i++) {
            buf[i] = USBFIFO3;
        }
        USBIENCSR1bits.RXPKTRDY = 0;
        memcpy(_endpointBuffers[3].rx[0], buf, pktlen);
        if (_manager) _manager->onOutPacket(3, _endpointBuffers[3].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP4RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 4;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;
        uint32_t *buf = (uint32_t *)alloca(WFB(pktlen));
        for (int i = 0; i < WFB(pktlen); i++) {
            buf[i] = USBFIFO4;
        }
        USBIENCSR1bits.RXPKTRDY = 0;
        memcpy(_endpointBuffers[4].rx[0], buf, pktlen);
        if (_manager) _manager->onOutPacket(4, _endpointBuffers[4].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP5RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 5;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;
        uint32_t *buf = (uint32_t *)alloca(WFB(pktlen));
        for (int i = 0; i < WFB(pktlen); i++) {
            buf[i] = USBFIFO5;
        }
        USBIENCSR1bits.RXPKTRDY = 0;
        memcpy(_endpointBuffers[5].rx[0], buf, pktlen);
        if (_manager) _manager->onOutPacket(5, _endpointBuffers[5].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP6RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 6;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;
        uint32_t *buf = (uint32_t *)alloca(WFB(pktlen));
        for (int i = 0; i < WFB(pktlen); i++) {
            buf[i] = USBFIFO6;
        }
        USBIENCSR1bits.RXPKTRDY = 0;
        memcpy(_endpointBuffers[6].rx[0], buf, pktlen);
        if (_manager) _manager->onOutPacket(6, _endpointBuffers[6].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP7RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 7;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;
        uint32_t *buf = (uint32_t *)alloca(WFB(pktlen));
        for (int i = 0; i < WFB(pktlen); i++) {
            buf[i] = USBFIFO7;
        }
        USBIENCSR1bits.RXPKTRDY = 0;
        memcpy(_endpointBuffers[7].rx[0], buf, pktlen);
        if (_manager) _manager->onOutPacket(7, _endpointBuffers[7].tx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP1TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 1;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(1, _endpointBuffers[1].tx[0], _endpointBuffers[3].size);
    }
        
    if (isEP2TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 2;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(2, _endpointBuffers[2].tx[0], _endpointBuffers[3].size);
    }
        
    if (isEP3TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 3;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(3, _endpointBuffers[3].rx[0], _endpointBuffers[3].size);
    }
        
    if (isEP4TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 4;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(4, _endpointBuffers[4].rx[0], _endpointBuffers[3].size);
    }
        
    if (isEP5TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 5;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(5, _endpointBuffers[5].rx[0], _endpointBuffers[3].size);
    }
        
    if (isEP6TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 6;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(6, _endpointBuffers[6].rx[0], _endpointBuffers[3].size);
    }
        
    if (isEP7TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 7;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(7, _endpointBuffers[7].rx[0], _endpointBuffers[3].size);
    }
        


    clearIntFlag(_USB_VECTOR);
}

bool USBHS::setAddress(uint8_t address) {
    USBCSR0bits.FUNC = address & 0x7F;
	return true;
}



#endif
