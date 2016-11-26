# dmxmaster
A simple FTDI proxy for Enttec Open DMX.

This program expects UDP packets of 512 bytes. Each byte represents a single DMX channel, each packet a universe. DmxMaster will then use the ftdi driver to write these packets out to the USB->DMX convertor.

This has been tested with the [Open DMX USB from Enttec](https://www.enttec.com/?main_menu=Products&pn=70303). It should be adaptable to other products.

The test.py file should give you a quick example of how to use dmxmaster.

## License
This work is licensed under the [MIT license](https://tldrlegal.com/license/mit-license)
