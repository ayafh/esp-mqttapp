# ESP32 MQTT Sensor Publisher

This project runs on an **ESP32** using the **ESP-IDF** framework. It reads **4 digital inputs** and **2 analog values (ADC)**, and publishes the data every 2 seconds to **individual MQTT topics** in plain **JSON format**. The MQTT messages are consumed by **Telegraf** and can be forwarded to **InfluxDB** or logged to file.

The system is built using an ESP32 microcontroller programmed in C with the ESP-IDF framework. After
flashing the firmware to the ESP32, the board connects to the Wi-Fi network and establishes a connection
to a locally hosted Mosquitto MQTT broker . The ESP32 periodically
(every 2 seconds) reads data from four digital GPIO inputs and two analog ADC channels. Each sensor value
is wrapped in a JSON object of the form {"value": <reading>} and published to its respective MQTT
topic, such as /esp32/digital1, /esp32/analog2, etc.
On the host machine, Mosquitto runs as the MQTT broker, receiving the published messages
from the ESP32. then Telegraf which is a lightweight metrics collection agent, is then configured to subscribe to the same
MQTT topics using its mqtt consumer plugin. The relevant configuration specifies the broker address, topics
to monitor, data format as json, and a JSON path for extracting the numerical value. The extracted sensor
readings are then forwarded to an InfluxDB database using Telegraf’s influxdb v2 output plugin where
they can be stored and visualized.

## Requirements

### Hardware:
- ESP32 Dev Board

### Software:
- [ESP-IDF v5+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- Mosquitto (MQTT broker)
- Telegraf
- InfluxDB (optional)


---

##  MQTT Topics Published

| Topic              | Description         | Format          |
|-------------------|---------------------|------------------|
| /esp32/digital1    | GPIO18 input        | `{"value": 0}`   |
| /esp32/digital2    | GPIO19 input        | `{"value": 1}`   |
| /esp32/digital3    | GPIO34 input        | `{"value": 0}`   |
| /esp32/digital4    | GPIO35 input        | `{"value": 1}`   |
| /esp32/analog1     | ADC GPIO36 (CH0)    | `{"value": 534}` |
| /esp32/analog2     | ADC GPIO39 (CH3)    | `{"value": 672}` |

---

##  MQTT Broker (Mosquitto)

### Install and start Mosquitto:

```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

### Allow external clients:

Edit `/etc/mosquitto/mosquitto.conf` and add:

```conf
listener 1883
allow_anonymous true
```

Restart:
```bash
sudo systemctl restart mosquitto
```

### Test MQTT:

```bash
# Terminal 1 - subscribe
mosquitto_sub -h localhost -t "/esp32/#" -v

# Terminal 2 - publish
mosquitto_pub -h localhost -t "/esp32/test" -m "Hello from PC"
```

---

##  Telegraf Setup

### Install:

```bash
sudo apt update
sudo apt install telegraf
```

### Create a config:

Edit `/etc/telegraf/telegraf.conf`:

----------------- Telegraf Default Configuration ------------------

[agent]
interval = "10s"
round_interval = true
metric_batch_size = 1000
metric_buffer_limit = 10000
collection_jitter = "0s"
flush_interval = "10s"
flush_jitter = "0s"
precision = ""
logfile = ""

[[outputs.{plugin_name}]]

Plugin Configuration
[[inputs.{plugin_name}]]

Plugin Configuration
[[aggregators.{plugin_name}]]

Plugin Configuration
[[processors.{plugin_name}]]

Plugin Configuration
----------------
Make sure to follow this format since it's the required TOML structure.
- in this project the sole aim is to collect data and visualize it ,
this is why processors and aggregators were not used.

### Start Telegraf:

```bash
sudo systemctl restart telegraf
sudo systemctl status telegraf
```

### View logs:

```bash
tail -f /var/log/telegraf_output.txt
```

Example output:
```bash
esp32_sensor,topic=/esp32/analog1 value="672"
esp32_sensor,topic=/esp32/digital3 value="1"
```

---

## InfluxDB Integration
In this project, we are working with InfluxDB Cloud.

Create an account at InfluxDB Cloud.

Create a bucket where the sensor data will be stored.

Generate an API token with write access.

These credentials are required in the Telegraf output configuration.

After MQTT starts publishing and the connection between Telegraf and the broker is successful:

Go to InfluxDB Cloud.

Access your bucket.

Select the metrics you want to visualize.

Click the “Run” button to display the graphs.




## Useful Commands

| Task                   | Command                                |
|------------------------|----------------------------------------|
| Build project          | `idf.py build`                         |
| Flash to ESP32         | `idf.py flash`                         |
| Monitor serial logs    | `idf.py monitor`                       |
| Subscribe to MQTT      | `mosquitto_sub -h localhost -t "/esp32/#"` |
| View Telegraf log      | `tail -f /var/log/telegraf_output.txt` |

---



## notes: 

feel free to modify and use anything , goodluck everyone! 

-Aya FAHMA & Douaa BENZINA at CDTA 
