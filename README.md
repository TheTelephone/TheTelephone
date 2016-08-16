TheTelephone
===

This project extends [PureData](https://puredata.info/) to build a _telephone simulator_.

This includes:

* simulation of [Voice-over-IP](https://en.wikipedia.org/wiki/Voice_over_IP) degradations (i.&thinsp;e., coding, compression, and network degradations), 
* standardized degradations of the speech signal (e.&thinsp;g., [MNRU](https://en.wikipedia.org/wiki/Modulated_Noise_Reference_Unit) and resampling), and 
* extended signal processing (e.&thinsp;g., voice activity detection).

Moreover, [PureData](https://puredata.info/) is extended with the following features:

* the capability of _offline processing_ (synchronous read and write),
* the communication via [Websockets](https://en.wikipedia.org/wiki/WebSocket), and
* a dynamic convolver (mainly for binaural synthesis).

Build procedure
---
### Install dependencies

```bash
sudo apt install puredata-dev libsndfile-dev libresample-dev libfftw3-dev libwebsocket-dev libopus-dev libgsm1-dev libspeexdsp-dev
```

### Compile

```bash
cd TheTelephone && cmake . && make
```

### Install [__externals__](http://pdstatic.iem.at/externals-HOWTO/)

```bash
make install
```

By default [externals](http://pdstatic.iem.at/externals-HOWTO/) are installed into `$ENV{HOME}/pd-externals` of the current user.

This can be overriden setting `CMAKE_INSTALL_PREFIX`.

For further information about installing [externals](http://pdstatic.iem.at/externals-HOWTO/), please take a look [here](https://puredata.info/docs/faq/how-do-i-install-externals-and-help-files).

License
---
This project is licensed as [GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt) or later.

An exception are all externals linking or including third-party code.

For these [externals](http://pdstatic.iem.at/externals-HOWTO/), the license of the third-party code applies if this license is incompatbile to [GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt) otherwise the [GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt) is applied.

Authors
---
* Dennis Guse
* Frank Haase
