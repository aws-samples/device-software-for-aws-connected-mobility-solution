# **CMS Device Example**

## **Introduction**

CMS(Connected Mobility Solution) Device Example is designed to be used in conjunction with the [AWS Connected Mobility Solution](https://aws.amazon.com/solutions/implementations/connected-mobility-solution/). While CMS builds the cloud framework, this project creates an example running in a real OBD device which hooks up with an ECU simulator and sends data to the cloud.

The following hardware are required:
1. OBD dongle [Freematics ONE + Model B](https://freematics.com/products/freematics-one-plus-model-b/) which runs this example code with SOC [EPS32](https://www.espressif.com/en/products/socs/esp32); 
2. An micro SD card with capacity more than 1G bytes;
3. An ECU simulator which supports OBD-II protocol.

## **Cloning code**

Clone this repositoty and its dependencies under your project directory like "~/CMS":
>`cd ~/CMS`
>
>`git clone --recursive ssh://git.amazon.com/pkg/CMS-Device-Example` (To be upated with new github URL)

Get ExpressIF esp32 tool chain: esp-idf release/v4.3. Follow [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/get-started/index.html) to setup the tool and learn how to use it.

> `git clone --recursive -b release/v4.3 git@github.com:espressif/esp-idf.git`

## **Setting up System**

### **1. Set up Cloud stuff**

Follow [this guide](https://docs.aws.amazon.com/solutions/latest/connected-vehicle-solution/welcome.html) to setup CMS cloud side framework related stuff.

### **2. Register Vehicle**

Follow [this guide](https://docs.aws.amazon.com/solutions/latest/connected-vehicle-solution/welcome.html) to register a vehicle and get back the following information:

* AWS IoT Core endpoint where the new vehicle is registered to. And it corresponding mqtt port number;
* mqtt client identification of new created vehicle;
* One AWS root CA. Rename it to "root_cert_auth.pem";
* The certificate and private key that attached to the AWS IoT Thing(the reprensentative of the new registered car). Rename certificate file to "client.crt". Rename private key file to "client.key";
* VIN(Vehicle Identification Number);

### **3. Configure OBD Dongle**

before starting compiling code, example needs to be proeprly configured as below:

1. Put AWS root CA file "`root_cert_auth.pem`" into folder `~/CMS/CMS-Device-Example/project/credentials`;
2. Put device certificate file "`client.crt`" and private key file "`client.key`" into folder `~/CMS/CMS-Device-Example/project/credentials`;
3. Run esp-idf menuconfig to change the following code configurations. Leave the rest as default. And Remember to save the chagne.
    >`cd ~/CMS/CMS-Device-Example/project`
    >
    >`idf.py menuconfig`
    * Set client identification that obtained from registration to `Example Configuration -> The MQTT client identifier used in this example`;
    * Set endpoint that obtained from registration to `Example Configuration -> Endpoint of the MQTT broker to connect to`;
    * Set mqtt port that obtained from registration to `Example Configuration -> Port of the MQTT broker used`;
    * Set VIN that obtained from registration to `Connected mobility demo config -> cms VIN`;
    * Set your wifi SSID to `Example Connection Configuration -> WiFi SSID`;
    * Set your wifi password to `Example Connection Configuration -> WiFi Password`;

## **Building and provisioning**

ExpressIF esp-idf V4.3 is used to complie this code. Please refer to [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/get-started/index.html) to setup the toolchaine, project environment and serial port driver. Then run the following commands to compile and monitor.

### **1. Compile code**
>`cd ~/CMS/CMS-Device-Example/project`
>
>`idf.py fullclean` (run this only a full and complete compile is required)
>
>`idf.py build`

### **2. Provision esp32 SOC**
>`cd ~/CMS/CMS-Device-Example/project`
>
>`idf.py flash`

### **3. Start/Restart code and monitor running status**
>`cd ~/CMS/CMS-Device-Example/project`
>
>`idf.py monitor`

### **4. Or all those steps can be combined into a single one**
>`cd ~/CMS/CMS-Device-Example/project`
>
>`idf.py build flash monitor`

## **Getting Started** 

After setting up cloud side stuff, and adding user account. Now it's ready to log into CMS fleet manager dashboard. Connect ECU simulator to the ODB dongle. And power up both of them. Then the new registered car shall be visible in CMS fleet manager dashboard. Select the new registerd car, the following information shall be seen from the right pannel:

* Instant geographical location (simulated when GPS signal is not ready);
* Car sofware version (now it's OBD dongle software version);
* Car Trouble codes (simulated by ECU simulator), speed, fuel level, odometer and oil temperature;
* Trip information.

## **Code Of Conduct**

[Code Of Conduct](CODE_OF_CONDUCT.md)

## **Security**
See [CONTRIBUTING](CONTRIBUTING.md) for more information.

## **License**

This library is licensed under the MIT-0 License. See the [LICENSE](LICENSE) file.