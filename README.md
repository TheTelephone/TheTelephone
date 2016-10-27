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

For [Ubuntu 16.04](http://releases.ubuntu.com/16.04/):
```bash
sudo apt install puredata-dev libsndfile-dev libresample-dev libfftw3-dev libwebsockets-dev libopus-dev libgsm1-dev libspeex-dev libspeexdsp-dev libjson0-dev
```

For [MacOS](www.apple.com/macos/) using [Homebrew](http://brew.sh):
1. Install [PureData](https://puredata.info/docs/faq/macosx)
2. Install [`m_pd.h`](https://github.com/pure-data/pure-data/blob/master/src/m_pd.h) into the search path (ATTENTION: use the exact version of your [PureData](https://puredata.info/)); most recent: `curl https://raw.githubusercontent.com/pure-data/pure-data/master/src/m_pd.h
 > /usr/local/include/m_pd.h`
3. Install [Homebrew](http://brew.sh)
4. `brew install cmake libsndfile libresample fftw libwebsockets opus-tools libgsm speex json-c`

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

### Documentation

Documentation can be generated using [Doxygen](www.doxygen.org/).

For [Ubuntu 16.04](http://releases.ubuntu.com/16.04/) install the following packages:
```bash
sudo apt install doxygen graphviz
```

Run:
```bash
cd TheTelephone && cmake . && make doc
```

License
---
This project is licensed as [GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt) or later.

An exception are all externals linking or including third-party code.

For these [externals](http://pdstatic.iem.at/externals-HOWTO/), the license of the third-party code applies if this license is incompatbile to [GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt) otherwise the [GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt) is applied.

Authors
---
* Dennis Guse
* Frank Haase
