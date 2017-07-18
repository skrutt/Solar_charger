# Solar_charger
First iteration of a solar charging BMS

Goals is to have a low draw solar battery, managing the charge so that the battery will always be topped off and ready. This is for now atleast not intended to be a high power solution but more as a backup. Of course more is always better when it comes to solar power but for now i just want to get my feet wet. 

For now this system is intended to be used with a single bank of lipo cells(18650s), in the furure this build might be adapted to a 12 v lead battery instead and this will be kept in mind for the components, should only require small changes.

A PCB is intended to come for the future, here is a list of current features and then wanted features:

Current features:

Accurate battery voltage monitoring: this uses a separate sense wire to the positive battery pole and a semi heavy gauge to the negative for system ground.

Charging and load have their own heavy gauge negative wire and share positive with system for positive. This means there are four wires in total to the batteries, two load and two "sense", where the negative sense is also system ground. 

Charge regulation: Here we have a Nchannel mosfet for low side switching in the charge source, a 6v solar cell is intended to be used for charging, this is placed in "reverse", feeding current into the solar cell when active. this means it has a floating ground. A diode is also present to prevent back-feeding the solar cells, 

Load regulation: For now there is a single load output, activated when there is a high enough voltage on the battery, deactivated when the voltage drops.

Visualisation: An oled screen shows current battery voltage, and if charge or load is active.

Future features:

Current monitoring: Monitoring of both charge, system and load currents. 

No diode: will be looking into using a second mosfet to reduce voltage drop over diode to cells, will require active monitoring of solar cell voltage.

Multiple loads: Will be setting up multiple load connections, also these will have a timer on them.

Draw minimization: the system is quite power hungry for now, soon it will be set to minimize needed power by turning of screen, leds, reviewing the MCU clock and using sleep states. Goal is less than half a mA. User panel: The system will be controlled and monitored through a UI, this will include the oled, which will be activated by a button and have a timer. there will also be buttons for activating the loads and status of all these.

