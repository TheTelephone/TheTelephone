TheTelephone
===

![TheTelephone-logo](TheTelephone-logo.svg)

TheTelephone provides components for [PureData](https://puredata.info/) to simulate speech telephony-related degradations.

This includes:

* simulation of [Voice-over-IP](https://en.wikipedia.org/wiki/Voice_over_IP) degradations (i.e., coding, compression, and network degradations),
* standardized degradations of the speech signal (e.g., [MNRU](https://en.wikipedia.org/wiki/Modulated_Noise_Reference_Unit) and resampling), and
* extended signal processing (e.g., voice activity detection).

Moreover, [PureData](https://puredata.info/) is extended with the following features:

* the capability of _offline processing_ (synchronous read and write),
* the communication via [Websockets](https://en.wikipedia.org/wiki/WebSocket), and
* a dynamic convolver (e.g., for binaural synthesis).

Actually, TheTelephone was implemented to build a _software-based telephone simulator_ for laboratory experiments on [Quality of Experience](https://en.wikipedia.org/wiki/Quality_of_experience).
For these experiments TheTelephone (simulating telephone connections) and [TheFragebogen](http://thefragebogen.de) (presenting questionnaires and coordinate the experiment) were used together.
For a detailed description of the used setup see [_Guse et al.:_ Multi-episodic perceived quality for one session of consecutive usage episodes with a speech telephony service
](https://doi.org/10.1007/s41233-017-0008-3)).

Build procedure
---
_ATTENTION:_ only the [externals](http://pdstatic.iem.at/externals-HOWTO/) are compiled (and later installed) for which _all_ required libraries are installed.  

For further information about installing [externals](http://pdstatic.iem.at/externals-HOWTO/), please take a look [here](https://puredata.info/docs/faq/how-do-i-install-externals-and-help-files).


### Linux-like (e.g., [Ubuntu 16.10](http://releases.ubuntu.com/16.10/) and [Windows 10](https://www.microsoft.com/en-us/windows) using [Ubuntu Bash](https://msdn.microsoft.com/en-us/commandline/wsl/)):

#### Install Tools and Dependencies

For [Ubuntu 16.10](http://releases.ubuntu.com/16.10/):
`sudo apt install build-essential git cmake puredata puredata-dev libsndfile-dev libresample-dev libfftw3-dev libwebsockets-dev libopus-dev libgsm1-dev libspeex-dev libspeexdsp-dev libjson-c-dev`

For [Windows 10](https://www.microsoft.com/en-us/windows) using [Ubuntu Bash](https://msdn.microsoft.com/en-us/commandline/wsl/)):
`sudo apt install build-essential git cmake puredata puredata-dev libsndfile-dev libresample-dev libfftw3-dev libwebsockets-dev libopus-dev libgsm1-dev libspeex-dev libspeexdsp-dev libjson-c-dev`

__NOTE__ for [Bash on Ubuntu on Windows](https://msdn.microsoft.com/en-us/commandline/wsl/):  
* this environment behaves like Ubuntu 14.04, and
* this environment does not provide a XServer and thus PureData's UI cannot be shown.

#### Compile and Install [__externals__](http://pdstatic.iem.at/externals-HOWTO/)

```shell
git clone https://github.com/TheTelephone/TheTelephone
cd TheTelephone && cmake . && cmake --build . && make install
```

### [MacOS](www.apple.com/macos/) using [Homebrew](http://brew.sh):

#### Install Tools and Dependencies

1. Install [PureData](https://puredata.info/docs/faq/macosx)
2. Make sure [PureData](https://puredata.info/docs/faq/macosx)'s header file [`m_pd.h`](https://github.com/pure-data/pure-data/blob/master/src/m_pd.h) is in your path (__should__ match your [PureData](https://puredata.info/) version).  
To grab the most recent, run: `curl https://raw.githubusercontent.com/pure-data/pure-data/master/src/m_pd.h > /usr/local/include/m_pd.h`
3. Install [Homebrew](http://brew.sh)
4. Install required tools and dependencies: `brew install cmake git libsndfile libresample fftw libwebsockets opus-tools libgsm speex json-c`

#### Compile and Install [__externals__](http://pdstatic.iem.at/externals-HOWTO/)

```shell
git clone https://github.com/TheTelephone/TheTelephone
cd TheTelephone && cmake . && cmake --build . && make install
```

### [Microsoft Windows 7/8/10](https://de.wikipedia.org/wiki/Microsoft_Windows):

__ATTENTION__: Following these instruction will only build a _small_ subset of the included externals as _no_ external libraries are installed.

#### Install Tools and Dependencies

1. Install [PureData](https://puredata.info/)
2. Install [cmake](https://cmake.org/)
3. Install [TDM-GCC](http://tdm-gcc.tdragon.net)

#### Compile and Install [__externals__](http://pdstatic.iem.at/externals-HOWTO/)

Run in shell (e.g., `cmd.exe`):
```shell
git clone https://github.com/TheTelephone/TheTelephone
cd TheTelephone
cmake -G"MinGW Makefiles" -DPUREDATA_ROOT="C:/Program Files (x86)/Pd/"
cmake --build .
```

__NOTE__: `PUREDATA_ROOT` needs to contain the correct  path to PureData.
__NOTE__: The compiled externals need to be installed _manually_.

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
