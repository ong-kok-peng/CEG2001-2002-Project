<h1>CEG2001-2002 Sensors & Control + Embedded Systems Joint Module Project</h1>
<h2>Rain Sense system using sensors and actuators</h2>
<p>What it does: 
  <ol>
    <li>rainfall detection</li>
    <li>rainfall intensity measurement</li>
    <li>ambient sunlight intensity measurement</li>
  </ol>
</p>
<p>Sensors & actuators used
  <ol>
    <li>4 LDRs light intensity measurement assembly</li>
    <li>DHT22 temp and humidity sensor</li>
    <li>generic rain sensor</li>
    <li>SG90 servo motor</li>
  </ol>
</p>
<p>MCUs used
  <ol>
    <li>MSP430F5529 - read sensors and control actuators</li>
    <li>Arduino UNO R3 - displays information on OLED screen</li>
  </ol>
</p>
<h2>How to use and run</h2>
<p>Software used
  <ol>
    <li>TI Code Composer Studio v12.8.1</li>
    <li>Arduino IDE v1.8.19 or > v2.x.x</li>
  </ol>
</p>
<p>Usage instructions
  <ol>
    <li>Clone or download the repo</li>
    <li>Open CCS and create a new CCS project</li>
    <li>On file explorer navigate to the CCS workspace folder aka "ccs_workspace_v12" and into the newly project folder</li>
    <li>Copy the whole external_sources folder and main.c file to the root of that project folder</li>
    <li>Reload CCS. If files are not shown, add files to project</li>
    <li>Plug MSP430 to the PC. Build project and debug. Stop debugging and load into flash.</li>
    <li>Open serial terminal with 115200 Baud, and enjoy!</li>
  </ol>
</p>
