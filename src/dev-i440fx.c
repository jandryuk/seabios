// initialization function which are specific to i440fx chipset
//
// Copyright (C) 2008  Kevin O'Connor <kevin@koconnor.net>
// Copyright (C) 2006 Fabrice Bellard
//
// Copyright (C) 2010 Isaku Yamahata <yamahata at valinux co jp>
// Split out from pciinit.c
//
// This file may be distributed under the terms of the GNU LGPLv3 license.
//

#include "config.h" // CONFIG_DEBUG_LEVEL
#include "util.h" // dprintf
#include "ioport.h" // outb
#include "pci.h" // pci_config_writeb
#include "pci_regs.h" // PCI_INTERRUPT_LINE
#include "acpi.h"
#include "dev-i440fx.h"

#define I440FX_PAM0     0x59

void i440fx_bios_make_writable(u16 bdf, void *arg)
{
    make_bios_writable_intel(bdf, I440FX_PAM0);
}

void i440fx_bios_make_readonly(u16 bdf, void *arg)
{
    make_bios_readonly_intel(bdf, I440FX_PAM0);
}

/* PIIX3/PIIX4 PCI to ISA bridge */
void piix_isa_bridge_init(u16 bdf, void *arg)
{
    int i, irq;
    u8 elcr[2];

    elcr[0] = 0x00;
    elcr[1] = 0x00;
    for (i = 0; i < 4; i++) {
        irq = pci_irqs[i];
        /* set to trigger level */
        elcr[irq >> 3] |= (1 << (irq & 7));
        /* activate irq remapping in PIIX */
        pci_config_writeb(bdf, 0x60 + i, irq);
    }
    outb(elcr[0], 0x4d0);
    outb(elcr[1], 0x4d1);
    dprintf(1, "PIIX3/PIIX4 init: elcr=%02x %02x\n", elcr[0], elcr[1]);
}

/* PIIX3/PIIX4 IDE */
void piix_ide_init(u16 bdf, void *arg)
{
    pci_config_writew(bdf, 0x40, 0x8000); // enable IDE0
    pci_config_writew(bdf, 0x42, 0x8000); // enable IDE1
    pci_bios_allocate_regions(bdf, NULL);
}

/* PIIX4 Power Management device (for ACPI) */
void piix4_pm_init(u16 bdf, void *arg)
{
    // acpi sci is hardwired to 9
    pci_config_writeb(bdf, PCI_INTERRUPT_LINE, 9);

    pci_config_writel(bdf, 0x40, PORT_ACPI_PM_BASE | 1);
    pci_config_writeb(bdf, 0x80, 0x01); /* enable PM io space */
    pci_config_writel(bdf, 0x90, PORT_SMB_BASE | 1);
    pci_config_writeb(bdf, 0xd2, 0x09); /* enable SMBus io space */
}

#define PIIX4_ACPI_ENABLE       0xf1
#define PIIX4_ACPI_DISABLE      0xf0
#define PIIX4_GPE0_BLK          0xafe0
#define PIIX4_GPE0_BLK_LEN      4

void piix4_fadt_init(u16 bdf, void *arg)
{
    struct fadt_descriptor_rev1 *fadt = arg;
    fadt->acpi_enable = PIIX4_ACPI_ENABLE;
    fadt->acpi_disable = PIIX4_ACPI_DISABLE;
    fadt->gpe0_blk = cpu_to_le32(PIIX4_GPE0_BLK);
    fadt->gpe0_blk_len = PIIX4_GPE0_BLK_LEN;
}
