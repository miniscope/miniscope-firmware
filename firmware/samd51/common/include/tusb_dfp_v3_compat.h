/**
 * DFP v3.x compatibility shim for TinyUSB.
 *
 * TinyUSB's dcd_samd.c expects Microchip DFP v2.x register naming
 * conventions (e.g., USB->DEVICE.CTRLA.bit.SWRST), while our DFP v3.8.x
 * uses flat integer register fields (USB_REGS->DEVICE.USB_CTRLA).
 *
 * This header reconstructs the v2-style union/bitfield types, struct
 * layout, constant macros, and IRQ names. It is included AFTER the real
 * DFP sam.h via the wrapper at cmake/tinyusb_compat/sam.h.
 *
 * Only the subset used by TinyUSB 0.17.0 dcd_samd.c is provided.
 */
#ifndef TUSB_DFP_V3_COMPAT_H
#define TUSB_DFP_V3_COMPAT_H

#ifdef FW_ENABLE_USB

#include <stdint.h>

/* ======================================================================
 * v2-style register union types  (each has .bit and .reg members)
 * ====================================================================== */

typedef union {
    struct {
        uint8_t SWRST:1;
        uint8_t ENABLE:1;
        uint8_t RUNSTDBY:1;
        uint8_t :4;
        uint8_t MODE:1;
    } bit;
    uint8_t reg;
} USB_CTRLA_Type;

typedef union {
    struct {
        uint8_t SWRST:1;
        uint8_t ENABLE:1;
        uint8_t :6;
    } bit;
    uint8_t reg;
} USB_SYNCBUSY_Type;

typedef union {
    struct {
        uint8_t CQOS:2;
        uint8_t DQOS:2;
        uint8_t :4;
    } bit;
    uint8_t reg;
} USB_QOSCTRL_Type;

typedef union {
    struct {
        uint16_t DETACH:1;
        uint16_t UPRSM:1;
        uint16_t SPDCONF:2;
        uint16_t NREPLY:1;
        uint16_t :3;
        uint16_t GNAK:1;
        uint16_t LPMHDSK:2;
        uint16_t :5;
    } bit;
    uint16_t reg;
} USB_DEVICE_CTRLB_Type;

typedef union {
    struct {
        uint8_t DADD:7;
        uint8_t ADDEN:1;
    } bit;
    uint8_t reg;
} USB_DEVICE_DADD_Type;

typedef union {
    struct {
        uint8_t :2;
        uint8_t SPEED:2;
        uint8_t :2;
        uint8_t LINESTATE:2;
    } bit;
    uint8_t reg;
} USB_DEVICE_STATUS_Type;

typedef union {
    struct {
        uint16_t MFNUM:3;
        uint16_t FNUM:11;
        uint16_t :1;
        uint16_t FNCERR:1;
    } bit;
    uint16_t reg;
} USB_DEVICE_FNUM_Type;

typedef union {
    struct {
        uint16_t SUSPEND:1;
        uint16_t :1;
        uint16_t SOF:1;
        uint16_t EORST:1;
        uint16_t WAKEUP:1;
        uint16_t EORSM:1;
        uint16_t UPRSM:1;
        uint16_t RAMACER:1;
        uint16_t LPMNYET:1;
        uint16_t LPMSUSP:1;
        uint16_t :6;
    } bit;
    uint16_t reg;
} USB_DEVICE_INTENCLR_Type;

typedef union {
    struct {
        uint16_t SUSPEND:1;
        uint16_t :1;
        uint16_t SOF:1;
        uint16_t EORST:1;
        uint16_t WAKEUP:1;
        uint16_t EORSM:1;
        uint16_t UPRSM:1;
        uint16_t RAMACER:1;
        uint16_t LPMNYET:1;
        uint16_t LPMSUSP:1;
        uint16_t :6;
    } bit;
    uint16_t reg;
} USB_DEVICE_INTENSET_Type;

typedef union {
    struct {
        uint16_t SUSPEND:1;
        uint16_t :1;
        uint16_t SOF:1;
        uint16_t EORST:1;
        uint16_t WAKEUP:1;
        uint16_t EORSM:1;
        uint16_t UPRSM:1;
        uint16_t RAMACER:1;
        uint16_t LPMNYET:1;
        uint16_t LPMSUSP:1;
        uint16_t :6;
    } bit;
    uint16_t reg;
} USB_DEVICE_INTFLAG_Type;

typedef union {
    uint16_t reg;
} USB_DEVICE_EPINTSMRY_Type;

typedef union {
    uint32_t reg;
} USB_DESCADD_Type;

typedef union {
    struct {
        uint16_t TRANSP:5;
        uint16_t :1;
        uint16_t TRANSN:5;
        uint16_t :1;
        uint16_t TRIM:3;
        uint16_t :1;
    } bit;
    uint16_t reg;
} USB_PADCAL_Type;

/* ======================================================================
 * v2-style endpoint register union types
 * ====================================================================== */

typedef union {
    struct {
        uint8_t EPTYPE0:3;
        uint8_t :1;
        uint8_t EPTYPE1:3;
        uint8_t NYETDIS:1;
    } bit;
    uint8_t reg;
} USB_DEVICE_EPCFG_Type;

typedef union { uint8_t reg; } USB_DEVICE_EPSTATUSCLR_Type;
typedef union { uint8_t reg; } USB_DEVICE_EPSTATUSSET_Type;
typedef union { uint8_t reg; } USB_DEVICE_EPSTATUS_Type;

typedef union {
    struct {
        uint8_t TRCPT0:1;
        uint8_t TRCPT1:1;
        uint8_t TRFAIL0:1;
        uint8_t TRFAIL1:1;
        uint8_t RXSTP:1;
        uint8_t STALL0:1;
        uint8_t STALL1:1;
        uint8_t :1;
    } bit;
    uint8_t reg;
} USB_DEVICE_EPINTFLAG_Type;

typedef union { uint8_t reg; } USB_DEVICE_EPINTENCLR_Type;

typedef union {
    struct {
        uint8_t TRCPT0:1;
        uint8_t TRCPT1:1;
        uint8_t TRFAIL0:1;
        uint8_t TRFAIL1:1;
        uint8_t RXSTP:1;
        uint8_t STALL0:1;
        uint8_t STALL1:1;
        uint8_t :1;
    } bit;
    uint8_t reg;
} USB_DEVICE_EPINTENSET_Type;

/* ======================================================================
 * v2-style descriptor bank union types
 * ====================================================================== */

typedef union { uint32_t reg; } USB_DEVICE_ADDR_Type;

typedef union {
    struct {
        uint32_t BYTE_COUNT:14;
        uint32_t MULTI_PACKET_SIZE:14;
        uint32_t SIZE:3;
        uint32_t AUTO_ZLP:1;
    } bit;
    uint32_t reg;
} USB_DEVICE_PCKSIZE_Type;

typedef union { uint16_t reg; } USB_DEVICE_EXTREG_Type;
typedef union { uint8_t  reg; } USB_DEVICE_STATUS_BK_Type;

/* ======================================================================
 * v2-style aggregate structs
 * ====================================================================== */

typedef struct {
    USB_DEVICE_ADDR_Type        ADDR;         /* 0x00 */
    USB_DEVICE_PCKSIZE_Type     PCKSIZE;      /* 0x04 */
    USB_DEVICE_EXTREG_Type      EXTREG;       /* 0x08 */
    USB_DEVICE_STATUS_BK_Type   STATUS_BK;    /* 0x0A */
    uint8_t                     Reserved1[5]; /* 0x0B-0x0F */
} UsbDeviceDescBank;

typedef struct {
    USB_DEVICE_EPCFG_Type       EPCFG;        /* 0x00 */
    uint8_t                     Reserved1[3]; /* 0x01-0x03 */
    USB_DEVICE_EPSTATUSCLR_Type EPSTATUSCLR;  /* 0x04 */
    USB_DEVICE_EPSTATUSSET_Type EPSTATUSSET;  /* 0x05 */
    USB_DEVICE_EPSTATUS_Type    EPSTATUS;     /* 0x06 */
    USB_DEVICE_EPINTFLAG_Type   EPINTFLAG;    /* 0x07 */
    USB_DEVICE_EPINTENCLR_Type  EPINTENCLR;   /* 0x08 */
    USB_DEVICE_EPINTENSET_Type  EPINTENSET;   /* 0x09 */
    uint8_t                     Reserved2[22]; /* 0x0A-0x1F */
} UsbDeviceEndpoint;

typedef struct {
    USB_CTRLA_Type               CTRLA;        /* 0x00 */
    uint8_t                      Reserved1[1]; /* 0x01 */
    USB_SYNCBUSY_Type            SYNCBUSY;     /* 0x02 */
    USB_QOSCTRL_Type             QOSCTRL;      /* 0x03 */
    uint8_t                      Reserved2[4]; /* 0x04-0x07 */
    USB_DEVICE_CTRLB_Type        CTRLB;        /* 0x08 */
    USB_DEVICE_DADD_Type         DADD;         /* 0x0A */
    uint8_t                      Reserved3[1]; /* 0x0B */
    USB_DEVICE_STATUS_Type       STATUS;       /* 0x0C */
    uint8_t                      FSMSTATUS;    /* 0x0D */
    uint8_t                      Reserved4[2]; /* 0x0E-0x0F */
    USB_DEVICE_FNUM_Type         FNUM;         /* 0x10 */
    uint8_t                      Reserved5[2]; /* 0x12-0x13 */
    USB_DEVICE_INTENCLR_Type     INTENCLR;     /* 0x14 */
    uint8_t                      Reserved6[2]; /* 0x16-0x17 */
    USB_DEVICE_INTENSET_Type     INTENSET;     /* 0x18 */
    uint8_t                      Reserved7[2]; /* 0x1A-0x1B */
    USB_DEVICE_INTFLAG_Type      INTFLAG;      /* 0x1C */
    uint8_t                      Reserved8[2]; /* 0x1E-0x1F */
    USB_DEVICE_EPINTSMRY_Type    EPINTSMRY;    /* 0x20 */
    uint8_t                      Reserved9[2]; /* 0x22-0x23 */
    USB_DESCADD_Type             DESCADD;      /* 0x24 */
    USB_PADCAL_Type              PADCAL;       /* 0x28 */
    uint8_t                      Reserved10[0xD6]; /* 0x2A-0xFF */
    UsbDeviceEndpoint            DeviceEndpoint[8]; /* 0x100 */
} UsbDevice;

typedef union {
    UsbDevice DEVICE;
} Usb;

/* Override the v3 USB_REGS macro with v2-compatible "USB" name */
#ifdef USB
#undef USB
#endif
#define USB ((Usb *)0x41000000UL)

/* ======================================================================
 * v2-style IRQ names (v3 renamed these)
 * ====================================================================== */
#define USB_0_IRQn  USB_OTHER_IRQn
#define USB_1_IRQn  USB_SOF_HSOF_IRQn
#define USB_2_IRQn  USB_TRCPT0_IRQn
#define USB_3_IRQn  USB_TRCPT1_IRQn

/* ======================================================================
 * v2-style bare constant macros → v3 _Msk equivalents
 *
 * DFP v2 provided bare names (e.g. USB_CTRLA_ENABLE) as bitmask values.
 * DFP v3 only provides USB_CTRLA_ENABLE_Msk (and a function-like macro).
 * ====================================================================== */

/* CTRLA */
#undef USB_CTRLA_ENABLE
#define USB_CTRLA_ENABLE    USB_CTRLA_ENABLE_Msk
#undef USB_CTRLA_RUNSTDBY
#define USB_CTRLA_RUNSTDBY  USB_CTRLA_RUNSTDBY_Msk

/* Device INTENSET */
#undef USB_DEVICE_INTENSET_EORST
#define USB_DEVICE_INTENSET_EORST    USB_DEVICE_INTENSET_EORST_Msk
#undef USB_DEVICE_INTENSET_SUSPEND
#define USB_DEVICE_INTENSET_SUSPEND  USB_DEVICE_INTENSET_SUSPEND_Msk

/* Device INTENCLR */
#undef USB_DEVICE_INTENCLR_SUSPEND
#define USB_DEVICE_INTENCLR_SUSPEND  USB_DEVICE_INTENCLR_SUSPEND_Msk

/* Device CTRLB */
#undef USB_DEVICE_CTRLB_DETACH
#define USB_DEVICE_CTRLB_DETACH      USB_DEVICE_CTRLB_DETACH_Msk

/* Device DADD */
#undef USB_DEVICE_DADD_ADDEN
#define USB_DEVICE_DADD_ADDEN        USB_DEVICE_DADD_ADDEN_Msk

/* Device INTFLAG */
#undef USB_DEVICE_INTFLAG_SOF
#define USB_DEVICE_INTFLAG_SOF       USB_DEVICE_INTFLAG_SOF_Msk
#undef USB_DEVICE_INTFLAG_SUSPEND
#define USB_DEVICE_INTFLAG_SUSPEND   USB_DEVICE_INTFLAG_SUSPEND_Msk
#undef USB_DEVICE_INTFLAG_WAKEUP
#define USB_DEVICE_INTFLAG_WAKEUP    USB_DEVICE_INTFLAG_WAKEUP_Msk
#undef USB_DEVICE_INTFLAG_EORST
#define USB_DEVICE_INTFLAG_EORST     USB_DEVICE_INTFLAG_EORST_Msk

/* Endpoint EPINTENSET */
#undef USB_DEVICE_EPINTENSET_TRCPT0
#define USB_DEVICE_EPINTENSET_TRCPT0 USB_DEVICE_EPINTENSET_TRCPT0_Msk
#undef USB_DEVICE_EPINTENSET_TRCPT1
#define USB_DEVICE_EPINTENSET_TRCPT1 USB_DEVICE_EPINTENSET_TRCPT1_Msk
#undef USB_DEVICE_EPINTENSET_RXSTP
#define USB_DEVICE_EPINTENSET_RXSTP  USB_DEVICE_EPINTENSET_RXSTP_Msk

/* Endpoint EPSTATUSCLR */
#undef USB_DEVICE_EPSTATUSCLR_STALLRQ0
#define USB_DEVICE_EPSTATUSCLR_STALLRQ0 USB_DEVICE_EPSTATUSCLR_STALLRQ0_Msk
#undef USB_DEVICE_EPSTATUSCLR_STALLRQ1
#define USB_DEVICE_EPSTATUSCLR_STALLRQ1 USB_DEVICE_EPSTATUSCLR_STALLRQ1_Msk
#undef USB_DEVICE_EPSTATUSCLR_DTGLOUT
#define USB_DEVICE_EPSTATUSCLR_DTGLOUT  USB_DEVICE_EPSTATUSCLR_DTGLOUT_Msk
#undef USB_DEVICE_EPSTATUSCLR_DTGLIN
#define USB_DEVICE_EPSTATUSCLR_DTGLIN   USB_DEVICE_EPSTATUSCLR_DTGLIN_Msk
#undef USB_DEVICE_EPSTATUSCLR_BK0RDY
#define USB_DEVICE_EPSTATUSCLR_BK0RDY   USB_DEVICE_EPSTATUSCLR_BK0RDY_Msk

/* Endpoint EPSTATUSSET */
#undef USB_DEVICE_EPSTATUSSET_BK1RDY
#define USB_DEVICE_EPSTATUSSET_BK1RDY   USB_DEVICE_EPSTATUSSET_BK1RDY_Msk
#undef USB_DEVICE_EPSTATUSSET_STALLRQ0
#define USB_DEVICE_EPSTATUSSET_STALLRQ0 USB_DEVICE_EPSTATUSSET_STALLRQ0_Msk
#undef USB_DEVICE_EPSTATUSSET_STALLRQ1
#define USB_DEVICE_EPSTATUSSET_STALLRQ1 USB_DEVICE_EPSTATUSSET_STALLRQ1_Msk

/* Endpoint EPINTFLAG */
#undef USB_DEVICE_EPINTFLAG_TRCPT0
#define USB_DEVICE_EPINTFLAG_TRCPT0  USB_DEVICE_EPINTFLAG_TRCPT0_Msk
#undef USB_DEVICE_EPINTFLAG_TRCPT1
#define USB_DEVICE_EPINTFLAG_TRCPT1  USB_DEVICE_EPINTFLAG_TRCPT1_Msk
#undef USB_DEVICE_EPINTFLAG_TRFAIL0
#define USB_DEVICE_EPINTFLAG_TRFAIL0 USB_DEVICE_EPINTFLAG_TRFAIL0_Msk
#undef USB_DEVICE_EPINTFLAG_TRFAIL1
#define USB_DEVICE_EPINTFLAG_TRFAIL1 USB_DEVICE_EPINTFLAG_TRFAIL1_Msk
#undef USB_DEVICE_EPINTFLAG_RXSTP
#define USB_DEVICE_EPINTFLAG_RXSTP   USB_DEVICE_EPINTFLAG_RXSTP_Msk

/* ======================================================================
 * v2-style USB fuse calibration macros
 *
 * In DFP v3, these are at SW0_FUSES_REGS offset 0x04 (FUSES_SW0_WORD_1).
 * In DFP v2, TinyUSB expects USB_FUSES_*_ADDR / _Msk / _Pos macros.
 * ====================================================================== */

#define USB_FUSES_TRANSP_ADDR  (0x00800080UL + 4)
#define USB_FUSES_TRANSP_Pos   FUSES_SW0_WORD_1_USB_TRANSP_Pos
#define USB_FUSES_TRANSP_Msk   FUSES_SW0_WORD_1_USB_TRANSP_Msk

#define USB_FUSES_TRANSN_ADDR  (0x00800080UL + 4)
#define USB_FUSES_TRANSN_Pos   FUSES_SW0_WORD_1_USB_TRANSN_Pos
#define USB_FUSES_TRANSN_Msk   FUSES_SW0_WORD_1_USB_TRANSN_Msk

#define USB_FUSES_TRIM_ADDR    (0x00800080UL + 4)
#define USB_FUSES_TRIM_Pos     FUSES_SW0_WORD_1_USB_TRIM_Pos
#define USB_FUSES_TRIM_Msk     FUSES_SW0_WORD_1_USB_TRIM_Msk

#endif /* FW_ENABLE_USB */
#endif /* TUSB_DFP_V3_COMPAT_H */
