TheTelephone
===

Extending [PureData](https://puredata.info/) to build a _telephone simulator_.

This includes:

* simulation of [Voice-over-IP](https://en.wikipedia.org/wiki/Voice_over_IP) degradations (i.e., coding, compression, and network degradations),
* standardized degradations of the speech signal (e.g., [MNRU](https://en.wikipedia.org/wiki/Modulated_Noise_Reference_Unit) and resampling), and
* extended signal processing (e.g., voice activity detection).

Moreover, [PureData](https://puredata.info/) is extended with the following features:

* the capability of _offline processing_ (synchronous read and write),
* the communication via [Websockets](https://en.wikipedia.org/wiki/WebSocket), and
* a dynamic convolver (e.g., for binaural synthesis).

Build procedure
---
### Install dependencies

For [Ubuntu 16.10](http://releases.ubuntu.com/16.10/):
```bash
sudo apt install build-essential cmake puredata-dev libsndfile-dev libresample-dev libfftw3-dev libwebsockets-dev libopus-dev libgsm1-dev libspeex-dev libspeexdsp-dev libjson-c-dev
```

For [MacOS](www.apple.com/macos/) using [Homebrew](http://brew.sh):

1. Install [PureData](https://puredata.info/docs/faq/macosx)
2. Make sure [PureData](https://puredata.info/docs/faq/macosx)'s header file [`m_pd.h`](https://github.com/pure-data/pure-data/blob/master/src/m_pd.h) is in your path (__should__ match your [PureData](https://puredata.info/) version).  
To grab the most recent, run: `curl https://raw.githubusercontent.com/pure-data/pure-data/master/src/m_pd.h > /usr/local/include/m_pd.h`
3. Install [Homebrew](http://brew.sh)
4. `brew install cmake libsndfile libresample fftw libwebsockets opus-tools libgsm speex json-c`

### Compile

```bash
cd TheTelephone && cmake . && make
```

_ATTENTION:_ only the [externals](http://pdstatic.iem.at/externals-HOWTO/) are compiled (and later installed) for which _all_ required libraries are installed.  

### Install [__externals__](http://pdstatic.iem.at/externals-HOWTO/)

```bash
make install
```

By default the compiled [externals](http://pdstatic.iem.at/externals-HOWTO/) are installed into the home directory of the current user:
* Linux: `$ENV{HOME}/Library/Pd`
* MacOS: `$ENV{HOME}/pd-externals`

This can be overriden by setting `CMAKE_INSTALL_PREFIX`.

For further information about installing [externals](http://pdstatic.iem.at/externals-HOWTO/), please take a look [here](https://puredata.info/docs/faq/how-do-i-install-externals-and-help-files).

### Documentation

Documentation can be generated using [Doxygen](www.doxygen.org/).

For [Ubuntu 16.10](http://releases.ubuntu.com/16.10/) install the following packages:
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

For these [externals](http://pdstatic.iem.at/externals-HOWTO/), the license of the third-party code applies if this license is incompatbile to the [GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt) otherwise the [GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt) or later applies.

Authors
---
* Dennis Guse
* Frank Haase
