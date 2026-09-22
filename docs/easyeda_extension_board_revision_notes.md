# EasyEDA Extension Board Revision Notes

This note records the current EasyEDA project state before schematic changes.

## Current Status

- Source project: `<local-easyeda-project>.epro2`
- EasyEDA version observed: `3.2.91`
- The `Run API Gateway` extension was installed, but the Bridge service did not
  connect reliably.
- The `.epro2` file is a zip archive containing an EasyEDA V3 `.epru` log file.
- The source log contains one schematic page and one PCB document:

| Document | Purpose |
| --- | --- |
| `SCH_PAGE` | Schematic page to revise first |
| `PCB` | Existing PCB layout, do not edit until the schematic is reviewed |

## Safe Editing Rule

Do not directly edit the PCB stage yet. Revise and review the schematic first,
then regenerate or update PCB after the pin map is confirmed.

Before writing back to the EasyEDA project file:

1. Create a backup copy of the original `.epro2`.
2. Modify only the schematic-page records.
3. Open the modified project in EasyEDA and visually inspect the schematic.
4. Do not route or update PCB until the schematic has been approved.

## Target Hardware Interfaces

Use board labels in the schematic and documentation, for example `A8`, `B12`,
not MCU package names.

| Interface | Preferred pins | Notes |
| --- | --- | --- |
| Debug UART | A10 TX / A11 RX | Board Type-C CH340E |
| MaixCAM2 UART | A8 TX / A9 RX | Vision link |
| X42S RS485 | B12 TX / B13 RX | Automatic-direction RS485 module |
| RS485 direction reserve | B14 | Leave optional for manual-direction modules |
| TJC X2 7-inch screen | B15 TX / B16 RX | 115200 serial HMI |
| IMU660RB | VCC/GND/SCL/SDA/SA0/CS | Keep only these six signals |
| TB6612 dual motor | A12/A13/A26/A27 plus PWM pins | Match car firmware after final confirmation |
| Grayscale sensor | 8 digital channels | Current firmware has seven verified channels; add one reserve channel |
| Electromagnet MOSFET board | Power/GND/control terminal | Keep control signal isolated from motor power |

## Keep Free / Avoid

| Resource | Reason |
| --- | --- |
| A19/A20 | SWD download/debug |
| B6/B7/B8/B9 | Board SPI Flash area |
| B21 | User key |
| B22 | User LED |

## Next Step

Use the parsed schematic source to prepare a schematic-only revision. Stop for
visual review before touching PCB layout.
