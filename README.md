# Rfid-IO
Simple RFID devices manager with Arduino and MFRC522

### Capacities
- Dump sector(s)
- Display UUID
- Change public key to custom private
  - Get/Set custom ID

### Preview
*Case and card/badge devices*
<p align="center">
  <img width="30%" src="/docs/rfid_case_small.jpg"/> <img width="30%" src="/docs/card_hardware_smaller.jpg"/> <img width="30%" src="/docs/badge_hardware_smaller.jpg"/>
</p>

*Serial program: display both devices IDs and change card to TEST*
<p align="center">
  <img src="/docs/rfid_serial_preview.gif"/>
</p>
 
### Schematic
<p align="center">
  <img src="/docs/schematics_bb.jpg"/>
</p>

### Notes
Even if the private key is changed on one of the sector and that it can not technically be read without it, it is important to know that there are exploit that can bypass this protection.

Because of the timeout issue on the side of the RFID library, you should retrieve and put back the nfc device on the case with each command to avoid any problem.
