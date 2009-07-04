// Support for generating ACPI tables (on emulators)
//
// Copyright (C) 2008  Kevin O'Connor <kevin@koconnor.net>
// Copyright (C) 2006 Fabrice Bellard
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "acpi.h" // struct rsdp_descriptor
#include "util.h" // memcpy
#include "memmap.h" // bios_table_cur_addr
#include "pci.h" // pci_find_device
#include "biosvar.h" // GET_EBDA
#include "pci_ids.h" // PCI_VENDOR_ID_INTEL
#include "pci_regs.h" // PCI_INTERRUPT_LINE


/****************************************************/
/* ACPI tables init */

/* Table structure from Linux kernel (the ACPI tables are under the
   BSD license) */

#define ACPI_TABLE_HEADER_DEF   /* ACPI common table header */ \
    u32 signature;          /* ACPI signature (4 ASCII characters) */ \
    u32 length;                 /* Length of table, in bytes, including header */ \
    u8  revision;               /* ACPI Specification minor version # */ \
    u8  checksum;               /* To make sum of entire table == 0 */ \
    u8  oem_id [6];             /* OEM identification */ \
    u8  oem_table_id [8];       /* OEM table identification */ \
    u32 oem_revision;           /* OEM revision number */ \
    u8  asl_compiler_id [4];    /* ASL compiler vendor ID */ \
    u32 asl_compiler_revision;  /* ASL compiler revision number */


struct acpi_table_header         /* ACPI common table header */
{
    ACPI_TABLE_HEADER_DEF
} PACKED;

/*
 * ACPI 1.0 Root System Description Table (RSDT)
 */
#define RSDT_SIGNATURE 0x54445352 // RSDT
struct rsdt_descriptor_rev1
{
    ACPI_TABLE_HEADER_DEF       /* ACPI common table header */
    u32 table_offset_entry [3]; /* Array of pointers to other */
    /* ACPI tables */
} PACKED;

/*
 * ACPI 1.0 Firmware ACPI Control Structure (FACS)
 */
#define FACS_SIGNATURE 0x53434146 // FACS
struct facs_descriptor_rev1
{
    u32 signature;           /* ACPI Signature */
    u32 length;                 /* Length of structure, in bytes */
    u32 hardware_signature;     /* Hardware configuration signature */
    u32 firmware_waking_vector; /* ACPI OS waking vector */
    u32 global_lock;            /* Global Lock */
    u32 S4bios_f        : 1;    /* Indicates if S4BIOS support is present */
    u32 reserved1       : 31;   /* Must be 0 */
    u8  resverved3 [40];        /* Reserved - must be zero */
} PACKED;


/*
 * ACPI 1.0 Fixed ACPI Description Table (FADT)
 */
#define FACP_SIGNATURE 0x50434146 // FACP
struct fadt_descriptor_rev1
{
    ACPI_TABLE_HEADER_DEF     /* ACPI common table header */
    u32 firmware_ctrl;          /* Physical address of FACS */
    u32 dsdt;                   /* Physical address of DSDT */
    u8  model;                  /* System Interrupt Model */
    u8  reserved1;              /* Reserved */
    u16 sci_int;                /* System vector of SCI interrupt */
    u32 smi_cmd;                /* Port address of SMI command port */
    u8  acpi_enable;            /* Value to write to smi_cmd to enable ACPI */
    u8  acpi_disable;           /* Value to write to smi_cmd to disable ACPI */
    u8  S4bios_req;             /* Value to write to SMI CMD to enter S4BIOS state */
    u8  reserved2;              /* Reserved - must be zero */
    u32 pm1a_evt_blk;           /* Port address of Power Mgt 1a acpi_event Reg Blk */
    u32 pm1b_evt_blk;           /* Port address of Power Mgt 1b acpi_event Reg Blk */
    u32 pm1a_cnt_blk;           /* Port address of Power Mgt 1a Control Reg Blk */
    u32 pm1b_cnt_blk;           /* Port address of Power Mgt 1b Control Reg Blk */
    u32 pm2_cnt_blk;            /* Port address of Power Mgt 2 Control Reg Blk */
    u32 pm_tmr_blk;             /* Port address of Power Mgt Timer Ctrl Reg Blk */
    u32 gpe0_blk;               /* Port addr of General Purpose acpi_event 0 Reg Blk */
    u32 gpe1_blk;               /* Port addr of General Purpose acpi_event 1 Reg Blk */
    u8  pm1_evt_len;            /* Byte length of ports at pm1_x_evt_blk */
    u8  pm1_cnt_len;            /* Byte length of ports at pm1_x_cnt_blk */
    u8  pm2_cnt_len;            /* Byte Length of ports at pm2_cnt_blk */
    u8  pm_tmr_len;             /* Byte Length of ports at pm_tm_blk */
    u8  gpe0_blk_len;           /* Byte Length of ports at gpe0_blk */
    u8  gpe1_blk_len;           /* Byte Length of ports at gpe1_blk */
    u8  gpe1_base;              /* Offset in gpe model where gpe1 events start */
    u8  reserved3;              /* Reserved */
    u16 plvl2_lat;              /* Worst case HW latency to enter/exit C2 state */
    u16 plvl3_lat;              /* Worst case HW latency to enter/exit C3 state */
    u16 flush_size;             /* Size of area read to flush caches */
    u16 flush_stride;           /* Stride used in flushing caches */
    u8  duty_offset;            /* Bit location of duty cycle field in p_cnt reg */
    u8  duty_width;             /* Bit width of duty cycle field in p_cnt reg */
    u8  day_alrm;               /* Index to day-of-month alarm in RTC CMOS RAM */
    u8  mon_alrm;               /* Index to month-of-year alarm in RTC CMOS RAM */
    u8  century;                /* Index to century in RTC CMOS RAM */
    u8  reserved4;              /* Reserved */
    u8  reserved4a;             /* Reserved */
    u8  reserved4b;             /* Reserved */
#if 0
    u32 wb_invd         : 1;    /* The wbinvd instruction works properly */
    u32 wb_invd_flush   : 1;    /* The wbinvd flushes but does not invalidate */
    u32 proc_c1         : 1;    /* All processors support C1 state */
    u32 plvl2_up        : 1;    /* C2 state works on MP system */
    u32 pwr_button      : 1;    /* Power button is handled as a generic feature */
    u32 sleep_button    : 1;    /* Sleep button is handled as a generic feature, or not present */
    u32 fixed_rTC       : 1;    /* RTC wakeup stat not in fixed register space */
    u32 rtcs4           : 1;    /* RTC wakeup stat not possible from S4 */
    u32 tmr_val_ext     : 1;    /* The tmr_val width is 32 bits (0 = 24 bits) */
    u32 reserved5       : 23;   /* Reserved - must be zero */
#else
    u32 flags;
#endif
} PACKED;

/*
 * MADT values and structures
 */

/* Values for MADT PCATCompat */

#define DUAL_PIC                0
#define MULTIPLE_APIC           1


/* Master MADT */

#define APIC_SIGNATURE 0x43495041 // APIC
struct multiple_apic_table
{
    ACPI_TABLE_HEADER_DEF     /* ACPI common table header */
    u32 local_apic_address;     /* Physical address of local APIC */
#if 0
    u32 PCATcompat      : 1;    /* A one indicates system also has dual 8259s */
    u32 reserved1       : 31;
#else
    u32 flags;
#endif
} PACKED;


/* Values for Type in APIC_HEADER_DEF */

#define APIC_PROCESSOR          0
#define APIC_IO                 1
#define APIC_XRUPT_OVERRIDE     2
#define APIC_NMI                3
#define APIC_LOCAL_NMI          4
#define APIC_ADDRESS_OVERRIDE   5
#define APIC_IO_SAPIC           6
#define APIC_LOCAL_SAPIC        7
#define APIC_XRUPT_SOURCE       8
#define APIC_RESERVED           9           /* 9 and greater are reserved */

/*
 * MADT sub-structures (Follow MULTIPLE_APIC_DESCRIPTION_TABLE)
 */
#define APIC_HEADER_DEF   /* Common APIC sub-structure header */\
    u8  type;                               \
    u8  length;

/* Sub-structures for MADT */

struct madt_processor_apic
{
    APIC_HEADER_DEF
    u8  processor_id;           /* ACPI processor id */
    u8  local_apic_id;          /* Processor's local APIC id */
#if 0
    u32 processor_enabled: 1;   /* Processor is usable if set */
    u32 reserved2       : 31;   /* Reserved, must be zero */
#else
    u32 flags;
#endif
} PACKED;

struct madt_io_apic
{
    APIC_HEADER_DEF
    u8  io_apic_id;             /* I/O APIC ID */
    u8  reserved;               /* Reserved - must be zero */
    u32 address;                /* APIC physical address */
    u32 interrupt;              /* Global system interrupt where INTI
                                 * lines start */
} PACKED;

#if CONFIG_KVM
/* IRQs 5,9,10,11 */
#define PCI_ISA_IRQ_MASK    0x0e20
#else
#define PCI_ISA_IRQ_MASK    0x0000
#endif

struct madt_intsrcovr {
    APIC_HEADER_DEF
    u8  bus;
    u8  source;
    u32 gsi;
    u16 flags;
} PACKED;

#include "acpi-dsdt.hex"

static inline u16 cpu_to_le16(u16 x)
{
    return x;
}

static inline u32 cpu_to_le32(u32 x)
{
    return x;
}

static void acpi_build_table_header(struct acpi_table_header *h,
                                    u32 sig, int len, u8 rev)
{
    h->signature = sig;
    h->length = cpu_to_le32(len);
    h->revision = rev;
    memcpy(h->oem_id, CONFIG_APPNAME6, 6);
    memcpy(h->oem_table_id, CONFIG_APPNAME4, 4);
    memcpy(h->asl_compiler_id, CONFIG_APPNAME4, 4);
    memcpy(h->oem_table_id + 4, (void*)&sig, 4);
    h->oem_revision = cpu_to_le32(1);
    h->asl_compiler_revision = cpu_to_le32(1);
    h->checksum -= checksum(h, len);
}

#define SSDT_SIGNATURE 0x54445353// SSDT
static int
acpi_build_processor_ssdt(u8 *ssdt)
{
    u8 *ssdt_ptr = ssdt;
    int i, length;
    int smp_cpus = CountCPUs;
    int acpi_cpus = smp_cpus > 0xff ? 0xff : smp_cpus;

    ssdt_ptr[9] = 0; // checksum;
    ssdt_ptr += sizeof(struct acpi_table_header);

    // caluculate the length of processor block and scope block excluding PkgLength
    length = 0x0d * acpi_cpus + 4;

    // build processor scope header
    *(ssdt_ptr++) = 0x10; // ScopeOp
    if (length <= 0x3e) {
        *(ssdt_ptr++) = length + 1;
    } else {
        *(ssdt_ptr++) = 0x7F;
        *(ssdt_ptr++) = (length + 2) >> 6;
    }
    *(ssdt_ptr++) = '_'; // Name
    *(ssdt_ptr++) = 'P';
    *(ssdt_ptr++) = 'R';
    *(ssdt_ptr++) = '_';

    // build object for each processor
    for(i=0;i<acpi_cpus;i++) {
        *(ssdt_ptr++) = 0x5B; // ProcessorOp
        *(ssdt_ptr++) = 0x83;
        *(ssdt_ptr++) = 0x0B; // Length
        *(ssdt_ptr++) = 'C';  // Name (CPUxx)
        *(ssdt_ptr++) = 'P';
        if ((i & 0xf0) != 0)
            *(ssdt_ptr++) = (i >> 4) < 0xa ? (i >> 4) + '0' : (i >> 4) + 'A' - 0xa;
        else
            *(ssdt_ptr++) = 'U';
        *(ssdt_ptr++) = (i & 0xf) < 0xa ? (i & 0xf) + '0' : (i & 0xf) + 'A' - 0xa;
        *(ssdt_ptr++) = i;
        *(ssdt_ptr++) = 0x10; // Processor block address
        *(ssdt_ptr++) = 0xb0;
        *(ssdt_ptr++) = 0;
        *(ssdt_ptr++) = 0;
        *(ssdt_ptr++) = 6;    // Processor block length
    }

    acpi_build_table_header((struct acpi_table_header *)ssdt,
                            SSDT_SIGNATURE, ssdt_ptr - ssdt, 1);

    return ssdt_ptr - ssdt;
}

struct rsdp_descriptor *RsdpAddr;

/* base_addr must be a multiple of 4KB */
void acpi_bios_init(void)
{
    if (! CONFIG_ACPI)
        return;

    dprintf(3, "init ACPI tables\n");

    // This code is hardcoded for PIIX4 Power Management device.
    int bdf = pci_find_device(PCI_VENDOR_ID_INTEL
                              , PCI_DEVICE_ID_INTEL_82371AB_3);
    if (bdf < 0)
        // Device not found
        return;

    struct rsdp_descriptor *rsdp;
    struct rsdt_descriptor_rev1 *rsdt;
    struct fadt_descriptor_rev1 *fadt;
    struct facs_descriptor_rev1 *facs;
    struct multiple_apic_table *madt;
    u8 *dsdt, *ssdt;
    u32 base_addr, rsdt_addr, fadt_addr, addr, facs_addr, dsdt_addr, ssdt_addr;
    u32 acpi_tables_size, madt_addr, madt_size;
    int i;

    /* reserve memory space for tables */
    bios_table_cur_addr = ALIGN(bios_table_cur_addr, 16);
    rsdp = (void *)bios_table_cur_addr;
    bios_table_cur_addr += sizeof(*rsdp);

    addr = base_addr = RamSize - CONFIG_ACPI_DATA_SIZE;
    add_e820(addr, CONFIG_ACPI_DATA_SIZE, E820_ACPI);
    rsdt_addr = addr;
    rsdt = (void *)(addr);
    addr += sizeof(*rsdt);

    fadt_addr = addr;
    fadt = (void *)(addr);
    addr += sizeof(*fadt);

    addr = ALIGN(addr, 64);
    facs_addr = addr;
    facs = (void *)(addr);
    addr += sizeof(*facs);

    dsdt_addr = addr;
    dsdt = (void *)(addr);
    addr += sizeof(AmlCode);

    ssdt_addr = addr;
    ssdt = (void *)(addr);
    addr += acpi_build_processor_ssdt(ssdt);

    int smp_cpus = CountCPUs;
    addr = ALIGN(addr, 8);
    madt_addr = addr;
    madt_size = sizeof(*madt) +
        sizeof(struct madt_processor_apic) * smp_cpus +
        sizeof(struct madt_io_apic);
    madt = (void *)(addr);
    addr += madt_size;

    acpi_tables_size = addr - base_addr;

    dprintf(1, "ACPI tables: RSDP addr=0x%08lx"
            " ACPI DATA addr=0x%08lx size=0x%x\n",
            (unsigned long)rsdp,
            (unsigned long)rsdt, acpi_tables_size);

    /* RSDP */
    memset(rsdp, 0, sizeof(*rsdp));
    rsdp->signature = RSDP_SIGNATURE;
    memcpy(rsdp->oem_id, CONFIG_APPNAME6, 6);
    rsdp->rsdt_physical_address = cpu_to_le32(rsdt_addr);
    rsdp->checksum -= checksum(rsdp, 20);
    RsdpAddr = rsdp;

    /* RSDT */
    memset(rsdt, 0, sizeof(*rsdt));
    rsdt->table_offset_entry[0] = cpu_to_le32(fadt_addr);
    rsdt->table_offset_entry[1] = cpu_to_le32(madt_addr);
    rsdt->table_offset_entry[2] = cpu_to_le32(ssdt_addr);
    acpi_build_table_header((struct acpi_table_header *)rsdt,
                            RSDT_SIGNATURE, sizeof(*rsdt), 1);

    /* FADT */
    memset(fadt, 0, sizeof(*fadt));
    fadt->firmware_ctrl = cpu_to_le32(facs_addr);
    fadt->dsdt = cpu_to_le32(dsdt_addr);
    fadt->model = 1;
    fadt->reserved1 = 0;
    int pm_sci_int = pci_config_readb(bdf, PCI_INTERRUPT_LINE);
    fadt->sci_int = cpu_to_le16(pm_sci_int);
    fadt->smi_cmd = cpu_to_le32(PORT_SMI_CMD);
    fadt->acpi_enable = 0xf1;
    fadt->acpi_disable = 0xf0;
    fadt->pm1a_evt_blk = cpu_to_le32(PORT_ACPI_PM_BASE);
    fadt->pm1a_cnt_blk = cpu_to_le32(PORT_ACPI_PM_BASE + 0x04);
    fadt->pm_tmr_blk = cpu_to_le32(PORT_ACPI_PM_BASE + 0x08);
    fadt->pm1_evt_len = 4;
    fadt->pm1_cnt_len = 2;
    fadt->pm_tmr_len = 4;
    fadt->plvl2_lat = cpu_to_le16(0xfff); // C2 state not supported
    fadt->plvl3_lat = cpu_to_le16(0xfff); // C3 state not supported
    /* WBINVD + PROC_C1 + PWR_BUTTON + SLP_BUTTON + FIX_RTC */
    fadt->flags = cpu_to_le32((1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 6));
    acpi_build_table_header((struct acpi_table_header *)fadt, FACP_SIGNATURE,
                            sizeof(*fadt), 1);

    /* FACS */
    memset(facs, 0, sizeof(*facs));
    facs->signature = FACS_SIGNATURE;
    facs->length = cpu_to_le32(sizeof(*facs));

    /* DSDT */
    memcpy(dsdt, AmlCode, sizeof(AmlCode));

    /* MADT */
    memset(madt, 0, madt_size);
    madt->local_apic_address = cpu_to_le32(BUILD_APIC_ADDR);
    madt->flags = cpu_to_le32(1);
    struct madt_processor_apic *apic = (void *)&madt[1];
    for(i=0;i<smp_cpus;i++) {
        apic->type = APIC_PROCESSOR;
        apic->length = sizeof(*apic);
        apic->processor_id = i;
        apic->local_apic_id = i;
        apic->flags = cpu_to_le32(1);
        apic++;
    }
    struct madt_io_apic *io_apic = (void *)apic;
    io_apic->type = APIC_IO;
    io_apic->length = sizeof(*io_apic);
    io_apic->io_apic_id = smp_cpus;
    io_apic->address = cpu_to_le32(BUILD_IOAPIC_ADDR);
    io_apic->interrupt = cpu_to_le32(0);

    struct madt_intsrcovr *intsrcovr = (void*)&io_apic[1];
    for (i = 0; i < 16; i++) {
        if (!(PCI_ISA_IRQ_MASK & (1 << i)))
            /* No need for a INT source override structure. */
            continue;
        memset(intsrcovr, 0, sizeof(*intsrcovr));
        intsrcovr->type   = APIC_XRUPT_OVERRIDE;
        intsrcovr->length = sizeof(*intsrcovr);
        intsrcovr->source = i;
        intsrcovr->gsi    = i;
        intsrcovr->flags  = 0xd; /* active high, level triggered */
        intsrcovr++;
        madt_size += sizeof(struct madt_intsrcovr);
    }

    acpi_build_table_header((struct acpi_table_header *)madt,
                            APIC_SIGNATURE, madt_size, 1);
}

u32
find_resume_vector()
{
    dprintf(4, "rsdp=%p\n", RsdpAddr);
    if (!RsdpAddr || RsdpAddr->signature != RSDP_SIGNATURE)
        return 0;
    struct rsdt_descriptor_rev1 *rsdt = (void*)RsdpAddr->rsdt_physical_address;
    dprintf(4, "rsdt=%p\n", rsdt);
    if (!rsdt || rsdt->signature != RSDT_SIGNATURE)
        return 0;
    void *end = (void*)rsdt + rsdt->length;
    int i;
    for (i=0; (void*)&rsdt->table_offset_entry[i] < end; i++) {
        struct fadt_descriptor_rev1 *fadt = (void*)rsdt->table_offset_entry[i];
        if (!fadt || fadt->signature != FACP_SIGNATURE)
            continue;
        dprintf(4, "fadt=%p\n", fadt);
        struct facs_descriptor_rev1 *facs = (void*)fadt->firmware_ctrl;
        dprintf(4, "facs=%p\n", facs);
        if (! facs || facs->signature != FACS_SIGNATURE)
            return 0;
        // Found it.
        dprintf(4, "resume addr=%d\n", facs->firmware_waking_vector);
        return facs->firmware_waking_vector;
    }
    return 0;
}
